/*
 * This code is provided solely for the personal and private use of students
 * taking the CSC369H course at the University of Toronto. Copying for purposes
 * other than this use is expressly prohibited. All forms of distribution of
 * this code, including but not limited to public repositories on GitHub,
 * GitLab, Bitbucket, or any other online platform, whether as given or with
 * any changes, are expressly prohibited.
 *
 * Authors: Alexey Khrabrov, Karen Reid
 *
 * All of the files in this directory and all subdirectories are:
 * Copyright (c) 2019 Karen Reid
 */

/**
 * CSC369 Assignment 1 - a1fs driver implementation.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
// Using 2.9.x FUSE API
#define FUSE_USE_VERSION 29
#define BMP_MEMBERS 64
#include <fuse.h>

#include "a1fs.h"
#include "fs_ctx.h"
#include "options.h"
#include "map.h"

//NOTE: All path arguments are absolute paths within the a1fs file system and
// start with a '/' that corresponds to the a1fs root directory.
//
// For example, if a1fs is mounted at "~/my_csc369_repo/a1b/mnt/", the path to a
// file at "~/my_csc369_repo/a1b/mnt/dir/file" (as seen by the OS) will be
// passed to FUSE callbacks as "/dir/file".
//
// Paths to directories (except for the root directory - "/") do not end in a
// trailing '/'. For example, "~/my_csc369_repo/a1b/mnt/dir/" will be passed to
// FUSE callbacks as "/dir".


// TODO: sultan included this, see if this works





/**
 * Initialize the file system.
 *
 * Called when the file system is mounted. NOTE: we are not using the FUSE
 * init() callback since it doesn't support returning errors. This function must
 * be called explicitly before fuse_main().
 *
 * @param fs    file system context to initialize.
 * @param opts  command line options.
 * @return      true on success; false on failure.
 */
static bool a1fs_init(fs_ctx *fs, a1fs_opts *opts)
{
	// Nothing to initialize if only printing help or version
	if (opts->help || opts->version) return true;

	size_t size;
	void *image = map_file(opts->img_path, A1FS_BLOCK_SIZE, &size);
	if (!image) return false;

	return fs_ctx_init(fs, image, size, opts);
}

/**
 * Cleanup the file system.
 *
 * Called when the file system is unmounted. Must cleanup all the resources
 * created in a1fs_init().
 */
static void a1fs_destroy(void *ctx)
{
	fs_ctx *fs = (fs_ctx*)ctx;
	if (fs->image) {
		if (fs->opts->sync && (msync(fs->image, fs->size, MS_SYNC) < 0)) {
			perror("msync");
		}
		munmap(fs->image, fs->size);
		fs_ctx_destroy(fs);
	}
}

/** Get file system context. */
static fs_ctx *get_fs(void)
{
	return (fs_ctx*)fuse_get_context()->private_data;
}


/**
 * Get file system statistics.
 *
 * Implements the statvfs() system call. See "man 2 statvfs" for details.
 * The f_bfree and f_bavail fields should be set to the same value.
 * The f_ffree and f_favail fields should be set to the same value.
 * The f_fsid and f_flag fields are ignored.
 *
 * @param path  path to any file in the file system. Can be ignored.
 * @param st    pointer to the struct statvfs that receives the result.
 * @return      0 on success; -errno on error.
 */
static int a1fs_statfs(const char *path, struct statvfs *st)
{
	(void)path;// unused
	fs_ctx *fs = get_fs();
	a1fs_superblock* superblock = (a1fs_superblock *)fs->image;

	memset(st, 0, sizeof(*st));
	st->f_bsize   = A1FS_BLOCK_SIZE;
	st->f_frsize  = A1FS_BLOCK_SIZE;
	//TODO
	st->f_bsize = A1FS_BLOCK_SIZE;
	st->f_files = superblock->num_inodes;
	st->f_bfree = superblock->num_unused_blocks;
	st->f_ffree = superblock->num_unused_inodes;
	st->f_bavail = superblock->num_unused_blocks;
	st->f_favail = superblock->num_unused_inodes;
	st->f_namemax = A1FS_NAME_MAX;
	
	return 0;

	//return -ENOSYS;
}

/**
 * Get file or directory attributes.
 *
 * Implements the stat() system call. See "man 2 stat" for details.
 * The st_dev, st_blksize, and st_ino fields are ignored.
 *
 * NOTE: the st_blocks field is measured in 512-byte units (disk sectors).
 *
 * Errors:
 *   ENAMETOOLONG  the path or one of its components is too long.
 *   ENOENT        a component of the path does not exist.
 *   ENOTDIR       a component of the path prefix is not a directory.
 *
 * @param path  path to a file or directory.
 * @param st    pointer to the struct stat that receives the result.
 * @return      0 on success; -errno on error;
 */
static int a1fs_getattr(const char *path, struct stat *st)
{

	if (strlen(path) >= A1FS_PATH_MAX) return -ENAMETOOLONG;
	fs_ctx *fs = get_fs();
	memset(st, 0, sizeof(*st));

	//NOTE: This is just a placeholder that allows the file system to be mounted
	// without errors. You should remove this from your implementation.
	// if (strcmp(path, "/") == 0) {
	// 	st->st_mode = S_IFDIR | 0777;
	// 	return 0;
	// }
	
	//1. parse the string of the path, and find the file that is being mentioned
	//	1. get the last name of the path, before the last /. 
	char filename[83];
	if (strcmp(path, "/") == 0){
		strcpy(filename, path);
	} else {
		int index_of_last_slash;
		for (int i = strlen(path) - 1; i >= 0; i--){
			if (path[i] == '/'){
				index_of_last_slash = i;
				break;
			}
		}
		uint32_t ind;
		uint32_t ind2 = 0;
		for (ind = index_of_last_slash; ind < strlen(path); ind++){
			filename[ind2] = path[ind];
			ind2++;
		}
		filename[ind2] = '\0';
	}
	
	// 	2. This would give us the file name we want to search, iterate over all inodes
	a1fs_superblock * spblk = ((a1fs_superblock *)(fs -> image));
	//int offset = (((a1fs_superblock *)(fs->image))->inode_table) * A1FS_BLOCK_SIZE;
	//p1fs_inode * inode_ptr = (a1fs_inode *)(fs->image + offset);
	


	int offset_for_inode_tbl = spblk -> inode_table * A1FS_BLOCK_SIZE;
	void * inode_tbl_ptr = fs->image + offset_for_inode_tbl;
	a1fs_inode * cur_inode = (a1fs_inode *)(inode_tbl_ptr);
	
	for (int i = 0; i < 64; i++){//TODO
	    if ((cur_inode->size > 0) && (cur_inode->mode == (S_IFDIR | 0777))){//Check whether the current inode contains stuff and is a directory
	        int num_dentries = cur_inode->size / sizeof(a1fs_dentry);
	        //for each extent, thats offset into data table, so go there, and increment the ptr
	        //Extent array has len 8
	        for (int j = 0; j < 8; j++){
	            a1fs_extent ext = cur_inode->extent_array[j];
	            if (ext.count != 0){
	                int data_tbl_offset = ext.start; 
	                void * data_table_ptr = fs->image + ((data_tbl_offset + spblk->data_table) * A1FS_BLOCK_SIZE);
	                a1fs_dentry * d_entry = (a1fs_dentry *)data_table_ptr;
	                for (int n = 0; n < num_dentries; n++){
	                    if (strcmp(d_entry->name, filename) == 0){
	                        st->st_size = cur_inode->size;
	                        st->st_mode = S_IFDIR | 0777;
	                        st->st_nlink = cur_inode->links;
	                        st->st_mtime = cur_inode->mtime.tv_sec;
	                        return 0;
	                    }
	                    d_entry += 1; //Go to the next dentry
	                }  
	            }
	        } 
	    }
	    cur_inode += 1; //Go to next inode
	}
	return -ENOENT;
}

/**
 * Read a directory.
 *
 * Implements the readdir() system call. Should call filler() for each directory
 * entry. See fuse.h in libfuse source code for details.
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" exists and is a directory.
 *
 * Errors:
 *   ENOMEM  not enough memory (e.g. a filler() call failed).
 *
 * @param path    path to the directory.
 * @param buf     buffer that receives the result.
 * @param filler  function that needs to be called for each directory entry.
 *                Pass 0 as offset (4th argument). 3rd argument can be NULL.
 * @param offset  unused.
 * @param fi      unused.
 * @return        0 on success; -errno on error.
 */
static int a1fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                        off_t offset, struct fuse_file_info *fi)
{
	fs_ctx * fs = get_fs();
	char dir_path[252];
	int end_offset = 1;
	if (strcmp(path, "/") == 0){
		strcpy(dir_path, path); 
	} else {
		if (path[strlen(path)-1] == '/'){
			// then ignore the last slash;
			end_offset = 2;
		} 
		int index_of_last_slash;
		for (int i = strlen(path) - end_offset; i >= 0; i--){
			if (path[i] == '/'){
				index_of_last_slash = i;
				break;
			}
		}
		uint32_t ind;
		uint32_t ind2 = 0;
		for (ind = index_of_last_slash; ind < strlen(path); ind++){
			dir_path[ind2] = path[ind];
			ind2++;
		}
		dir_path[ind2] = '\0';
	}
	
	// now go through all the inodes, and if it is a directory, try to find the dentry with the specified name
	void * offset_for_inodes = fs->image;
	a1fs_superblock * superblock = (a1fs_superblock *)offset_for_inodes;
	offset_for_inodes += superblock->inode_table * A1FS_BLOCK_SIZE;
	a1fs_inode * inode = (a1fs_inode *) offset_for_inodes;
	int found_inode_number = -1;
	for (int i = 0; i < (int)superblock->num_inodes; i ++){
		if (inode->mode == (S_IFDIR | 0777)){
			// then we have an inode that stores a directory, go through the dentries in the inode
			//int num_dentries = (int)(inode->size / sizeof(a1fs_dentry));
			for (int k = 0; k < 8; k++){
				a1fs_extent extent = inode->extent_array[k];
				if (extent.count > 0){
					for (int var = 0; var < (int)extent.count; var ++){
						// get to the first dentry from the extent 
						void * dentry_start_ptr = fs->image + A1FS_BLOCK_SIZE * (superblock->data_table + extent.start + var);
						a1fs_dentry * dentry = (a1fs_dentry *)dentry_start_ptr; 
						if (strcmp(dentry->name, dir_path) == 0){
							// file found
							found_inode_number = dentry->ino;
						}
					}
				}
			}
		}
	}
	// HERE, THE INODE OF THE DIRECTORY YOU ARE TRYING TO FIND SHOULD HAVE BEEN FOUND, AND SHOULD NOT BE -1;
	if (found_inode_number != -1){
		// call filler on all the entries in this inode
		void * found_inode = fs->image + A1FS_BLOCK_SIZE * (superblock->inode_table);
		while (found_inode_number > 0){
			//increment pointer, decrement counter
			found_inode += sizeof(a1fs_inode);
			found_inode_number --;
		}
		int num_dentries_in_found_inode = (int)(((a1fs_inode *)(found_inode))->size / sizeof(a1fs_dentry));
		if (num_dentries_in_found_inode > 0){
			//call filler on all these boys
			for (int haha = 0; haha < 8; haha ++){
				//iterate through the extent array and then there you will find the dentries
				a1fs_extent found_ino_extent = ((a1fs_inode *)(found_inode))->extent_array[haha];
				if (found_ino_extent.count > 0){
					// valid place to start finding dentries
					void * dentry_location = fs->image + A1FS_BLOCK_SIZE * (superblock->data_table + found_ino_extent.start);
					for (int lol = 0; lol < num_dentries_in_found_inode; lol++){
						// send all these into filler function
						a1fs_dentry * found_dentry = (a1fs_dentry *) dentry_location;
						filler(buf, found_dentry->name, NULL, 0);
						dentry_location += sizeof(a1fs_dentry);
					}
				}
			}
		} else {
			return -ENOMEM;
		}
	} else {
		return -ENOMEM;
	}







	// now if the path  == "/" then you go to the first inode and print all the dirents stored in the thing

	// if not, then you find the dirent with the name that is specified, access its inode, and print out all of the dirents stored there







	(void)offset;// unused
	(void)fi;// unused
	//fs_ctx *fs = get_fs();
	//void)path;
	//(void)filler;
	//(void)buf;

	//NOTE: This is just a placeholder that allows the file system to be mounted
	// without errors. You should remove this from your implementation.
	// if (strcmp(path, "/") == 0) {
	// 	filler(buf, "." , NULL, 0);
	// 	filler(buf, "..", NULL, 0);
	// 	return 0;
	// }

	//TODO
	(void)fs;
	return -ENOMEM;
}

void get_i_holder_and_j_holder(int *i_holder, int* j_holder, uint64_t* inode_bmp){
	for (int i = 0; i < BMP_MEMBERS; i ++){
		for (int bit = 0; bit < BMP_MEMBERS; bit++){
			int inode_bmp_bit = ((inode_bmp[i] & (1<<bit))>>bit);
			if(inode_bmp_bit == 0){
				*i_holder = i;
				*j_holder = bit;
				inode_bmp_bit = (inode_bmp[i] | (1 << bit));
				return;
			}
		}
	}
}


/**
 * Create a directory.
 *
 * Implements the mkdir() system call.
 *
 * NOTE: the mode argument may not have the type specification bits set, i.e.
 * S_ISDIR(mode) can be false. To obtain the correct directory type bits use
 * "mode | S_IFDIR".
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" doesn't exist.
 *   The parent directory of "path" exists and is a directory.
 *   "path" and its components are not too long.
 *
 * Errors:
 *   ENOMEM  not enough memory (e.g. a malloc() call failed).
 *   ENOSPC  not enough free space in the file system.
 *
 * @param path  path to the directory to create.
 * @param mode  file mode bits.
 * @return      0 on success; -errno on error.
 */
static int a1fs_mkdir(const char *path, mode_t mode)
{
	fs_ctx *fs = get_fs();

	// iterate over inodes and find first available inode
	a1fs_superblock* superblock = (a1fs_superblock *)fs->image;
	uint64_t * inode_bmp = fs->image + superblock->inode_bmp * A1FS_BLOCK_SIZE;
	int i_holder = 0;
	int j_holder = 0;
	get_i_holder_and_j_holder(&i_holder, &j_holder, inode_bmp);
	// find the offset for the inode you need to access
	int inode_table_access_offset = i_holder * BMP_MEMBERS + j_holder;
	// setting the inode properties 
	a1fs_inode * inode_ptr = (a1fs_inode *)(((a1fs_superblock *)(fs->image))->inode_table * A1FS_BLOCK_SIZE + sizeof(a1fs_inode) * inode_table_access_offset); // MAKE SURE THIS THING IS CORRECT
	// TODO: set the inode_ptr -> mode attribute of the inode struct
	inode_ptr->links = 2;
	inode_ptr->size = fs->size; // TODO:  CHECK IF THIS WORKS ;ALSDKFJ;ASLFJ;ASDKLJF;ASDKLFJ
	clock_gettime(CLOCK_REALTIME, &(inode_ptr->mtime));
	inode_ptr->overflow_pointer = NULL;
	inode_ptr->overflow_flag = 0;
	inode_ptr->mode = mode;
	
	// Finding first available block that is open -> we can store the two initial dentries in it since they are small enough
	int i_first_open_bit = 0;
    int j_first_open_bit = 0;
	uint64_t * datablock_bmp = fs->image + (superblock->datablock_bmp * A1FS_BLOCK_SIZE);
	//Navigate to the beginning of the datablock bitmap
	for (int i = 0; i < BMP_MEMBERS; i++){//BMP_MEMBERS is number of top-level items in the bitmap array
		for (int bit = 0; bit < BMP_MEMBERS; bit++){
			int blockBitsInt = ((datablock_bmp[i] & (1<<bit))>>bit);
			if (blockBitsInt == 0){
					i_first_open_bit = i;
					j_first_open_bit = bit;
					blockBitsInt = (datablock_bmp[i] | (1<<bit)); // setting the corresponding inode bit
					break;
			}
		}
    }
	int block_bitmap_offset = (i_first_open_bit * BMP_MEMBERS) + j_first_open_bit;
	//TODO: Need to do error checking here - what if the file system is full?
	int data_table = ((a1fs_superblock *)(fs->image))->data_table;
	int free_block_location = (data_table + block_bitmap_offset) * A1FS_BLOCK_SIZE;
	a1fs_dentry * free_block = (a1fs_dentry *)((fs->image) + free_block_location); //Get the free block
	//Now, we create the two directory entries, and add them to the free block that we just found.
	a1fs_dentry self_ref;
	strcpy(self_ref.name, ".\0");
	self_ref.ino = inode_table_access_offset;

	a1fs_dentry parent_ref;
	strcpy(parent_ref.name, "..\0");
	parent_ref.ino = fs->current_inode_offset;
	//Then, add the two dentries into the free_block
	*free_block = self_ref;
	free_block += sizeof(a1fs_dentry);
	*free_block = parent_ref;

	//TODO
	(void)path;
	(void)mode;
	(void)fs;
	return 0;
	//return -ENOSYS;
}

/**
 * Remove a directory.
 *
 * Implements the rmdir() system call.
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" exists and is a directory.
 *
 * Errors:
 *   ENOTEMPTY  the directory is not empty.
 *
 * @param path  path to the directory to remove.
 * @return      0 on success; -errno on error.
 */
static int a1fs_rmdir(const char *path)
{
	fs_ctx *fs = get_fs();

	//TODO
	(void)path;
	(void)fs;
	return -ENOSYS;
}

/**
 * Create a file.
 *
 * Implements the open()/creat() system call.
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" doesn't exist.
 *   The parent directory of "path" exists and is a directory.
 *   "path" and its components are not too long.
 *
 * Errors:
 *   ENOMEM  not enough memory (e.g. a malloc() call failed).
 *   ENOSPC  not enough free space in the file system.
 *
 * @param path  path to the file to create.
 * @param mode  file mode bits.
 * @param fi    unused.
 * @return      0 on success; -errno on error.
 */
static int a1fs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	(void)fi;// unused
	assert(S_ISREG(mode));
	fs_ctx *fs = get_fs();

	//TODO
	(void)path;
	(void)mode;
	(void)fs;
	return -ENOSYS;
}

/**
 * Remove a file.
 *
 * Implements the unlink() system call.
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" exists and is a file.
 *
 * @param path  path to the file to remove.
 * @return      0 on success; -errno on error.
 */
static int a1fs_unlink(const char *path)
{
	fs_ctx *fs = get_fs();

	//TODO
	(void)path;
	(void)fs;
	return -ENOSYS;
}

/**
 * Rename a file or directory.
 *
 * Implements the rename() system call.
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "from" exists.
 *   The parent directory of "to" exists and is a directory.
 *
 * Errors:
 *   ENOMEM  not enough memory (e.g. a malloc() call failed).
 *   ENOSPC  not enough free space in the file system.
 *
 * @param from  original file path.
 * @param to    new file path.
 * @return      0 on success; -errno on error.
 */
static int a1fs_rename(const char *from, const char *to)
{
	fs_ctx *fs = get_fs();
	//I think we should use search, find inode referred to by path, and then change the name in the dentry? to the **to path.
	//TODO
	(void)from;
	(void)to;
	(void)fs;
	return -ENOSYS;
}


/**
 * Change the access and modification times of a file or directory.
 *
 * Implements the utimensat() system call. See "man 2 utimensat" for details.
 *
 * NOTE: You only have to implement the setting of modification time (mtime).
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" exists.
 *
 * @param path  path to the file or directory.
 * @param tv    timestamps array. See "man 2 utimensat" for details.
 * @return      0 on success; -errno on failure.
 */
static int a1fs_utimens(const char *path, const struct timespec tv[2])
{
	fs_ctx *fs = get_fs();
	//I think that we should use the search function to find the inode, and then update the 
	//TODO
	(void)path;
	(void)tv;
	(void)fs;
	return -ENOSYS;
}

/**
 * Change the size of a file.
 *
 * Implements the truncate() system call. Supports both extending and shrinking.
 * If the file is extended, future reads from the new uninitialized range must
 * return zero data.
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" exists and is a file.
 *
 * Errors:
 *   ENOMEM  not enough memory (e.g. a malloc() call failed).
 *   ENOSPC  not enough free space in the file system.
 *
 * @param path  path to the file to set the size.
 * @param size  new file size in bytes.
 * @return      0 on success; -errno on error.
 */
static int a1fs_truncate(const char *path, off_t size)
{
	fs_ctx *fs = get_fs();

	//TODO
	(void)path;
	(void)size;
	(void)fs;
	return -ENOSYS;
}


/**
 * Read data from a file.
 *
 * Implements the pread() system call. Should return exactly the number of bytes
 * requested except on EOF (end of file) or error, otherwise the rest of the
 * data will be substituted with zeros. Reads from file ranges that have not
 * been written to must return zero data.image
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" exists and is a file.
 *
 * @param path    path to the file to read from.
 * @param buf     pointer to the buffer that receives the data.
 * @param size    buffer size - number of bytes requested.
 * @param offset  offset from the beginning of the file to read from.
 * @param fi      unused.
 * @return        number of bytes read on success; 0 if offset is beyond EOF;
 *                -errno on error.
 */
static int a1fs_read(const char *path, char *buf, size_t size, off_t offset,
                     struct fuse_file_info *fi)
{
	(void)fi;// unused
	fs_ctx *fs = get_fs();

	//TODO
	(void)path;
	(void)buf;
	(void)size;
	(void)offset;
	(void)fs;
	return -ENOSYS;
}

/**
 * Write data to a file.
 *
 * Implements the pwrite() system call. Should return exactly the number of
 * bytes requested except on error. If the offset is beyond EOF (end of file),
 * the file must be extended. If the write creates a "hole" of uninitialized
 * data, future reads from the "hole" must return zero data.
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" exists and is a file.
 *
 * @param path    path to the file to write to.
 * @param buf     pointer to the buffer containing the data.
 * @param size    buffer size - number of bytes requested.
 * @param offset  offset from the beginning of the file to write to.
 * @param fi      unused.
 * @return        number of bytes written on success; -errno on error.
 */
static int a1fs_write(const char *path, const char *buf, size_t size,
                      off_t offset, struct fuse_file_info *fi)
{
	(void)fi;// unused
	fs_ctx *fs = get_fs();

	//TODO
	(void)path;
	(void)buf;
	(void)size;
	(void)offset;
	(void)fs;
	return -ENOSYS;
}


static struct fuse_operations a1fs_ops = {
	.destroy  = a1fs_destroy,
	.statfs   = a1fs_statfs,
	.getattr  = a1fs_getattr,
	.readdir  = a1fs_readdir,
	.mkdir    = a1fs_mkdir,
	.rmdir    = a1fs_rmdir,
	.create   = a1fs_create,
	.unlink   = a1fs_unlink,
	.rename   = a1fs_rename,
	.utimens  = a1fs_utimens,
	.truncate = a1fs_truncate,
	.read     = a1fs_read,
	.write    = a1fs_write,
};

int main(int argc, char *argv[])
{
	a1fs_opts opts = {0};// defaults are all 0
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	if (!a1fs_opt_parse(&args, &opts)) return 1;

	fs_ctx fs = {0};
	if (!a1fs_init(&fs, &opts)) {
		fprintf(stderr, "Failed to mount the file system\n");
		return 1;
	}

	return fuse_main(args.argc, args.argv, &a1fs_ops, &fs);
}

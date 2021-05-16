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
 * CSC369 Assignment 1 - a1fs formatting tool.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#include <fcntl.h>

#include "a1fs.h"
#include "map.h"



//TODO: SULTAN INCLUDED THIS
#include "fs_ctx.h"


/** Command line options. */
typedef struct mkfs_opts {
	/** File system image file path. */
	const char *img_path;
	/** Number of inodes. */
	size_t n_inodes;

	/** Print help and exit. */
	bool help;
	/** Overwrite existing file system. */
	bool force;
	/** Sync memory-mapped image file contents to disk. */
	bool sync;
	/** Verbose output. If false, the program must only print errors. */
	bool verbose;
	/** Zero out image contents. */
	bool zero;

} mkfs_opts;

static int num_inodes_required = 0;

static const char *help_str = "\
Usage: %s options image\n\
\n\
Format the image file into a1fs file system. The file must exist and\n\
its size must be a multiple of a1fs block size - %zu bytes.\n\
\n\
Options:\n\
    -i num  number of inodes; required argument\n\
    -h      print help and exit\n\
    -f      force format - overwrite existing a1fs file system\n\
    -s      sync image file contents to disk\n\
    -v      verbose output\n\
    -z      zero out image contents\n\
";

static void print_help(FILE *f, const char *progname)
{
	fprintf(f, help_str, progname, A1FS_BLOCK_SIZE);
}


static bool parse_args(int argc, char *argv[], mkfs_opts *opts)
{
	char o;
	while ((o = getopt(argc, argv, "i:hfsvz")) != -1) {
		switch (o) {
			case 'i': opts->n_inodes = strtoul(optarg, NULL, 10); break;

			case 'h': opts->help    = true; return true;// skip other arguments
			case 'f': opts->force   = true; break;
			case 's': opts->sync    = true; break;
			case 'v': opts->verbose = true; break;
			case 'z': opts->zero    = true; break;

			case '?': return false;
			default : assert(false);
		}
	}

	if (optind >= argc) {
		fprintf(stderr, "Missing image path\n");
		return false;
	}
	opts->img_path = argv[optind];

	if (opts->n_inodes == 0) {
		fprintf(stderr, "Missing or invalid number of inodes\n");
		return false;
	}
	return true;
}


/** Determine if the image has already been formatted into a1fs. */
static bool a1fs_is_present(void *image)
{
	//TODO
	a1fs_superblock * superblock_location = (a1fs_superblock*) image + A1FS_BLOCK_SIZE;	
	uint64_t magic = (uint64_t) superblock_location->magic;
	return magic == A1FS_MAGIC;
}


/**
 * Format the image into a1fs.
 *
 * NOTE: Must update mtime of the root directory.
 *
 * @param image  pointer to the start of the image.
 * @param size   image size in bytes.
 * @param opts   command line options.
 * @return       true on success;
 *               false on error, e.g. options are invalid for given image size.
 */
int Ceil(float num){
    int current = (int) num;
    int next = current + 1;
    if (num == current){
        return current;
    } else {
        return next;
    }
}

static bool mkfs(void *image, size_t size, mkfs_opts *opts)
{
	//TODO
	uint64_t offset_for_data_table = 0;
	int overall_offset = 0;
	num_inodes_required = opts->n_inodes;
	int num_inode_bmp_blocks = Ceil((float) opts->n_inodes / (A1FS_BLOCK_SIZE * 8));
	int num_blocks_inode_table = Ceil((float) (opts->n_inodes * sizeof(a1fs_inode)/A1FS_BLOCK_SIZE));
	int num_blocks_in_file = Ceil((float) (size/A1FS_BLOCK_SIZE));
	int available_blocks = num_blocks_in_file - num_inode_bmp_blocks - num_blocks_inode_table - 1;
	int num_blocks_data_bmp = 1;
	a1fs_dentry self = {0};
	a1fs_dentry parent_self = {0};
	a1fs_dentry first_dentry = {0};
	while (true){
		int file_blocks = available_blocks - num_blocks_data_bmp;
		if (file_blocks > num_blocks_data_bmp * A1FS_BLOCK_SIZE *8){
			num_blocks_data_bmp++;
			available_blocks --;
		} else {
			break;
		}
	}
	// Start looping -> first block is the superblock
	// Then come the inode bitmap and the data bitmap
	// and finally the inode table and the data table
	a1fs_superblock superblock;
	for (int i = 0; i < num_blocks_in_file; i++){
		//iterating till num_blocks_in_file because break when youre done making all the inode stucts
		int offset_in_image = i * A1FS_BLOCK_SIZE;
		a1fs_inode inode_table[opts->n_inodes];
		void * location = (void *) image + offset_in_image;  
		if (i == 0){
			//set up the superblock
			
			superblock.inode_bmp = 1;
			superblock.datablock_bmp = superblock.inode_bmp + num_inode_bmp_blocks;
			
			superblock.inode_table = superblock.datablock_bmp + num_blocks_data_bmp;
			superblock.data_table = superblock.inode_table + num_blocks_inode_table;
			superblock.num_inodes = opts->n_inodes;
			superblock.num_blocks = available_blocks;
			superblock.num_unused_inodes = superblock.num_inodes;
			superblock.num_unused_blocks = superblock.num_blocks;
			a1fs_superblock * location2 = (a1fs_superblock *)location;
			memcpy(location2, &superblock, sizeof(a1fs_superblock));
		}
		if ((i >= 1)&&(i < superblock.datablock_bmp)){//ASK SULTAN TO CHECK DIS
			//set up the inode bitmap
			uint64_t inode_bmp[64];
			for (int j = 0; j < 64; j++){
				inode_bmp[j] = 0;
			}
			uint64_t * location2 = (uint64_t *)location;
			memcpy(location2, inode_bmp, sizeof(uint64_t) * 64);
		}
		if ((i >= superblock.datablock_bmp)&&(i < superblock.inode_table)){
			/// set up the data block bitmap
			uint64_t datablock_bmp[64];
			for (int k = 0; k < 64; k++){
				datablock_bmp[k] = 0;
			}
			uint64_t * location2 = (uint64_t*)location;
			memcpy(location2, datablock_bmp, sizeof(uint64_t) * 64);
		}
		if ((i >= superblock.inode_table)&&(i < superblock.data_table)){
			//set up inode table	
			offset_for_data_table = ((a1fs_superblock *)image)->inode_table * A1FS_BLOCK_SIZE;
			int offset_into_inode_table = i - superblock.inode_table; //Gives the block number within the inode table, e.g. the third block of the inode blocks
			overall_offset = offset_into_inode_table * A1FS_BLOCK_SIZE + offset_for_data_table;
			int num_inodes_per_block = (int) A1FS_BLOCK_SIZE / sizeof(a1fs_inode);
			for (int m = 0; m < num_inodes_per_block; m++){
				int constant = m + offset_into_inode_table * (int)(A1FS_BLOCK_SIZE / sizeof(a1fs_inode));
				struct a1fs_inode inode;
				inode.links = 0; 
				for (int asdf = 0; asdf < 8; asdf ++){
				    a1fs_extent extnt = {0};
				    extnt.start = 0;
				    extnt.count = 0;
				    inode.extent_array[asdf] = extnt;
				}
				inode.overflow_pointer = NULL;
				char buff[83];
				sprintf(buff, "%d", m + offset_into_inode_table * (int)(A1FS_BLOCK_SIZE / sizeof(a1fs_inode)));
				strcpy(inode.garbage, buff);
				inode.overflow_flag = 0;
				clock_gettime(CLOCK_REALTIME, &inode.mtime);// TODO: check if CLOCK_REALTIME OR REALTIME
				self.ino = 0;
				parent_self.ino = 0;
				if (constant == 0){
					inode.mode = S_IFDIR | 0777;
					inode.links = 1;
					inode.size = sizeof(a1fs_dentry);
					clock_gettime(CLOCK_REALTIME, &(inode.mtime));

					a1fs_extent ext = {0};
					ext.start = 0;
					ext.count = 1;
					inode.extent_array[0] = ext;

					int first_data_block_dirent = A1FS_BLOCK_SIZE * superblock.data_table;
					void * first_dentry_location = image + first_data_block_dirent;
					//a1fs_dentry * first_dentry;
					strcpy(first_dentry.name, "/");
					first_dentry.ino = 1;
					//memcpy(first_inode_ptr, fst_inode, sizeof(a1fs_inode));
					memcpy(first_dentry_location, &first_dentry, sizeof(a1fs_dentry));
				}
				else if (constant == 1){
					//a1fs_inode * dentry_inode;
					inode.mode = S_IFDIR | 0777;
					inode.links = 2;
					inode.size = sizeof(a1fs_dentry) * 2;
					clock_gettime(CLOCK_REALTIME, &(inode.mtime));

					self.ino = 1;
					parent_self.ino = 1;
					strcpy(self.name, ".");
					strcpy(parent_self.name, "..");

					a1fs_extent newext;
					newext.count = 1;
					newext.start = 1;
					inode.extent_array[0] = newext;

					void * dentry_location = image + A1FS_BLOCK_SIZE * (superblock.data_table + 1);
					memcpy(dentry_location, &self, sizeof(a1fs_dentry));
					dentry_location += sizeof(a1fs_dentry);
					memcpy(dentry_location, &parent_self, sizeof(a1fs_dentry));
				} 

				inode_table[m + offset_into_inode_table * (int)(A1FS_BLOCK_SIZE / sizeof(a1fs_inode))] = inode;	
				fflush(stdout);
				overall_offset += sizeof(a1fs_inode);
			}
			memcpy(image + offset_for_data_table, &inode_table, sizeof(a1fs_inode) * opts->n_inodes);
			msync(image + offset_for_data_table, sizeof(a1fs_inode) * opts->n_inodes, MS_SYNC);
		}
	}
	(void)size;
	(void)opts;
	return true;
}


int main(int argc, char *argv[])
{
	mkfs_opts opts = {0};// defaults are all 0
	if (!parse_args(argc, argv, &opts)) {
		// Invalid arguments, print help to stderr
		print_help(stderr, argv[0]);
		return 1;
	}
	if (opts.help) {
		// Help requested, print it to stdout
		print_help(stdout, argv[0]);
		return 0;
	}

	// Map image file into memory
	size_t size;
	void *image = map_file(opts.img_path, A1FS_BLOCK_SIZE, &size);
	if (image == NULL) return 1;

	// Check if overwriting existing file system
	int ret = 1;
	if (!opts.force && a1fs_is_present(image)) {
		fprintf(stderr, "Image already contains a1fs; use -f to overwrite\n");
		goto end;
	}

	if (opts.zero) memset(image, 0, size);
	if (!mkfs(image, size, &opts)) {
		fprintf(stderr, "Failed to format the image\n");
		goto end;
	}

	// Sync to disk if requested
	if (opts.sync && (msync(image, size, MS_SYNC) < 0)) {
		perror("msync");
		goto end;
	}

	ret = 0;
end:
	munmap(image, size);
	return ret;
}

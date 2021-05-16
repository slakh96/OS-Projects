"""
File Name: referenceTracer.py
Author: Shahzil Lakhani and Abhishek Kapoor
Date: November 25, 2019
"""

"""
INSTRUCTIONS FOR HOW TO RUN THIS PROGRAM:  
This program takes as input a reference file (given as a file path) and prints all the relevant information and table. 
On line 79, this function is called with a sample reference file path parameter. Change this path to a different 
reference file in order to run this program on a different reference file. 

This program will translate each memory reference into a page number (assuming a page size of 4096 bytes). 
This program will output a table of each unique instruction pages with a count of the number of accesses for each page,
and a table of unique data pages with the corresponding count of the number of accesses for each page.
"""

def produceTable(fileName):
    """
    This is a function that takes a file path and reads it to generate the table output
    :param fileName: A string representing the path of the file (or just name if the file is in the same directory)
    """

    traceFile = open(fileName, "r")
    count_dictionary = {'S': 0, 'I': 0, 'L': 0, 'M': 0}
    instruction_dictionary = {}
    data_dictionary = {}

    line = traceFile.readline()
    while line != "":
        master_list = line.lstrip().replace("  ", " ").replace(" ", ",").rstrip().split(",")
        if master_list[0] in count_dictionary:
            count_dictionary[master_list[0]] += 1
        else:
            print("Error Occurred: Key not found in the dictionary")

        page_number = "0x" + master_list[1][0:len(master_list[1]) - 3] + "000"
        if master_list[0] == 'I':
            if page_number in instruction_dictionary:
                instruction_dictionary[page_number] += 1
            else:
                instruction_dictionary[page_number] = 1
        else:
            if page_number in data_dictionary:
                data_dictionary[page_number] += 1
            else:
                data_dictionary[page_number] = 1
        line = traceFile.readline()

    string_to_print = \
    """
Counts:
  Instructions {}
  Loads        {}
  Stores       {}
  Modifies     {}
    """.format(count_dictionary["I"], count_dictionary["L"], count_dictionary["S"], count_dictionary["M"])

    print(string_to_print)

    print("Instructions: ")
    for instruction_address in instruction_dictionary:
        print(instruction_address + "," + str(instruction_dictionary[instruction_address]))
    print("Data: ")
    for data_address in data_dictionary:
        print(data_address + "," + str(data_dictionary[data_address]))

    # print("NUMBER OF INSTRUCTION PAGES ", len(instruction_dictionary))
    # print("NUMBER OF DATA PAGES ", len(data_dictionary))




if __name__ == "__main__":
    # Change this file path variable to execute this program with a different reference file
    reference_file_path = "/u/csc369h/fall/pub/a3/traces/addr-simpleloop.ref"
    # invoke the function itself
    produceTable(reference_file_path)


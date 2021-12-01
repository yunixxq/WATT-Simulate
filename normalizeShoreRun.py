#!/bin/python3

import sys
import pandas

def run(in_file, out_file):
    input_file = open(in_file, "r")
    output_file = open(out_file, "w")
    output_file.write("pages,is_write\n")
    last_valid_line = ""
    while True:
        line = input_file.readline()
        if not line:
            break
        if not "unfix" in line:
            continue
        if line == last_valid_line:
            continue
        last_valid_line = line
        pid = int(line.split(")")[0].split(".")[-1])
        is_write = "dirty" in line
        output_file.write(str(pid) + "," + str(is_write) + "\n")

    output_file.close()


if __name__ == "__main__":
    if len(sys.argv) ==3:
        print("Converting: ", sys.argv[1], " to ", sys.argv[2])
        run(sys.argv[1], sys.argv[2])
    else:
        print("Usage:")
        print(sys.argv[0], " shore-kits-trace-in normalized-trace-out.csv")

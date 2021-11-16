#!/bin/python3

import matplotlib.pyplot as plt
import sys
from joblib import Parallel, delayed
import pandas

def isWrite(zugriff):
    return "Mark page dirty" in zugriff

def isRead(zugriff):
    return "Resolving Swip" in zugriff

def getWorker(zugriff):
    return int(zugriff.split(" ")[0])

def getCommand(zugriff):
    return zugriff.split(":")[0].split(" ", 1)[1]

def getPid(zugriff): # Remove /n at end
    return int(zugriff.split(": ")[1])

def getAccesses(zugriffe):
    return list(filter(lambda x: isRead(x) or isWrite(x), zugriffe))

def getReads(zugriffe):
    return list(filter(lambda x: isRead(x), zugriffe))

def getWrites(zugriffe):
    return list(filter(lambda x: isWrite(x), zugriffe))

def getAccessFromFile(file):
    datei = open(file, "r")
    zugriffe = datei.readlines()
    return getAccesses(zugriffe)
    
def do_run(in_file, out_file):

    valid_lines = getAccessFromFile(in_file)
    pids = map(lambda x: getPid(x), valid_lines)
    is_write = map(lambda x: isWrite(x), valid_lines)
    df = {"pages": pids, "is_write": is_write}
    df = pandas.DataFrame(data=df)
    df.to_csv(out_file, index=False)

# do_run("traces/test.txt", "tpcc_64_-5.csv")


if __name__ == "__main__":
    if len(sys.argv) ==3:
        print("Converting: ", sys.argv[1], " to ", sys.argv[2])
        do_run(sys.argv[1], sys.argv[2])
    else:
        print("Usage:")
        print(sys.argv[0], " leanstore-trace-in normalized-trace-out.csv")
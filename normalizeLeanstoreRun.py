#!/bin/python3

from os import access
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
    return int(zugriff.split(": ")[1].replace('.', ''))

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

def removePreWriteRead(pids, is_write):
    delete_access = False
    accesses_ago = 0
    delete_pid = 0
    pid_out = []
    is_write_out = []
    for (pid, write) in reversed(list(zip(pids, is_write))):
        if delete_access:
            if delete_pid == pid:
                delete_access = False
                if accesses_ago > 2 or write:
                    print("PID access ", pid, " removed, was Write: ", write, " write was ", accesses_ago, " ago.")
                continue
            else:
                accesses_ago += 1

        pid_out.append(pid)
        is_write_out.append(write)
        if write:
            delete_access = True
            accesses_ago = 0
            delete_pid = pid
    return (reversed(pid_out), reversed(is_write_out))
        

    
def do_run(in_file, out_file):
    print("Reading lines")
    valid_lines = getAccessFromFile(in_file)
    print("Getting pids")
    pids = list(map(lambda x: getPid(x), valid_lines))
    print("Getting writes")
    is_write = list(map(lambda x: isWrite(x), valid_lines))
    print("Removing preWriteReads")
    (pids, is_write) = removePreWriteRead(pids, is_write)
    print("printing to file")
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
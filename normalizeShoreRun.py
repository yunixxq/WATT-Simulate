#!/bin/python3

import matplotlib.pyplot as plt
import os, shutil, random, time, json, csv, sys
from joblib import Parallel, delayed
import pandas

def isFix(zugriff):
    return "fix " in zugriff and " in mode " in zugriff

def cleanInfo(zugriff):
    return "Page " in zugriff and " is clean" in zugriff

def isUnfix(zugriff):
    return "unfix " in zugriff

def isUnfixClean(zugriff):
    return "unfix clean " in zugriff

def isUnfixDirty(zugriff):
    return "unfix dirty " in zugriff

def isRefix(zugriff):
    return "Refix" in zugriff

def isMarkClean(zugriff):
    return "Mark " in zugriff and "clean" in zugriff

def isMarkDirty(zugriff):
    return "Mark " in zugriff and "dirty" in zugriff

def isUpgrade(zugriff):
    return "Upgrading Latch " in zugriff

def getPidStr(zugriff): # Remove /n at end
    return zugriff.split("(")[1].split(")")[0]

def getPid(pages: list, zugriff):
    return pages.index(getPidStr(zugriff))

def getRelevant(zugriffe):
    return list(filter(lambda x: isFix(x) or isUnfix(x) or cleanInfo(x) or isRefix(x), zugriffe))

def getErrors(zugriffe):
    return list(filter(lambda x: not (isFix(x) or cleanInfo(x) or isUnfix(x) or isMarkClean(x) or isRefix(x) or isMarkDirty(x) or isUpgrade(x)), zugriffe))

def getAccessFromFile(file):
    datei = open(file, "r")
    zugriffe = datei.readlines()
    return zugriffe

def generateOutput(unfixes, out_file):
    pages = list(set(map(getPidStr, unfixes)))
    print("Dataset contains " + str(len(pages)) + " pages")
    print("Dataset contains " + str(len(unfixes)) + " unfixes")
    page_hash = {}
    for page in range(0, len(pages)):
        page_hash[pages[page]] = page
    pids = list(map(lambda x: page_hash[getPidStr(x)], unfixes))
    is_write = list(map(lambda x: isUnfixDirty(x), unfixes))
    df = {"pages": pids, "is_write": is_write}
    df = pandas.DataFrame(data=df)
    df.to_csv(out_file, index=False)

    
def do_run(in_file, out_file):

    lines = getAccessFromFile(in_file)
    print(getErrors(lines)[:10])

    unfixes = list(filter(lambda x: isUnfix(x), lines))

    if len(lines) < 1000000:
        fixes = list(filter(lambda x: isFix(x), lines))
        cleanUnfixes = list(filter(lambda x: isUnfixClean(x), lines))
        dirtyUnfixes = list(filter(lambda x: isUnfixDirty(x), lines))
        refixes = list(filter(lambda x: isRefix(x), lines))
        if(len(fixes + refixes) != len(unfixes)):
            print("Trace might be from broken run! not all fixed pages get unfixed")
        assert(len(cleanUnfixes + dirtyUnfixes) == len(unfixes))
        
    
    generateOutput(unfixes, out_file)
    if False:
        print("Fixes: "+ str(len(fixes)))
        print("Unfixes: "+ str(len(unfixes)))
        print("UnfixesClean: "+ str(len(cleanUnfixes)))
        print("UnfixesDirty: "+ str(len(dirtyUnfixes)))
        print("Refixes: "+ str(len(refixes)))
        print ("fix + refix: " + str(len(fixes) + len(refixes)))
        print("Pages: " + str(len(pages)))


if __name__ == "__main__":
    if len(sys.argv) ==3:
        print("Converting: ", sys.argv[1], " to ", sys.argv[2])
        do_run(sys.argv[1], sys.argv[2])
    else:
        print("Usage:")
        print(sys.argv[0], " shore-kits-trace-in normalized-trace-out.csv")

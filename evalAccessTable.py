#!/bin/python3

import matplotlib.pyplot as plt
import os, random, time
from joblib import Parallel, delayed
import pandas


def evictMinPage(ram: dict, dirtyInRam: set):
    return min(ram, key=lambda x: ram[x])

def randomFindPageToEvict(ram: dict, dirtyInRam: set):
    return random.choice(list(ram))

def evict_lru_strange(ram: dict, dirtyInRam: set):
    return min(ram, key=lambda x: ram[x] +40 if x in dirtyInRam else ram[x])

# FlagFunction: [pos, nextZugriff, write, dirty]
def executeStrategy(pidAndNextAndWrite, ramSize, flagFunction, evictFinder, heatUp= 0):
    currentRam = {}
    dirtyInRam = set()
    pageMisses = 0
    dirtyEvicts = 0
    for pos in range(0, len(pidAndNextAndWrite)):
        if pos == heatUp: # Heatup, ignore previous accesses
            pageMisses = 0
            dirtyEvicts = 0
        (pid, nextZugriff, write) = pidAndNextAndWrite[pos]
        
        if pid not in currentRam:
            # Load
            pageMisses += 1
            # Evict
            if len(currentRam) >= ramSize:
                evictMe = evictFinder(currentRam, dirtyInRam)

                del currentRam[evictMe]
                if evictMe in dirtyInRam:
                    dirtyEvicts += 1
                    dirtyInRam.remove(evictMe)
        
        currentRam[pid] = flagFunction(currentRam.get(pid, None), [pos, nextZugriff, write, pid in dirtyInRam])
        if write:
            dirtyInRam.add(pid)

    dirtyEvicts += len(dirtyInRam)
    return (pageMisses, dirtyEvicts)

def belady(pidAndNextAndWrite, ramSize, heatUp=0):
    return executeStrategy(pidAndNextAndWrite, ramSize, lambda _, x: x[1], evictMinPage, heatUp=heatUp)

def lru(pidAndNext, ramSize, heatUp=0):
    return executeStrategy(pidAndNext, ramSize, lambda _, x: x[0], evictMinPage, heatUp=heatUp)

def ran(pidAndNextAndWrite, ramSize, heatUp=0):
    return executeStrategy(pidAndNextAndWrite, ramSize, lambda _, x: 0, randomFindPageToEvict, heatUp=heatUp)

def lru2(pidAndNext, ramSize, heatUp=0): # Last_flag: (last_access, dirty)
    return executeStrategy(pidAndNext, ramSize,
        lambda last_flag, x: x[0],
        evict_lru_strange, heatUp=heatUp)

def lru3(pidAndNext, ramSize, heatUp=0): # Last_flag: (last_access, dirty)
    return executeStrategy(pidAndNext, ramSize,
        lambda last_flag, x:
            x[0] + 40 if x[2] or x[3] 
            else x[0],
        evictMinPage, heatUp=heatUp)

def lruStack(pidAndNextAndWrite: list[(int, int, int)], heatUp=0):
    stack = [] # The stack
    stackDist = {} # Fast access to counter of stack depth x
    stackDistDirty = {} # Stack for dirty Accesses (if depth > buffersize: dirty page was evicted)
    stackContains = {} # fast acces to check, if element was already loaded
    dirty_depth = {} # -1 => page is clean
    for pos in range(0, len(pidAndNextAndWrite)):
        if pos == heatUp: # Heatup, ignore previous accesses
            stackDist = {}
        (pid, next, write) = pidAndNextAndWrite[pos]

        if pid in stackContains: # Old Value
            depth = stack.index(pid)
            stackDist[depth] = stackDist[depth] + 1 if (depth in stackDist) else 1
            stack.remove(pid)

            prev_dirty_depth = dirty_depth[pid]
            dirty_depth[pid] = 0 if write else (-1 if(prev_dirty_depth == -1) else max(prev_dirty_depth, depth))
            if write: # refresh 
                if prev_dirty_depth != -1:
                    d_depth = max(prev_dirty_depth, depth)
                    # Mark in stack
                    stackDistDirty[d_depth] = stackDistDirty[d_depth] +1 if d_depth in stackDistDirty else 1

        else: # New value
            if -1 in stackDist:
                stackDist[-1] = stackDist[-1] +1
            else:
                stackDist[-1] = 1
            dirty_depth[pid] = 0 if write else -1
        stack.insert(0, pid)
        stackContains[pid]=0
    return (stackDist, stackDistDirty)

def evalStack(stack, ramSize):
    missrate = 0
    for pos in stack:
        value = stack[pos]
        if not (pos < ramSize and pos >= 0):
            missrate += value
    return missrate


def preprocess(zugriffe: list, reversed=True): # Watch out: nextAccess on reversed list => min gives latest access
    length = len(zugriffe)
    lastUse = {}
    pidAndAccess = []
    zugriffe.reverse()
    for pos, (pid, write) in enumerate(list(zugriffe)):
        if not reversed:
            pos = length - pos
        if pid in lastUse:
            pidAndAccess.append((pid, lastUse[pid], write))
            lastUse[pid] = pos
        else:
            pidAndAccess.append((pid, -1, write))
            lastUse[pid] = pos
    pidAndAccess.reverse()
    zugriffe.reverse()
    return pidAndAccess

def genEvalList(costfactor, elements):
    
    def evalLists(missList, writeList):
        costList = map(lambda read, write: read + write*costfactor, missList, writeList)
        norm_miss = map(lambda x: x/elements, missList)
        norm_costList = map(lambda x: x/elements, costList)
        return (norm_miss, norm_costList)
    return evalLists

def generateCSV(pidAndNextAndWrite, dirName, heatUp=0, all=False, quick=False):
    elements = len(pidAndNextAndWrite) - heatUp
    evalList = genEvalList(8, elements)

    pre = time.time()

    range0 = 3
    range1 = 20
    range2 = 100
    range3 = 1000
    range4 = 10000
    range5 = 100000
    range6 = 1000000
    range7 = 5000000
    xList = list(range(range0, range1, 1)) + list(range(range1, range2, 10)) + list(range(range2, range3, 100)) + list(range(range3, range4, 1000)) + list(range(range4, range5, 10000)) + list(range(range5, range6, 100000)) + list(range(range6, range7+1, 1000000))
    names = []
    yMissLists = []
    yCostLists = []

    post = time.time()
    print(post-pre)
    pre=post
       
    print("lruStack")
    (stackDist, dirtyStack) = lruStack(pidAndNextAndWrite, heatUp)
    
    post = time.time()
    print(post-pre)
    pre=post
    minValue = min([x for x in xList if x > max(stackDist)])
    xList = [x for x in xList if x<= minValue]

    print("Buffers to calculate: {}".format(len(xList)))

    print("Stacksize: " + str(max(stackDist)))
    lruMissList = list(map(lambda size: evalStack(stackDist, size), xList))
    lruDirtyList = list(map(lambda size: evalStack(dirtyStack, size), xList))
    
    written = sum(list(dirtyStack.values()))
    (_, _, write) = zip(*pidAndNextAndWrite)
    total_writes = sum(write)
    dirty_inRam = total_writes - written
    lruDirtyList = list(map(lambda hits: hits + dirty_inRam, lruDirtyList))
    
    (normMissList, normCostList) = evalList(lruMissList, lruDirtyList)

    names.append("lru")
    yMissLists.append(normMissList)
    yCostLists.append(normCostList)
    post = time.time()
    print(post-pre)
    pre=post

    if not quick:
        for (name, function) in [("rand", ran), ("opt", belady), ("strange lru2", lru2),  ("strange lru3", lru3)]:
            print(name)
            (missList, dirtyList) = list(zip(*Parallel(n_jobs=8)(delayed(function)(pidAndNextAndWrite, size, heatUp=heatUp) for size in xList)))
            (normMissList, normCostList) = evalList(missList, dirtyList)
            names.append(name)
            yMissLists.append(normMissList)
            yCostLists.append(normCostList)

            post = time.time()
            print(post-pre)
            pre=post

    def save_csv(xList, yMissLists, names, name):
        d = {"X": xList}
        for x in range(0, len(names)):
            d[names[x]] = yMissLists[x]
        df = pandas.DataFrame(data=d)
        df.to_csv(name+".csv", index=False)

    save_csv(xList, yMissLists, names, dirName + "miss")
    save_csv(xList, yCostLists, names, dirName + "cost")
    


def plotGraph(name, all=False):
    df = pandas.read_csv(name+"miss.csv")
    df_cost = pandas.read_csv(name+"cost.csv")

    def plotGraphInner(df, title, ylabel, file, miss=True):
        labels = list(df.head())
        labels.remove("X")
        for pos in range(0, len(labels)):
            misses = df[labels[pos]]
            label= labels[pos]
            if not miss:
                misses = list(map(lambda x: 1-x, misses))
            plt.plot(df["X"], misses, label=label)

        plt.xlabel("Buffer Size")
        plt.ylabel(ylabel)
        plt.ylim(0,1)
        plt.legend()
        plt.title(title)
        plt.savefig(file)
        plt.clf()

    def plotGraphCost(df, title, ylabel, file):
        labels = list(df.head())
        labels.remove("X")
        for pos in range(0, len(labels)):
            costs = df[labels[pos]]
            label= labels[pos]
            plt.plot(df["X"], costs, label=label)

        plt.xlabel("Buffer Size")
        plt.ylabel(ylabel)
        # plt.ylim(0,1)
        plt.legend()
        plt.title(title)
        plt.savefig(file)
        plt.clf()

    plotGraphInner(df, "Hitrate", "Hits", name + "_hits.pdf", miss=False)

    plotGraphInner(df, "Missrate", "Misses", name + "_misses.pdf")

    plotGraphCost(df_cost, "Cost", "Cost", name + "_cost.pdf")


def doOneRun(heatUp, all, data, name):
    elements= len(data)-heatUp
    dirpath = "./Elem_" + str(elements)+ "_heatUp_" + str(heatUp)
    if all:
        dirpath = "./ALL_heatUp_" + str(heatUp)
    
    try:
        os.mkdir(dirpath)
    except:
        pass

    print("Length data {}: {}".format(name, len(data)))
    try:
        print("try to load from cache")
        plotGraph(dirpath + name, all=all)
    except FileNotFoundError:
        print("Failed to load from cache")
        generateCSV(data, dirpath + name, heatUp=heatUp, all=all)
        plotGraph(dirpath + name, all=all)


def do_run():
    elementList = [10000, 100000, 1000000, -1]
    heatUpList = [0, 400, 1000, 4000, 10000, 40000]

    file = "./accesses_10000_3000_40_0.5_1.0.csv"

    loaded_data = pandas.read_csv(file)
    data = list(zip(loaded_data["pages"], loaded_data["is_write"]))
    
    if -1 in elementList:
        data_file = data
    else:
        elements = max(elementList) + max(heatUpList)
        data_file = data[:elements]

    preprocessed_data = preprocess(data_file)

    for (data, name) in [(preprocessed_data, "/Data")]:
        for elements in [-1]: # elementList:
            for heatUp in [0]: # heatUpList:
                print("-------")
                print("Elements: {}, heatup: {}".format(elements, heatUp))
                if elements == -1:
                    doOneRun(heatUp, True, data, name)
                else:
                    doOneRun(heatUp, False, data[:elements+heatUp], name)

do_run()
# Second Try: dont evict during locking (from resolve swip to unlock)
# Third try???

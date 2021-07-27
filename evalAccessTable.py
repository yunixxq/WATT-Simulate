#!/bin/python3

import matplotlib.pyplot as plt
import os, random, time
from joblib import Parallel, delayed
import pandas


def getWorker(zugriff):
    return int(zugriff.split(" ")[0])

def getCommand(zugriff):
    return zugriff.split(":")[0].split(" ", 1)[1]

def getPid(zugriff): # Remove /n at end
    return int(zugriff.split(": ")[1])

def evictMinPage(ram):
    return min(ram, key=ram.get)

def randomFindPageToEvict(ram: dict):
    return random.choice(list(ram))

# First Try:
# Count every SWIP to pageId as valid access

def getOnlySwip(zugriffe):
    return list(filter(lambda x: "Resolving Swip" in x, zugriffe))

def getOnlyHotSwip(zugriffe):
    return list(filter(lambda x: "Hot" in x, zugriffe))
def getOnlyColdSwip(zugriffe):
    return list(filter(lambda x: "cold" in x, zugriffe))
def getOnlyCoolSwip(zugriffe):
    return list(filter(lambda x: "cool" in x, zugriffe))

def executeStrategy(pidAndNextAndWrite, ramSize, flagFunction, evictFinder, heatUp= 0):
    currentRam = {}
    pageMisses = 0
    for pos in range(0, len(pidAndNextAndWrite)):
        if pos == heatUp: # Heatup, ignore previous accesses
            pageMisses = 0
            
        (pid, nextZugriff, write) = pidAndNextAndWrite[pos]
        flag = flagFunction([pos, nextZugriff])
        if pid in currentRam:
            currentRam[pid] = flag
        else:
            if len(currentRam) < ramSize:
                currentRam[pid] = flag
            else:
                evictMe = evictFinder(currentRam)

                del currentRam[evictMe]

                assert(len(currentRam) < ramSize)
                currentRam[pid] = flag
            pageMisses +=1
    return pageMisses

def belady(pidAndNextAndWrite, ramSize, heatUp=0, opt=False, minMiss=0, maxHit=0):
    global lastHit, lastMiss
    if not opt:
        return executeStrategy(pidAndNextAndWrite, ramSize, lambda x: x[1], evictMinPage, heatUp=heatUp)
    if (maxHit == lastHit):
        assert(minMiss == lastMiss)
        return (minMiss, maxHit)
    (lastMiss, lastHit) = executeStrategy(pidAndNext, ramSize, lambda x: x[1], evictMinPage, heatUp=heatUp)
    return (lastMiss, lastHit)

def lru(pidAndNext, ramSize, heatUp=0):
    return executeStrategy(pidAndNext, ramSize, lambda x: x[0], evictMinPage, heatUp=heatUp)

def ran(pidAndNextAndWrite, ramSize, heatUp=0):
    return executeStrategy(pidAndNextAndWrite, ramSize, lambda x: 0, randomFindPageToEvict, heatUp=heatUp)

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


def generateCSV(pidAndNextAndWrite, name, heatUp=0, all=False, quick=False):
    global lastHit, lastMiss
    elements = len(pidAndNextAndWrite) - heatUp

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
    
    minMiss = lruMissList[-1]

    def costFunction(read, write):
        return read + write * 8

    lruCostList = list(map(lambda read, write: read + write * 8, lruMissList, lruDirtyList))
    print(lruCostList)
    lruMissList = list(map(lambda x: float(x)/elements, lruMissList))

    names.append("lru")
    yMissLists.append(lruMissList)

    post = time.time()
    print(post-pre)
    pre=post

    if not quick:
        print("rand")
        ranMissList = Parallel(n_jobs=8)(delayed(ran)(pidAndNextAndWrite, size, heatUp=heatUp) for size in xList)
        #results = map(lambda size: ran(pidAndNext, size, heatUp=heatUp), xList))

        ranMissList = list(map(lambda x: float(x)/elements, ranMissList))
        names.append("ran")
        yMissLists.append(ranMissList)
        
        post = time.time()
        print(post-pre)
        pre=post

        print("opt")
        lastMiss = 0
        # results = (map(lambda size: belady(pidAndNext, size, heatUp=heatUp, opt=True, minMiss=minMiss, maxHit=maxHit), xList))
        optMissList = Parallel(n_jobs=8)(delayed(belady)(pidAndNextAndWrite, size, heatUp=heatUp) for size in xList)

        optMissList = list(map(lambda x: float(x)/elements, optMissList))
        names.append("opt")
        yMissLists.append(optMissList)

        post = time.time()
        print(post-pre)
        pre=post

    def save_csv(xList, yMissLists, names, name):
        d = {"X": xList}
        for x in range(0, len(names)):
            d[names[x]] = yMissLists[x]
        df = pandas.DataFrame(data=d)
        df.to_csv(name+".csv", index=False)

    save_csv(xList, yMissLists, names, name)


def plotGraph(name, all=False):
    df = pandas.read_csv(name+".csv")
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

    plotGraphInner(df, "Hitrate", "Hits", name + "_hits.pdf", miss=False)

    plotGraphInner(df, "Missrate", "Misses", name + "_misses.pdf")

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

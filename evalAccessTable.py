#!/bin/python3

import matplotlib.pyplot as plt
import os, random, time
from joblib import Parallel, delayed
import pandas
from abc import ABC, abstractmethod

class EvictStrategy(ABC):
    ram: dict = {}
    dirtyInRam: set = set()

    def __init__(self, write_cost=1):
        self.write_cost = write_cost
    
    def reset(self):
        self.ram = {}
        self.dirtyInRam = set()

    def minFromRam(self) -> int:
        return min(self.ram, key=self.ram.get)
    
    def handleRemove(self, pid: int) -> bool:
        del self.ram[pid]
        if pid in self.dirtyInRam:
            self.dirtyInRam.remove(pid)
            return True
        return False
    
    def handleDirty(self, pid: int, write: bool):
        if write:
            self.dirtyInRam.add(pid)

    def inRam(self, pid: int):
        return pid in self.ram
    
    def ramsize(self):
        return len(self.ram)
    
    def dirtyPages(self):
        return len(self.dirtyInRam)

    @abstractmethod
    def access(self, pid: int, pos: int, nextZugriff: int, write: bool) -> None:
        pass

    @abstractmethod
    def evictOne(self, curr_time: int) -> bool: # Returns True if it was write
        pass

class Lru(EvictStrategy):
    def access(self, pid: int, pos: int, nextZugriff: int, write: bool) -> None:
        self.ram[pid] = pos
        self.handleDirty(pid, write)

    def evictOne(self, curr_time: int) -> bool:
        pid = self.minFromRam()
        return self.handleRemove(pid)

class Ran(EvictStrategy):
    def access(self, pid: int, pos: int, nextZugriff: int, write: bool) -> None:
        self.ram[pid] = 1
        self.handleDirty(pid, write)
    
    def evictOne(self, curr_time: int) -> bool:
        pid = random.choice(list(self.ram))
        return self.handleRemove(pid)


class Belady(EvictStrategy):
    def access(self, pid: int, pos: int, nextZugriff: int, write: bool) -> None:
        self.ram[pid] = nextZugriff
        self.handleDirty(pid, write)

    def evictOne(self, curr_time: int) -> bool:
        pid = self.minFromRam()
        return self.handleRemove(pid)


class Cf_lru(EvictStrategy):
    def __init__(self, clean_percentage):
        super().__init__()
        self.clean_percentage = clean_percentage

    def access(self, pid: int, pos: int, nextZugriff: int, write: bool) -> None:
        self.ram[pid] = pos
        self.handleDirty(pid, write)

    def evictOne(self, curr_time: int) -> bool:
        pid = 0
        window_length = int(len(self.ram)*self.clean_percentage)
        window = list(sorted(self.ram, key=lambda x: self.ram[x]))[:window_length]
        window_clean = [x for x in window if x not in self.dirtyInRam]
        if len(window_clean) > 0:
            pid = window_clean[0]
        else:
            pid = window[0]
        return self.handleRemove(pid)

class Lru_wsr(EvictStrategy):
    def access(self, pid: int, pos: int, nextZugriff: int, write: bool) -> None:
        self.ram[pid] = (pos, False)
        self.handleDirty(pid, write)

    def evictOne(self, curr_time: int) -> bool:
        pid = 0
        while True:
            evict_cand = min(self.ram, key=lambda x: self.ram[x][0])
            if (evict_cand not in self.dirtyInRam) or self.ram[evict_cand][1]:
                return self.handleRemove(evict_cand)
            else:
                self.ram[evict_cand] = (self.ram[max(self.ram, key=lambda x: self.ram[x][0])][0] + 1.0 / len(self.ram), True)

class Lru_strange_1(EvictStrategy):
    def access(self, pid: int, pos: int, nextZugriff: int, write: bool) -> None:
        self.ram[pid] = pos
        self.handleDirty(pid, write)

    def evictOne(self, curr_time: int):
        pid = min(self.ram, key=lambda x: self.ram[x] +40 if x in self.dirtyInRam else self.ram[x])
        return self.handleRemove(pid)

class Lru_strange_2(EvictStrategy):
    def access(self, pid: int, pos: int, nextZugriff: int, write: bool) -> None:
        if write or pid in self.dirtyInRam:
            self.ram[pid] = pos + 40 
        else:
            self.ram[pid] = pos

    def evictOne(self, curr_time: int):
        pid = self.minFromRam()
        return self.handleRemove(pid)

class K_Entries(EvictStrategy):
    def __init__(self, k):
        self.k = k

    def access(self, pid: int, pos: int, nextZugriff: int, write: bool) -> None:
        if pid in self.ram:
            if len(self.ram[pid]) > self.k-1:
                self.ram[pid] = [pos] + self.ram[pid][:(self.k-1)]
            else:
                self.ram[pid] = [pos] + self.ram[pid]
        else:
            self.ram[pid] = [pos]
        assert(len(self.ram[pid]) <= self.k)
        self.handleDirty(pid, write)


class lfu_k_first(K_Entries): # gets current_frequencies
    def eval_ram_entry(self, pid: int, curr_time: int):
        return max(map(lambda time, pos : (pos+1)/(curr_time - time), 
            self.ram[pid], range(len(self.ram[pid]))))

    def evictOne(self, curr_time: int) -> bool:
        pid = min(self.ram, key=lambda pid: self.eval_ram_entry(pid, curr_time))
        return self.handleRemove(pid)

class lfu_k_ignore_first(K_Entries): # gets current_frequencies
    def eval_ram_entry(self, pid: int, curr_time: int):
        return max(map(lambda time, pos : pos/(curr_time - time), 
            self.ram[pid], range(len(self.ram[pid]))))

    def evictOne(self, curr_time: int) -> bool:
        pid = min(self.ram, key=lambda pid: self.eval_ram_entry(pid, curr_time))
        return self.handleRemove(pid)


def randomFindPageToEvict(ram: dict, dirtyInRam: set = {}):
    return random.choice(list(ram))

class lru_k(K_Entries):
    def eval_ram_entry(self, pid: int, curr_time: int):
        if(len(self.ram[pid]) == self.k):
            return (self.ram[pid][-1])
        else:
            return -1

    def evictOne(self, curr_time: int) -> bool:
        pid = min(self.ram, key=lambda pid: self.eval_ram_entry(pid, curr_time))
        return self.handleRemove(pid)

class lru_k_mod(K_Entries):
    def eval_ram_entry(self, pid: int, curr_time: int):
        if(len(self.ram[pid]) == self.k):
            return (curr_time - self.ram[pid][-1])
        else:
            return (curr_time - self.ram[pid][-1])*self.k/len(self.ram[pid])

    def evictOne(self, curr_time: int) -> bool:
        pid = max(self.ram, key=lambda pid: self.eval_ram_entry(pid, curr_time))
        return self.handleRemove(pid)


# FlagFunction: [pos, nextZugriff, write, dirty]
def executeStrategy(pidAndNextAndWrite, ramSize, strategy:EvictStrategy, heatUp= 0):
    pageMisses = 0
    dirtyEvicts = 0
    strategy.reset()
    for pos in range(0, len(pidAndNextAndWrite)):
        if pos == heatUp: # Heatup, ignore previous accesses
            pageMisses = 0
            dirtyEvicts = 0
        (pid, nextZugriff, write) = pidAndNextAndWrite[pos]
        if not strategy.inRam(pid):
            # Load
            pageMisses += 1
            # Evict
            if strategy.ramsize() >= ramSize:
                wasDirty = strategy.evictOne(pos)
                if wasDirty:
                    dirtyEvicts += 1
        strategy.access(pid, pos, nextZugriff, write)

    dirtyEvicts += strategy.dirtyPages()
    return (pageMisses, dirtyEvicts)

def staticOpt(pidAndNextAndWrite, ramSize, heatUp=0, write_cost=1):
    pidList = [x[0] for x in pidAndNextAndWrite]
    writeList = [x[0] for x in pidAndNextAndWrite if x[2]]
    accesses = {}
    writes = {}
    cost = {}
    for pid in set(pidList):
        accesses[pid] = pidList.count(pid)
        writes[pid] = writeList.count(pid)
        cost[pid] = accesses[pid] + writes[pid]*write_cost
    pidByCost = sorted(cost, key=lambda x: -cost.get(x))
    if ramSize == 10:
        print("Static page ranking")
        print(pidByCost)
    pageMisses, dirtyEvicts = (0,0)
    if len(set(pidList)) > ramSize:
        pageMisses = sum(map(lambda x: accesses[x], pidByCost[ramSize-1:])) + ramSize-1
        dirtyEvicts = sum(map(lambda x: writes[x], pidByCost[ramSize-1:])) + sum([1 for x in pidByCost[:ramSize-1]])
    else:
        pageMisses = len(set(pidList))
        dirtyEvicts = len(set(writeList))
    return (pageMisses, dirtyEvicts)

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
        norm_miss = list(map(lambda x: x/elements, missList))
        norm_hit = list(map(lambda x: 1-x, norm_miss))
        norm_costList = list(map(lambda x: x/elements, costList))
        return (norm_hit, norm_miss, norm_costList)
    return evalLists

def generateCSV(pidAndNextAndWrite, dirName, heatUp=0, write_cost=1):
    elements = len(pidAndNextAndWrite) - heatUp
    quick=False
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
    yReadList = []
    yWriteList = []

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

    names.append("elements")
    yReadList.append([elements for x in xList])
    yWriteList.append([elements for x in xList])

    print("Buffers to calculate: {}".format(len(xList)))

    print("Stacksize: " + str(max(stackDist)))
    lruMissList = list(map(lambda size: evalStack(stackDist, size), xList))
    lruDirtyList = list(map(lambda size: evalStack(dirtyStack, size), xList))
    
    written = sum(list(dirtyStack.values()))
    (_, _, write) = zip(*pidAndNextAndWrite)
    total_writes = sum(write)
    dirty_inRam = total_writes - written
    lruDirtyList = list(map(lambda hits: hits + dirty_inRam, lruDirtyList))

    def append(name, missList, dirtyList):
        names.append(name)
        yReadList.append(missList)
        yWriteList.append(dirtyList)

        post = time.time()
        print(name)
        print(post-pre)
        return post

    pre = append("lru", lruMissList, lruDirtyList)

    if not quick:
        # Standardized
        for (name, strategy) in [("rand", Ran()), ("opt", Belady()),
                #("cf_lru", Cf_lru(0.5)), ("lru_wsr", Lru_wsr()), # ("strange lru", Lru_strange_1()),
                #("lfu1_2", lfu_k_first(2)), ("lfu1_5", lfu_k_first(5)), ("lfu1_20", lfu_k_first(20)), ("lfu1_100", lfu_k_first(100)),
                ("lfu_2", lfu_k_ignore_first(2)), ("lfu_5", lfu_k_ignore_first(5)), ("lfu_20", lfu_k_ignore_first(20)), ("lfu_100", lfu_k_ignore_first(100)),
                ("lru_2", lru_k(2)), #("lru_3", lru_k(3))#, ("lru_5", lru_k(5)), ("lru_20", lru_k(20))
               # ,("lru_mod_2", lru_k_mod(2))
                ]:
            (missList, dirtyList) = list(zip(*Parallel(n_jobs=8)(delayed(executeStrategy)(pidAndNextAndWrite, size, strategy, heatUp=heatUp) for size in xList)))
            pre = append(name, missList, dirtyList)
        # Others
        for (name, function) in [("staticOpt", staticOpt)]:
            (missList, dirtyList) = list(zip(*Parallel(n_jobs=8)(delayed(function)(pidAndNextAndWrite, size, heatUp=heatUp, write_cost=write_cost) for size in xList)))
            pre = append(name, missList, dirtyList)

    def save_csv(xList, yMissLists, names, name):
        d = {"X": xList}
        for x in range(0, len(names)):
            d[names[x]] = yMissLists[x]
        df = pandas.DataFrame(data=d)
        df.to_csv(name+".csv", index=False)

    save_csv(xList, yReadList, names, dirName + "reads")
    save_csv(xList, yWriteList, names, dirName + "writes")
    


def plotGraph(name, write_cost = 8):
    df_read = pandas.read_csv(name+"reads.csv")
    df_write = pandas.read_csv(name+"writes.csv")
    elements = df_read["elements"][0]
    evalList = genEvalList(write_cost, elements)

    for elem in list(df_read["elements"]) + list(df_write["elements"]):
        assert(elem == elements)

    df_hit = pandas.DataFrame()
    df_miss = pandas.DataFrame()
    df_cost = pandas.DataFrame()
    df_writes = pandas.DataFrame()
    df_hit["X"] = df_read["X"]
    df_miss["X"] = df_read["X"]
    df_cost["X"] = df_read["X"]
    df_writes["X"] = df_read["X"]

    labels = list(df_read.columns.values)
    labels.remove("X")
    labels.remove("elements")

    for column in labels:
        (df_hit[column], df_miss[column], df_cost[column]) = evalList(df_read[column], df_write[column])
        df_writes[column] = df_write[column]

    def plotGraphInner(df, title, ylabel, file, labels, limit=False):
        for label in labels:
            plt.plot(df["X"], df[label], label=label, linewidth=0.5)

        plt.xlabel("Buffer Size")
        plt.ylabel(ylabel)
        if(limit):
            plt.ylim(0,1)
        plt.legend()
        plt.title(title)
        plt.savefig(file)
        plt.clf()

    plotGraphInner(df_hit, "Hitrate", "Hits", name + "_hits.pdf", labels, limit=True)

    plotGraphInner(df_miss, "Missrate", "Misses", name + "_misses.pdf", labels, limit=True)

    plotGraphInner(df_cost, "Cost", "Cost", name + "_cost.pdf", labels)

    plotGraphInner(df_writes, "Writes", "Writes", name + "_writes.pdf", labels)


def doOneRun(heatUp, all, data, name, write_cost = 8):
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
        plotGraph(dirpath + name, write_cost)
    except FileNotFoundError:
        print("Failed to load from cache")
        generateCSV(data, dirpath + name, heatUp=heatUp)
        plotGraph(dirpath + name, write_cost)

def get_data_file(file):
    loaded_data = pandas.read_csv(file)
    return list(zip(loaded_data["pages"], loaded_data["is_write"]))

def do_run(file, elementList = [-1], heatUpList = [0]):
    data = get_data_file(file)
    
    if -1 in elementList:
        data_file = data
    else:
        elements = max(elementList) + max(heatUpList)
        data_file = data[:elements]

    preprocessed_data = preprocess(data_file)

    for (data, name) in [(preprocessed_data, "/Data")]:
        for elements in elementList:
            for heatUp in heatUpList:
                print("-------")
                print("Elements: {}, heatup: {}".format(elements, heatUp))
                if elements == -1:
                    doOneRun(heatUp, True, data, name)
                else:
                    doOneRun(heatUp, False, data[:elements+heatUp], name)

def oneFullRun(file, csv_start, write_cost):
    data = preprocess(get_data_file(file))
    generateCSV(data, csv_start, write_cost=write_cost)


if __name__ == "__main__":
    file = "./accesses_10000_3000_40_0.5_1.0.csv"
    elementList = [-1] #[10000, 100000, 1000000, -1]
    heatUpList = [0] #[0, 400, 1000, 4000, 10000, 40000]

    do_run(file, elementList, heatUpList)

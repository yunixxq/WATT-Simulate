#!/bin/python3

import matplotlib.pyplot as plt
import os, random, time
from joblib import Parallel, delayed
from numpy import double
import pandas, itertools
from abc import ABC, abstractmethod
from collections.abc import Callable
from typing import List, Tuple
from make_graphs import plotGraph

class Executor(ABC):

    @abstractmethod
    def getExecutor() -> Callable[[List[Tuple[int, int, bool]], List[int], int, int],List[Tuple[int, int]]]:
        pass

class EvictStrategy(ABC):
    ram: dict = {}
    dirtyInRam: set = set()

    def __init__(self, write_cost=1):
        self.write_cost = write_cost
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
    def __init__(self, k=10):
        self.k = k
        super().__init__()

    def access(self, pid: int, pos: int, nextZugriff: int, write: bool) -> None:
        value = (pos, write)
        if pid in self.ram:
            if len(self.ram[pid]) > self.k-1:
                self.ram[pid] = [value] + self.ram[pid][:(self.k-1)]
                assert(len(self.ram[pid]) == self.k)
            else:
                self.ram[pid] = [value] + self.ram[pid]
                assert(len(self.ram[pid]) <= self.k)
        else:
            self.ram[pid] = [value]
        self.handleDirty(pid, write)

    def get_values(self, pid: int) -> List[int]:
        return list(map(lambda x: x[0], self.ram[pid]))

    def get_accesses(self, pid: int, curr_time) -> List[int]:
        return self.get_values(pid)

    def get_writes(self, pid: int) -> List[bool]:
        return list(map(lambda x: x[1], self.ram[pid]))
    
class K_Entries_History(K_Entries):
    history: dict = {}
    backlog = []
    def __init__(self, k=10, history_size = 10, backlog_length = -5):
        self.history = {}
        self.history_size = history_size
        self.backlog_length = backlog_length
        super().__init__(k)

    def access(self, pid: int, pos: int, nextZugriff: int, write: bool) -> None:
        if pid in self.backlog:
            self.backlog.remove(pid)
            self.ram[pid] = self.history[pid]
            self.history[pid] = []
        super().access(pid, pos, nextZugriff, write)
    
    def handleRemove(self, pid: int) -> bool:
        self.history[pid] = self.ram[pid]
        if self.history_size > 0:
            self.history[pid] = self.history[pid][:self.history_size]
        backlog_length = self.backlog_length
        if backlog_length < 0:
            backlog_length = int(self.ramsize()/-backlog_length)
        elif backlog_length > 0:
            backlog_length = int(self.ramsize()*backlog_length)
        else:
            backlog_length = 1_000_000
        self.backlog = [pid] + self.backlog
        while len(self.backlog) > backlog_length:
            drop_pid = self.backlog[-1]
            self.backlog = self.backlog[:-1]
            self.history[drop_pid] = []
        return super().handleRemove(pid)

class K_Entries_Write(K_Entries):
    def __init__(self, k, write_factor):
        self.write_factor = write_factor
        super().__init__(k)
    
    def makeYounger(self, pos, pid, curr_time) -> int: # Make page younger by a factor, if write
        value = self.get_values(pid)[pos]
        if not self.get_writes(pid)[pos]:
            return value
        else:
            return int(curr_time - (curr_time - value)*self.write_factor)

    def get_accesses(self, pid: int, curr_time) -> List[int]:
        return list(map(lambda x: self.makeYounger(x, pid, curr_time), range(len(self.ram[pid]))))



def get_lfu_k(cls, *args):
    assert(issubclass(cls, K_Entries))
    class lfu_k(cls): # gets current_frequencies
        def __init__(self, *args):
            super().__init__(*args)

        def get_frequency(self, pid: int, curr_time: int):
            return max(map(lambda time, pos : pos/(curr_time - time),
                self.get_accesses(pid, curr_time), list(range(len(self.ram[pid])))), key=lambda x: x)
            
        def evictOne(self, curr_time: int) -> bool:
            pid = min(self.ram, key=lambda pid: self.get_frequency(pid, curr_time)) 
            if len(self.ram[pid]) == 1:
                zeros = list(filter(lambda pid: len(self.ram[pid])== 1, self.ram))
                if len(zeros) > 1:
                    pid = min(zeros, key=lambda pid: self.get_accesses(pid, curr_time)[0])
            return self.handleRemove(pid)

    return lfu_k(*args)

def get_lru_k(cls, *args):
    assert(issubclass(cls, K_Entries))
    class lru_k(cls):
        def __init__(self, *args):
            super().__init__(*args)

        def get_age(self, pid: int, curr_time: int, pseudo_k = -1):
            if pseudo_k == -1:
                pseudo_k = self.k
            assert(len(self.ram[pid]) <= self.k)
            if(len(self.ram[pid]) == pseudo_k):
                return curr_time - self.get_accesses(pid, curr_time)[pseudo_k-1]
            else:
                return -1

        def evictOne(self, curr_time: int) -> bool:
            min_k = min(map(lambda pid: len(self.ram[pid]), self.ram))
            pid = max(self.ram, key=lambda pid: self.get_age(pid, curr_time, min_k))
            return self.handleRemove(pid)

    return lru_k(*args)

def randomFindPageToEvict(ram: dict, dirtyInRam: set = {}):
    return random.choice(list(ram))

# FlagFunction: [pos, nextZugriff, write, dirty]
def executeStrategy(pidAndNextAndWrite: list((int, int, bool)), ramSize, strategy:EvictStrategy, heatUp= 0, write_cost=0):
    pageMisses = 0
    dirtyEvicts = 0
    for pos in range(0, len(pidAndNextAndWrite)):
        if pos == heatUp: # Heatup, ignore previous accesses
            pageMisses = 0
            dirtyEvicts = 0
        pid: int
        nextZugriff: int
        write: bool
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

class StaticOpt(Executor):
    def staticOpt(pidAndNextAndWrite, ramSizes, heatUp=0, write_cost=1) -> List[Tuple[int, int]]:
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
        print("Static page ranking")
        print(pidByCost)
        returner=[]
        for ramSize in ramSizes:
            pageMisses, dirtyEvicts = (0,0)
            if len(set(pidList)) > ramSize:
                pageMisses = sum(map(lambda x: accesses[x], pidByCost[ramSize-1:])) + ramSize-1
                dirtyEvicts = sum(map(lambda x: writes[x], pidByCost[ramSize-1:])) + sum([1 for x in pidByCost[:ramSize-1] if writes[x] > 0])
            else:
                pageMisses = len(set(pidList))
                dirtyEvicts = len(set(writeList))
            returner.append((pageMisses, dirtyEvicts))
        return returner

    def getExecutor():
        return StaticOpt.staticOpt

def lruStack(pidAndNextAndWrite: List[Tuple[int, int, int]], heatUp=0):
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

def generateCSV(pidAndNextAndWrite, dirName, heatUp=0, write_cost=1):
    
    pre = time.time()

    def printTimestamp():
        nonlocal pre
        post = time.time()
        print(post-pre)
        pre = post

    def initLists(elements, readFile, writeFile):
        yReadList = {}
        yWriteList = {}
        xList = []
        try:
            df_read = pandas.read_csv(readFile)
            df_write = pandas.read_csv(writeFile)
            if elements == df_read["elements"][0]:
                for label in list(df_read.columns.values):
                    yReadList[label] = list(df_read[label])
                    yWriteList[label] = list(df_write[label])
                xList = list(df_read["X"])
        except FileNotFoundError:
            pass
        return (yReadList, yWriteList, xList)

    def store(sorted=False, sort_value=[]):
        nonlocal yReadList, yWriteList, readFile, writeFile
        dRead = pandas.DataFrame(data=yReadList)
        dWrite = pandas.DataFrame(data=yWriteList)
        if sorted:
            if len(sort_value) == 0:
                dRead.sort_index(axis=1, inplace=True)
                dWrite.sort_index(axis=1, inplace=True)
            else:
                dRead.sort_values(by=sort_value ,axis=1, inplace=True)
                dWrite.sort_values(by=sort_value ,axis=1, inplace=True)
        dRead.set_index("X", inplace=True)
        dWrite.set_index("X", inplace=True)

        dRead.to_csv(readFile)
        dWrite.to_csv(writeFile)


    def append(name, missList, dirtyList, storeThis=True):
        nonlocal elements
        yReadList[name] = missList
        yWriteList[name] = dirtyList

        if(storeThis):
            print(name)
            print(sum(missList + dirtyList)/elements)
            printTimestamp()
            print("****")
            store()


    readFile = dirName + "reads.csv"
    writeFile = dirName + "writes.csv"
    quick=False

    elements = len(pidAndNextAndWrite) - heatUp
    (yReadList, yWriteList, xList) = initLists(elements, readFile, writeFile)

    if "lru" not in yReadList:
        range0 = 3
        range1 = 20
        range2 = 100
        range3 = 1000
        range4 = 10000
        range5 = 100000
        range6 = 1000000
        range7 = 5000000
        xList = list(range(range0, range1, 1)) + list(range(range1, range2, 10)) + list(range(range2, range3, 100)) + list(range(range3, range4, 1000)) + list(range(range4, range5, 10000)) + list(range(range5, range6, 100000)) + list(range(range6, range7+1, 1000000))

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

        append("elements", [elements for _ in xList], [elements for _ in xList], storeThis=False)
        append("X", xList, xList, storeThis=False)

        print("Buffers to calculate: {}".format(len(xList)))

        print("Stacksize: " + str(max(stackDist)))
        lruMissList = list(map(lambda size: evalStack(stackDist, size), xList))
        lruDirtyList = list(map(lambda size: evalStack(dirtyStack, size), xList))
        
        written = sum(list(dirtyStack.values()))
        (_, _, write) = zip(*pidAndNextAndWrite)
        total_writes = sum(write)
        dirty_inRam = total_writes - written
        lruDirtyList = list(map(lambda hits: hits + dirty_inRam, lruDirtyList))
        append("lru", lruMissList, lruDirtyList)

    if not quick:
        # Standardized
        strats_h = []
        # Add k_entries_history
        for (k, a, b) in itertools.product([1, 3, 5, 10, 20, 50], [1, 3, 5, 10, 20, 50], [-2, -1, 2, 0]):
            if a <= k:
                name = "lfu" + str(k) + "_h" + str(a) + "_"
                if b == 0:
                    name += "inf"
                else:
                    name += str(b)

                strats_h.append((name, lambda: get_lfu_k(K_Entries_History, int(k), int(a), int(b))))
        
        strats_w = []
        for (k, a) in itertools.product([1, 3, 5, 10, 20, 50], ["03", "01", "05", "1", "2", "7"]):
            name = "lfu" + str(k) + "_w" + a
            value = double("0."+a)

            strats_w.append((name, lambda: get_lfu_k(K_Entries_Write, int(k), value)))



        for (name, strategy) in [
                ("rand", Ran),
                #("lru_alt", Lru),
                # ("cf_lru", lambda: Cf_lru(0.5)),
                ("lru_wsr", Lru_wsr), # ("strange lru", Lru_strange_1),
                #("lru2", lambda: get_lru_k(K_Entries, 2)),
                #("lru4", lambda: get_lru_k(K_Entries, 4)),
                #("lru10", lambda: get_lru_k(K_Entries, 10)),
                #("lru20", lambda: get_lru_k(K_Entries, 20)),
                #("lfu20", lambda: get_lfu_k(K_Entries, 20)),
                #("lfu5_w03", lambda: get_lfu_k(K_Entries_Write, 5, 0.03)),
                #("lfu5_w05", lambda: get_lfu_k(K_Entries_Write, 5, 0.05)),
                #("lfu5_w01", lambda: get_lfu_k(K_Entries_Write, 5, 0.01)),
                #("lfu5_w2", lambda: get_lfu_k(K_Entries_Write, 5, 0.2)),
                #("lfu5_w5", lambda: get_lfu_k(K_Entries_Write, 5, 0.5)),
                #("lfu5_w7", lambda: get_lfu_k(K_Entries_Write, 5, 0.7)),
                # ("zipf_best_read", lambda: get_lfu_k(K_Entries, 10)),
                ]: # + strats_h + strats_w:
            if name in yReadList:
                continue
            print(name)
            (missList, dirtyList) = list(zip(*Parallel(n_jobs=8)(delayed(executeStrategy)(pidAndNextAndWrite, size, strategy(), heatUp=heatUp, write_cost=write_cost) for size in xList)))
            append(name, missList, dirtyList)
        # Others
        name: str
        function: Executor
        for (name, function) in [
                #("staticOpt", StaticOpt)
                ]:
            if name in yReadList:
                continue
            executor = function.getExecutor()
            (missList, dirtyList) = list(zip(*(executor(pidAndNextAndWrite, xList, heatUp=heatUp, write_cost=write_cost))))
            append(name, missList, dirtyList)

    store(sorted=True)

    



def doOneRun(heatUp, all, data, name, write_cost = 1):
    elements= len(data)-heatUp
    dirpath = "./Elem_" + str(elements)+ "_heatUp_" + str(heatUp)
    if all:
        dirpath = "./ALL_heatUp_" + str(heatUp)
    
    try:
        os.mkdir(dirpath)
    except:
        pass

    print("Length data {}: {}".format(name, len(data)))
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
    # file = "./zipf_accesses.csv"
    file = "./tpcc_64_-5.csv"
    elementList = [-1] #[10000, 100000, 1000000, -1]
    heatUpList = [0] #[0, 400, 1000, 4000, 10000, 40000]

    do_run(file, elementList, heatUpList)

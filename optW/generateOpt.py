#!/bin/python3

from os import write
import gurobipy as gp
from gurobipy import GRB, tupledict, tuplelist
from numpy import number
from scipy.stats import zipfian
import pandas, random


def initModel():
    print("Init model")
    model = gp.Model("writeOpt")
    #model.setParam("NodefileStart", 0.5)
    #model.setParam("Threads", 2)
    model.setParam("Method", 1)
    return model

def getAccessLists(accesses, is_write):
    access_len = len(accesses)
    pages = set(accesses)
    if len(accesses) != len(is_write):
        print(len(accesses))
        print(len(is_write))
        assert(len(accesses) == len(is_write))

    print("Building AccessLists {} {}".format(len(pages), access_len))
    def getValidTimesForPage(fancyList: list, page: number, writes=False):
        my_accesses = [x for x in fancyList if x[1] == page and ((not writes) or x[2] == True)]
        if len(my_accesses) != 0:
            return (my_accesses[0][0], my_accesses[-1][0])
        else:
            return (-1,-1)

    fancyList = list(zip(range(access_len), accesses, is_write))

    first_read = {}
    last_read = {}
    for p in pages:
        (start, end) = getValidTimesForPage(fancyList, p)
        assert(start != -1)
        assert(end != -1)
        first_read[p] = start
        last_read[p] = end
    first_write = {}
    last_write = {}
    for p in pages:
        (start, end) = getValidTimesForPage(fancyList, p, True)
        if(start == -1):
            continue
        first_write[p] = start
        last_write[p] = end
    return (first_read, last_read, first_write, last_write)

def createTimes(first, last):
    return gp.tuplelist([(p,t) for p in last for t in range(first[p], last[p] + 2)])

def addVariables(model, readTimes, writeTimes):
    print("Generating Variables")
    ram: tupledict = model.addVars(readTimes, name="ram", vtype=GRB.BINARY)
    delta_ram: tupledict = model.addVars(readTimes, name="delta_ram", vtype=GRB.BINARY)
    dirty: tupledict = model.addVars(writeTimes, name="dirty", vtype=GRB.BINARY)
    delta_dirty: tupledict = model.addVars(writeTimes, name="delta_dirty", vtype=GRB.BINARY)

    return (ram, delta_ram, dirty, delta_dirty)

def addConstraints(model, writeTimes, ram, delta_ram, dirty, delta_dirty, first_read, last_read, first_write, last_write, accesses, is_write, ramsize):

    ram_size = model.addVar(ramsize, ramsize, vtype=GRB.INTEGER, name="RAMSIZE")

    ## Weitere Optimierungen:
    ## - Alle deltas bis auf die direkt am zugriff sind = 0 (read direkt davor, write direkt dort)
    print("Adding Constraints")
        #  $\sum_t p_{s,t} \leq P$ = Puffergröße
    model.addConstrs((ram.sum('*', time) <= ram_size for time in range(-1, len(accesses) +1)), "capacity")

    # $\sum_t \delta p_{s,t} \leq 1$ = nur 1 read per step
    ## Ist eigentlich auch egal.. dann soll es halt mehrere laden
    # macht es einfacher für den solver, wenn er mehrere laden kann
    # model.addConstrs((delta_ram.sum('*', time) <= 1 for time in range(-1, access_len +1)), "maxRead")

    # $d_{s,t} \leq p_{s,t}$ = Eine dirty Page ist im Puffer
    model.addConstrs((dirty[pageTime] <= ram[pageTime] for pageTime in writeTimes), "dirtyInRam")

    # $p_{s,t} \leq p_{s,t-1} + \delta p_{s,t}$ = Seite kann nur durch lesen eingelagert werden. (Ausnahme 1. Seite (Sonderregel))
    model.addConstrs((ram[p, t] <= ram[p, t-1] + delta_ram[p, t] for p in first_read for t in range(first_read[p]+1, last_read[p] + 1)), "readFresh");

    #  - $\delta p_{s,t_min} = 1$ = First access is read
    model.addConstrs((delta_ram[page, first_read[page]] == 1 for page in first_read), name="emptyStart")

    # $d_{s,t} \leq d_{s,t+1} + \delta d_{s,t}$ = Eine Seite, verliert ihr dirty flag nur, wenn sie geschrieben wird
    model.addConstrs((dirty[p, t] <= dirty[p, t+1] + delta_dirty[p, t] for p in first_write for t in range(first_write[p], last_write[p] + 1)), "writeDirty");

    # Optional: $d_{s,t_{max}} = 0$ =Am ende ist der Puffer Sauber
    model.addConstrs((dirty[page, last_write[page] + 1] == 0 for page in last_write), name="cleanStop")

    # $p_{s,t}\geq 1$ wenn gelesen, $\geq 0$ sonst
    model.addConstrs((ram[(p,t)] == 1 for t,p in enumerate(accesses)), name="read")

    # $d_{s,t} \geq 1$ wenn geschrieben, $\geq 0$ sonst
    model.addConstrs((dirty[(p,t)] == 1 for t,p in enumerate(accesses) if(is_write[t])), name="write")

def setObjective(model, delta_ram, delta_dirty, writecost):
    print("Setting Objective")
    write_cost = model.addVar(writecost, writecost, name="WRITE_COST")

    # $\min \sum_{s,t} (\delta d_{s,t} \cdot \alpha + \delta p_{s,t})$
    if writecost != 0:
        model.setObjective(delta_ram.sum() + write_cost * delta_dirty.sum(), GRB.MINIMIZE)
    else:
        model.setObjectiveN(delta_ram.sum(), 0, priority=1)
        model.setObjectiveN(delta_dirty.sum(), 1, priority=0)
    return (model, delta_ram, delta_dirty)


def generateModel(accesses: list, is_write: list, ramsize, write_cost):
    try:
        model = initModel()
        (first_read, last_read, first_write, last_write) = getAccessLists(accesses, is_write)
        readTimes = createTimes(first_read, last_read)
        writeTimes = createTimes(first_write, last_write)
        (ram, delta_ram, dirty, delta_dirty) = addVariables(model, readTimes, writeTimes)
        addConstraints(model, writeTimes, ram, delta_ram, dirty, delta_dirty, first_read, last_read, first_write, last_write, accesses, is_write, ramsize)
        setObjective(model, delta_ram, delta_dirty, write_cost)
        model.update()
        model.write("modell.mps")

    except gp.GurobiError as e:
        print('Error code ' + str(e.errno) + ': ' + str(e))

    except AttributeError:
        print('Encountered an attribute error')

def generateDataset(file_name, max_page, length):
    accesses = [random.randint(0, max_page) for x in range(length)]
    is_write = random.choices([True, False], [0.1, 0.9], k=length)
    df = {"pages": accesses, "is_write": is_write}
    df = pandas.DataFrame(data=df)
    df.to_csv(file_name, index=False)

def generateZipf(file_name, max_page, length, zipf_factor):
    print("Generating zipf")
    accesses = list(zipfian.rvs(zipf_factor, max_page, size=length))
    is_write = random.choices([True, False], [0.1, 0.9], k=length)
    df = {"pages": accesses, "is_write": is_write}
    df = pandas.DataFrame(data=df)
    df.to_csv(file_name, index=False)


def run_file(file_name, ram, write_cost):
    df = pandas.read_csv(file_name)
    accesses = list(df["pages"])
    is_write = list(df["is_write"])
    generateModel(accesses, is_write, ramsize, write_cost)


ramsize = 20
write_cost = 10

csv_name = "wopt.csv"
try:
    print("try read from cache")
    run_file(csv_name, ramsize, write_cost)
except FileNotFoundError:
    print("No CSV with name \"{}\" found".format(csv_name))
    max_page=100
    length = 2000
    # generate(csv_name, max_page, length)
    generateZipf(csv_name, max_page, length, 0.4)

    run_file(csv_name, ramsize, write_cost)

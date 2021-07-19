#!/bin/python3

from os import write
import gurobipy as gp
from gurobipy import GRB, tupledict, tuplelist
from numpy import number
from scipy.stats import zipfian
import pandas, random


def calcCost(accesses, is_write, ramsize, write_cost):
    try:
        access_len = len(accesses)
        pages = set(accesses)
        if len(accesses) != len(is_write):
            print(len(accesses))
            print(len(is_write))
            assert(len(accesses) == len(is_write))

        print("Building Tuplelist {} {}".format(len(pages), access_len))
        def getValidTimesForPage(accesses: list, page: number):
            first = accesses.index(page)
            last = len(accesses) -1 - accesses[::-1].index(page)
            return (first, last)
        
        first = {}
        last = {}
        for p in pages:
            (start, end) = getValidTimesForPage(accesses, p)
            first[p] = start
            last[p] = end



        pageTimesPost: tuplelist = gp.tuplelist([(p,t) for p in pages for t in range(first[p], last[p] + 2)])
        
        print("Building model")

        model = gp.Model("writeOpt")

        print("Adding Variables")
        ram: tupledict = model.addVars(pageTimesPost, name="ram", vtype=GRB.BINARY)
        dirty: tupledict = model.addVars(pageTimesPost, name="dirty", vtype=GRB.BINARY)
        delta_ram: tupledict = model.addVars(pageTimesPost, name="delta_ram", vtype=GRB.BINARY)
        delta_dirty: tupledict = model.addVars(pageTimesPost, name="delta_dirty", vtype=GRB.BINARY)

        print("Adding constraints")
        #  $\sum_t p_{s,t} \leq P$ = Puffergröße
        model.addConstrs((ram.sum('*', time) <= ramsize for time in range(-1, access_len +1)), "capacity")

        # $\sum_t \delta p_{s,t} \leq 1$ = nur 1 read per step
        model.addConstrs((delta_ram.sum('*', time) <= 1 for time in range(-1, access_len +1)), "maxRead")

        # $d_{s,t} \leq p_{s,t}$ = Eine dirty Page ist im Puffer
        model.addConstrs((dirty[pageTime] <= ram[pageTime] for pageTime in pageTimesPost), "dirtyInRam")

        # $p_{s,t} \leq p_{s,t-1} + \delta p_{s,t}$ = Seite kann nur durch lesen eingelagert werden. (Ausnahme 1. Seite (Sonderregel))
        model.addConstrs((ram[p, t] <= ram[p, t-1] + delta_ram[p, t] for p in pages for t in range(first[p]+1, last[p] + 1)), "readFresh");

        # $d_{s,t} \leq d_{s,t+1} + \delta d_{s,t}$ = Eine Seite, verliert ihr dirty flag nur, wenn sie geschrieben wird
        model.addConstrs((dirty[p, t] <= dirty[p, t+1] + delta_dirty[p, t] for p in pages for t in range(first[p], last[p] + 1)), "writeDirty");

        #  - $\delta p_{s,t_min} = 1$ = First access is read
        model.addConstrs((delta_ram[page, first[page]] == 1 for page in pages), name="emptyStart")

        # Optional: $d_{s,t_{max}} = 0$ =Am ende ist der Puffer Sauber
        model.addConstrs((dirty[page, last[page] + 1] == 0 for page in pages), name="cleanStop")

        # $p_{s,t}\geq 1$ wenn gelesen, $\geq 0$ sonst
        model.addConstrs((ram[(p,t)] == 1 for t,p in enumerate(accesses)), name="read")

        # $d_{s,t} \geq 1$ wenn geschrieben, $\geq 0$ sonst
        model.addConstrs((dirty[(p,t)] == 1 for t,p in enumerate(accesses) if(is_write[t])), name="write")

        model.update()

        print("Adding Objective")
        # $\min \sum_{s,t} (\delta d_{s,t} \cdot \alpha + \delta p_{s,t})$
        if write_cost != 0:
            model.setObjective(delta_ram.sum() + write_cost * delta_dirty.sum(), GRB.MINIMIZE)
        else:
            model.setObjectiveN(delta_ram.sum(), 0, priority=1)
            model.setObjectiveN(delta_dirty.sum(), 1, priority=0)

        print("Optimizing")
        model.optimize()

        print('Obj: {:n}'.format(model.objVal))
        print("Reads: {:n}, Writes: {:n}".format(delta_ram.sum().getValue(), delta_dirty.sum().getValue()))

    except gp.GurobiError as e:
        print('Error code ' + str(e.errno) + ': ' + str(e))

    except AttributeError:
        print('Encountered an attribute error')

def generate(file_name, max_page, length):
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
    calcCost(accesses, is_write, ramsize, write_cost)


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

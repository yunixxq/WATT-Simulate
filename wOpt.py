#!/bin/python3

from os import write
import gurobipy as gp
from gurobipy import GRB, tupledict, tuplelist


def calcCost(accesses, is_write, ramsize, write_cost):
    try:
        access_len = len(accesses)
        pages = set(accesses)
        if len(accesses) != len(is_write):
            print(len(accesses))
            print(len(is_write))
            assert(len(accesses) == len(is_write))


        pageTimes: tuplelist = gp.tuplelist([(p,t) for p in pages for t in range(-1,access_len+1)])

        model = gp.Model("writeOpt")

        ram: tupledict = model.addVars(pageTimes, name="ram", vtype=GRB.BINARY)
        dirty: tupledict = model.addVars(pageTimes, name="dirty", vtype=GRB.BINARY)
        delta_ram: tupledict = model.addVars(pageTimes, name="delta_ram", vtype=GRB.BINARY)
        delta_dirty: tupledict = model.addVars(pageTimes, name="delta_dirty", vtype=GRB.BINARY)

        #  $\sum_t p_{s,t} \leq P$ = Puffergröße
        model.addConstrs((ram.sum('*', time) <= ramsize for time in range(-1, access_len +1)), "capacity")

        # $\sum_t \delta p_{s,t} \leq 1$ = nur 1 read per step
        model.addConstrs((delta_ram.sum('*', time) <= 1 for time in range(-1, access_len +1)), "maxRead")

        # $d_{s,t} \leq p_{s,t}$ = Eine dirty Page ist im Puffer
        model.addConstrs((dirty[pageTime] <= ram[pageTime] for pageTime in pageTimes), "dirtyInRam")

        # $p_{s,t} \leq p_{s,t-1} + \delta p_{s,t}$ = Seite kann nur durch lesen eingelagert werden.
        model.addConstrs((ram[p, t] <= ram[p, t-1] + delta_ram[p, t] for p in pages for t in range(access_len +1)), "readFresh");

        # $d_{s,t} \leq d_{s,t+1} + \delta d_{s,t}$ = Eine Seite, verliert ihr dirty flag nur, wenn sie geschrieben wird
        model.addConstrs((dirty[p, t] <= dirty[p, t+1] + delta_dirty[p, t] for p in pages for t in range(-1, access_len)), "writeDirty");

        #  - $p_{s,0} = 0$ = am anfang ist der Puffer leer
        model.addConstrs((ram[page, -1] == 0 for page in pages), name="emptyStart")

        # Optional: $d_{s,t_{max}} = 0$ =Am ende ist der Puffer Sauber
        model.addConstrs((dirty[page, access_len] == 0 for page in pages), name="cleanStop")

        # $p_{s,t}\geq 1$ wenn gelesen, $\geq 0$ sonst
        model.addConstrs((ram[(p,t)] == 1 for t,p in enumerate(accesses)), name="read")

        # $d_{s,t} \geq 1$ wenn geschrieben, $\geq 0$ sonst
        model.addConstrs((dirty[(p,t)] == 1 for t,p in enumerate(accesses) if(is_write[t])), name="write")

        model.update()

        # $\min \sum_{s,t} (\delta d_{s,t} \cdot \alpha + \delta p_{s,t})$
        model.setObjective(delta_ram.sum() + write_cost * delta_dirty.sum(), GRB.MINIMIZE)
        model.optimize()

        pagesInRam = []
        dirtyInRam = []
        for pageTime in pageTimes:
            if ram[pageTime].getAttr("x") == 1:
                pagesInRam += [pageTime]
            if dirty[pageTime].getAttr("x") == 1:
                dirtyInRam += [pageTime]

        ram_time = []
        for time in range(access_len):
            currentInRam = [page for (page, time2) in pagesInRam if time == time2]
            currentDirty = [page for (page, time2) in dirtyInRam if time == time2]
            ram_time += [(time, currentInRam, currentDirty)]
        for timestep in ram_time:
            time = timestep[0]
            if(is_write[time]):
                print("w", end="(")
            else:
                print("r", end="(")
            print(accesses[time], end=") ")
            pagesstrings = [str(page) for page in timestep[1] if page not in timestep[2]]
            pagesstrings += [str(page) + "x" for page in timestep[2]]
            print(pagesstrings)

        writes = delta_dirty.sum().getValue()
        if write_cost == 0:
            writes = 0
            for time in range(access_len-1):
                writes += sum([1 for x in ram_time[time][2] if x not in ram_time[time+1][2]])

        print('Obj: {:n}'.format(model.objVal))
        print("Reads: {:n}, Writes: {:n}".format(delta_ram.sum().getValue(), writes))

    except gp.GurobiError as e:
        print('Error code ' + str(e.errno) + ': ' + str(e))

    except AttributeError:
        print('Encountered an attribute error')

ramsize = 3
write_cost = 10
accesses = [2, 2, 2, 4, 7, 1, 10, 10, 3, 1, 7, 3, 7, 7, 2, 2, 6, 1, 5, 5, 7, 5, 8, 4, 5, 9, 1, 3, 3, 7, 7, 6, 3, 3, 7, 6, 3, 4, 6, 5, 2, 1, 10, 8, 5, 1, 1, 1, 2, 7, 1, 5, 5, 9, 8, 1, 7, 10, 8, 2]
is_write = [False, False, True, False, True, True, False, True, False, False, False, True, False, True, True, False, True, False, False, False, True, False, True, True, False, True, False, False, False, True, False, True, True, False, True, False, False, False, True, False, True, True, False, True, False, False, False, True, False, True, True, False, True, False, False, False, True, True, True, False]
calcCost(accesses, is_write, ramsize, write_cost)
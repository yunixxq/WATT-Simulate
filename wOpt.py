#!/bin/python3

import gurobipy as gp
from gurobipy import GRB, tupledict, tuplelist


def calcCost(accesses, is_write, ramsize):
    access_len = len(accesses)
    pages = set(accesses)
    assert(len(accesses) == len(is_write))


    pageTimes: tuplelist = gp.tuplelist([(p,t) for p in pages for t in range(-1,access_len+1)])

    model = gp.Model("writeOpt")

    ram: tupledict = model.addVars(pageTimes, name="ram", vtype=GRB.BINARY)
    dirty: tupledict = model.addVars(pageTimes, name="dirty", vtype=GRB.BINARY)
    delta_ram: tupledict = model.addVars(pageTimes, name="delta_ram", vtype=GRB.BINARY)
    delta_dirty: tupledict = model.addVars(pageTimes, name="delta_dirty", vtype=GRB.BINARY)

    #  $\sum_t p_{s,t} \leq P$ = Puffergröße
    for time in range(access_len):
        model.addConstr(ram.sum('*', time) <= ramsize, "capacity" + str(time))

    # $d_{s,t} \leq p_{s,t}$ = Eine dirty Page ist im Puffer
    for pageTime in pageTimes:
        model.addConstr(dirty[pageTime] <= ram[pageTime], "dirtyInRam" + str(pageTime))

    for pageTime in pageTimes:
        # $p_{s,t} \leq p_{s,t-1} + \delta p_{s,t}$ = Seite kann nur durch lesen eingelagert werden.
        if pageTime[1] != -1: # First read between -1 and 0
            model.addConstr(ram[pageTime] <= ram[pageTime[0], pageTime[1]-1] + delta_ram[pageTime]);

        # $d_{s,t} \leq d_{s,t+1} + \delta d_{s,t}$ = Eine Seite, verliert ihr dirty flag nur, wenn sie geschrieben wird
        if pageTime[1] != access_len: # last write betwen step -1 and  step
            model.addConstr(dirty[pageTime] <= dirty[pageTime[0], pageTime[1]+1] + delta_dirty[pageTime], "writeDirty" + str(pageTime));

        #  - $p_{s,0} = 0$ = am anfang ist der Puffer leer
        if pageTime[1] == -1:
            model.addConstr(ram[pageTime] == 0, "emptyStart" + str(pageTime));

        #  - Optional:
        #    - $d_{s,t_{max}} = 0$ =Am ende ist der Puffer Sauber
        if pageTime[1] == access_len and False:
            model.addConstr(ram[pageTime] == 0, "emptyStop" + str(pageTime));

    # $p_{s,t}\geq 1$ wenn gelesen, $\geq 0$ sonst
    for t, p in enumerate(accesses):
        model.addConstr(ram[(p,t)] == 1, "read(" + str(p) + ")At" + str(t))
        # $d_{s,t} \geq 1$ wenn geschrieben, $\geq 0$ sonst
        if is_write[t]:
            model.addConstr(dirty[(p,t)] == 1, "write(" + str(p) + ")At" + str(t))


    # $\min \sum_{s,t} (\delta d_{s,t} \cdot \alpha + \delta p_{s,t})$


ramsize = 10
accesses = [0, 10, 20, 12, 14, 13, 12, 14, 42]
is_write = [False, False, True, False, False, True, False, True, False]
calcCost(accesses, is_write, ramsize)
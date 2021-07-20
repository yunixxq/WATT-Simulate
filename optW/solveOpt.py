#!/bin/python3

from os import write
import gurobipy as gp
from gurobipy import GRB, tupledict, tuplelist
from numpy import number
from scipy.stats import zipfian
import pandas, random

writeCost = 2
ramSize = 200

model = gp.read("modell.mps")
model.setParam("Method", 1)
if writeCost != 10:
    write_cost = model.getVarByName("WRITE_COST")
    write_cost.setAttr("ub", writeCost)
    write_cost.setAttr("lb", writeCost)
if ramSize != 20:
    ram_size = model.getVarByName("RAMSIZE")
    ram_size.setAttr("ub", ramSize)
    ram_size.setAttr("lb", ramSize)
model.optimize()

model.write("modell.sol")

vars = model.getVars()
reads = sum([x.x for x in vars if "delta_ram" in x.varName])
writes = sum([x.x for x in vars if "delta_dirty" in x.varName])

print('Obj: {:n}'.format(model.objVal))
print("Reads: {:n}, Writes: {:n}".format(reads, writes))
print("RamSize: {:n}, WriteCost: {:n}".format(model.getVarByName("WRITE_COST").x, model.getVarByName("RAMSIZE").x))

#!/bin/python3

import gurobipy as gp

def evalModelMulti(writeCosts, ramSizes):
    model = gp.read("modell.mps")
    model.setParam("Method", 1)
    readList = []
    writeList = []
    for writeCost in writeCosts:
        for ramSize in ramSizes:
            print("Evaluating Model for writeCost {} and ramSize {}".format(writeCost, ramSize))
            write_cost = model.getVarByName("WRITE_COST")
            write_cost.setAttr("ub", writeCost)
            write_cost.setAttr("lb", writeCost)
            ram_size = model.getVarByName("RAMSIZE")
            ram_size.setAttr("ub", ramSize)
            ram_size.setAttr("lb", ramSize)
            model.optimize()

            model.write("modell_{}_{}.sol".format(writeCost, ramSize))

            vars = model.getVars()
            reads = sum([x.x for x in vars if "delta_ram" in x.varName])
            writes = sum([x.x for x in vars if "delta_dirty" in x.varName])

            print('Obj: {:n}'.format(model.objVal))
            print("Reads: {:n}, Writes: {:n}".format(reads, writes))
            print("RamSize: {:n}, WriteCost: {:n}".format(model.getVarByName("RAMSIZE").x, model.getVarByName("WRITE_COST").x))
            readList.append(reads)
            writeList.append(writes)
    return (readList, writeList)


if __name__ == "__main__":
    writeCost = [1]
    ramSize = [21]
    evalModelMulti(writeCost, ramSize)
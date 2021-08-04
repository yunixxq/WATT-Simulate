#!/bin/python3

from evalAccessTable import plotGraph, oneFullRun as generateStandards
from optW.generateOpt import run_file as generateModel
from optW.solveOpt import evalModelMulti
import pandas, os

def run_all(directory, writeCost):
    directory = directory + "_" + str(writeCost)
    try:
        os.mkdir(directory)
    except:
        pass

    file = "./test.csv"
    csv_start = directory + "/Data"
    read_file = csv_start + "reads.csv"
    write_file = csv_start + "writes.csv"
    generateStandards(file, csv_start, writeCost)
    generateModel(file)
    
    df_reads = pandas.read_csv(read_file)
    df_writes = pandas.read_csv(write_file)
    xList = list(df_reads["X"])

    (reads, writes) = evalModelMulti([writeCost], reversed(xList))
    print(reads)
    print(writes)
    df_reads["wOpt"] = list(reversed(reads))
    df_writes["wOpt"] = list(reversed(writes))
    df_reads.to_csv(read_file, index=False)
    df_writes.to_csv(write_file, index=False)
    plotGraph(csv_start, writeCost)




if __name__ == "__main__":
    run_all("./evalFullRun", 1)

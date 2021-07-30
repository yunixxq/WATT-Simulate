#!/bin/python3

from evalAccessTable import do_run as generateStandards
from optW.generateOpt import run_file as generateModel
from optW.solveOpt import evalModelMulti
import pandas

def run_all(writeCost):
    file = "./accesses_10000_3000_40_0.5_1.0.csv"
    read_file = "./ALL_heatUp_0/Datareads.csv"
    write_file = "./ALL_heatUp_0/Datawrites.csv"
    generateStandards(file)
    generateModel(file)
    
    df_reads = pandas.read_csv(read_file)
    df_writes = pandas.read_csv(write_file)
    xList = list(df_reads["X"])

    (reads, writes) = evalModelMulti([writeCost], xList)
    print(reads)
    print(writes)
    df_reads["wOpt"] = reads
    df_writes["wOpt"] = writes
    df_reads.to_csv(read_file)
    df_writes.to_csv(write_file)
    



if __name__ == "__main__":
    run_all(8)

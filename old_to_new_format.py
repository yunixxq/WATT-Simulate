#!/bin/python3

import matplotlib.pyplot as plt
from numpy import double
import pandas, sys
from PyPDF2 import PdfFileMerger

def convertFile(name):
    df_read = pandas.read_csv(name+"reads.csv")
    df_write = pandas.read_csv(name+"writes.csv")
    elements = df_read["elements"][0]

    for elem in list(df_read["elements"]) + list(df_write["elements"]):
        assert(elem == elements)

    output = pandas.DataFrame(columns=["algo", "ramSize", "elements", "reads", "writes"])
    fresh_df = pandas.DataFrame(columns=["algo", "ramSize", "elements", "reads", "writes"])
    
    labels = list(df_read.columns.values)
    labels.remove("X")
    labels.remove("elements")
    for algo in labels:
        for ramsize in range(len(df_read["X"])):
            fresh_df.loc[ramsize] = ([algo, df_read["X"][ramsize], elements, df_read[algo][ramsize], df_write[algo][ramsize]])
        output = output.append(fresh_df, ignore_index=True)

    output.to_csv(name+"output.csv", index=False)

if __name__ == "__main__":
    if len(sys.argv) ==2:
        write_cost = 1
        filepath = sys.argv[1]
        convertFile(filepath)

    else:
        print("Usage:")
        print(sys.argv[0], " directory")

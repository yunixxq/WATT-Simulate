#!/bin/python3

import matplotlib.pyplot as plt
from numpy import double
import pandas, sys
from PyPDF2 import PdfFileMerger

def genEvalList(costfactor, elements):
    
    def evalLists(missList, writeList):
        costList = map(lambda read, write: read + write*costfactor, missList, writeList)
        norm_miss = list(map(lambda x: x/elements, missList))
        norm_hit = list(map(lambda x: 1-x, norm_miss))
        norm_costList = list(map(lambda x: x/elements, costList))
        norm_write = list(map(lambda x: x/elements, writeList))
        return (norm_hit, norm_miss, norm_costList, norm_write)
    return evalLists
    


def plotGraph(name, write_cost = 8):
    df_read = pandas.read_csv(name+"reads.csv")
    df_write = pandas.read_csv(name+"writes.csv")
    elements = df_read["elements"][0]
    evalList = genEvalList(write_cost, elements)

    for elem in list(df_read["elements"]) + list(df_write["elements"]):
        assert(elem == elements)

    df_hit = pandas.DataFrame()
    df_miss = pandas.DataFrame()
    df_cost = pandas.DataFrame()
    df_writes = pandas.DataFrame()
    df_hit["X"] = df_read["X"]
    df_miss["X"] = df_read["X"]
    df_cost["X"] = df_read["X"]
    df_writes["X"] = df_read["X"]

    labels = list(df_read.columns.values)
    labels.remove("X")
    labels.remove("elements")
    labels = sorted(labels, key=lambda x: sum(df_read[x] + df_write[x]))
    for column in labels:
        (df_hit[column], df_miss[column], df_cost[column], df_writes[column]) = evalList(df_read[column], df_write[column])

    df_hit.set_index("X", inplace=True)
    df_miss.set_index("X", inplace=True)
    df_cost.set_index("X", inplace=True)
    df_writes.set_index("X", inplace=True)

    def plotGraphInner(df, title, ylabel, file, labels, limit=False, transpose=False):
        nonlocal merger
        if(transpose):
            df = df.transpose(copy=True)
            plt.xticks(rotation=90, fontsize=3)
            plt.xlabel("Algorithms")
        else:
            plt.xlabel("Buffer Size")
        
        labels = list(df.columns.values)
        linewidth = 1
        if len(labels) > 3:
            linewidth = 0.5
        if len(labels) > 14:
            linewidth = 0.1
        for label in labels:
            plt.plot(df.index, df[label], label=label, linewidth=linewidth)

        plt.ylabel(ylabel)
        if(limit):
            plt.ylim(0,1)
        if len(labels) > 10:
            plt.legend(prop={"size":5})
        else:
            plt.legend()
        plt.title(title)
        plt.savefig(file)
        plt.clf()
        merger.append(file)

    merger = PdfFileMerger()

    plotGraphInner(df_cost, "Cost", "Cost", name + "_cost.pdf", labels)
    plotGraphInner(df_cost, "Cost_T", "Cost", name + "_cost_t.pdf", labels, transpose=True)

    plotGraphInner(df_hit, "Hitrate", "Hits", name + "_hits.pdf", labels, limit=True)
    plotGraphInner(df_hit, "Hitrate_T", "Hits", name + "_hits_t.pdf", labels, limit=True, transpose=True)

    plotGraphInner(df_miss, "Missrate", "Misses", name + "_misses.pdf", labels, limit=True)
    plotGraphInner(df_miss, "Missrate_T", "Misses", name + "_misses_t.pdf", labels, limit=True, transpose=True)

    plotGraphInner(df_writes, "Writes", "Writes", name + "_writes.pdf", labels)
    plotGraphInner(df_writes, "Writes_T", "Writes", name + "_writes_t.pdf", labels, transpose=True)

    merger.write(name + "out.pdf")
    merger.close()

if __name__ == "__main__":
    if len(sys.argv) ==2:
        write_cost = 1
        filepath = sys.argv[1]
        plotGraph(filepath, write_cost)

    elif len(sys.argv) ==3:
        write_cost = int(sys.argv[2])
        filepath = sys.argv[1]
        plotGraph(filepath, write_cost)

    else:
        print("Usage:")
        print(sys.argv[0], " directory [write_factor_integer]")

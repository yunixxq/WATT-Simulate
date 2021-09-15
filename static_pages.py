#!/bin/python3

import matplotlib.pyplot as plt
import time
import pandas
import random
from generate_traces import simulate_zipf, generate_zipf_distribution

# x = random.zipf(a=2, size=1000)

# print(x)
writes_to_total_fraction = 0.3
total_access = 10000

write_zipf = 0.5
read_zipf = 1.0

total_pages = 100
writes_to_total_pages = 0.4

buffer_size = 20

write_cost_factor = 1

write_pages = int(total_pages * writes_to_total_pages)
write_access = int(writes_to_total_fraction * total_access)

read_pages = total_pages - write_pages
read_access = total_access - write_access

timer = time.time()
csv_name = "Distribution_{}_{}_{}_{}_{}.csv".format(total_access, write_access, write_pages, write_zipf, read_zipf)
csv_name2 = "accesses_{}_{}_{}_{}_{}.csv".format(total_access, write_access, write_pages, write_zipf, read_zipf)


def printTime(last):
    post = time.time()
    print(post-last)
    return post

write_distribution, read_distribution = ([], [])
try:
    print("try read from cache")
    df = pandas.read_csv(csv_name)
    dists = list(df["distribution"])
    write_distribution = dists[:write_pages]
    read_distribution = dists[write_pages:]
except FileNotFoundError:
    print("Simulating")
    write_accesses = simulate_zipf(write_pages, write_access, write_zipf)
    write_distribution = generate_zipf_distribution(write_accesses)
    timer = printTime(timer)
    read_accesses =  simulate_zipf(read_pages, read_access, read_zipf)
    read_distribution = generate_zipf_distribution(read_accesses)

    d = {'distribution': write_distribution + read_distribution}
    df = pandas.DataFrame(data=d)
    df.to_csv(csv_name, index=False)

    reads = [(x, False) for x in read_accesses]
    writes = [(x, True) for x in write_accesses]
    total = random.sample(reads + writes, len(reads+writes))
    (pages, is_write) = zip(*total)
    d2 = {'pages': pages, "is_write": is_write}
    df2 = pandas.DataFrame(data=d2)
    df2.to_csv(csv_name2, index=False)


timer = printTime(timer)

assert(sum(write_distribution + read_distribution) == total_access)
assert(len(write_distribution + read_distribution) == total_pages)

write_misses = []
read_misses = []
total_misses = []

timer = printTime(timer)

for read_buffer in range(0, buffer_size + 1):
    read_miss = read_access - sum(read_distribution[0:read_buffer])
    write_miss = write_access - sum(write_distribution[0:(buffer_size - read_buffer)])
    read_miss += write_miss
    read_misses.append(read_miss)
    write_misses.append(write_miss)
    total_misses.append(read_miss + write_miss * write_cost_factor)

timer = printTime(timer)

read_per_access = list(map(lambda x: x/total_access, read_misses))
write_per_access = list(map(lambda x: x/total_access, write_misses))
total_per_access = list(map(lambda x: x/total_access, total_misses))

timer = printTime(timer)


plt.plot(range(0, buffer_size + 1), read_per_access, label="Read_per_access")
plt.plot(range(0, buffer_size + 1), write_per_access, label="Write_per_access")
plt.plot(range(0, buffer_size + 1), total_per_access, label="Cost_per_access")
plt.xlabel("Read_buffer_size")
plt.ylabel("Cost per Access")
plt.legend()

plt.show()

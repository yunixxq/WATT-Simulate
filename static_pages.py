#!/bin/python3

import matplotlib.pyplot as plt
from scipy.stats import distributions, zipfian
import time
import pandas

# x = random.zipf(a=2, size=1000)

# print(x)

def printTime(last):
    post = time.time()
    print(post-last)
    return post
def startTime():
    return time.time()


def simulate(discrete_elements, simulation_length, zipf_factor=2):
    x = list(zipfian.rvs(zipf_factor, discrete_elements, size=simulation_length))
    distribution = []
    for element in range(1, discrete_elements+1):
        counter = x.count(element)
        distribution.append(counter)
    return distribution

writes_to_total_fraction = 0.3
total_access = 100000

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

timer = startTime()
csv_name = "Distribution_{}_{}_{}_{}_{}.csv".format(total_access, write_access, write_pages, write_zipf, read_zipf)

write_distribution, read_distribution = ([], [])
try:
    print("try read from cache")
    df = pandas.read_csv(csv_name)
    dists = list(df["distribution"])
    write_distribution = dists[:write_pages]
    read_distribution = dists[write_pages:]
except:
    print("Simulating")
    write_distribution = simulate(write_pages, write_access, write_zipf)
    timer = printTime(timer)
    read_distribution =  simulate(read_pages, read_access, read_zipf)

    d = {'distribution': write_distribution + read_distribution}
    df = pandas.DataFrame(data=d)

    df.to_csv(csv_name, index=False)

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

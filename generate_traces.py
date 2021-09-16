#!/bin/python3

from scipy.stats import zipfian

import random, pandas

def simulate_zipf(discrete_elements, simulation_length, zipf_factor=2, shuffle = False, pages=-1):
    accesses = list(zipfian.rvs(zipf_factor, discrete_elements, size=simulation_length))
    if(shuffle):
        if pages==-1:
            pages = discrete_elements
        shuffled_pages = list(random.sample(range(pages), pages))
        accesses = list(map(lambda x: shuffled_pages[x-1], accesses))
    return accesses

def generate_zipf_distribution(access_list):
    distribution = []
    for element in range(min(access_list), max(access_list)+1):
        counter = access_list.count(element)
        distribution.append(counter)
    assert(sum(distribution) == len(access_list))
    return distribution


def generate_zipf_file(total_pages, write_pages, write_access, write_zipf, read_pages, read_access, read_zipf, shuffle, csv_name):
    print("Generating Writes")
    write_accesses = simulate_zipf(write_pages, write_access, write_zipf, shuffle, pages=total_pages)
    print("Generating Reads")
    read_accesses = simulate_zipf(read_pages, read_access, read_zipf, shuffle, pages=total_pages)
    reads = [(x, False) for x in read_accesses]
    writes = [(x, True) for x in write_accesses]
    print("Shuffeling Accesses")
    total = random.sample(reads + writes, len(reads+writes))
    (pages, is_write) = zip(*total)
    d2 = {'pages': pages, "is_write": is_write}
    df2 = pandas.DataFrame(data=d2)
    df2.to_csv(csv_name, index=False)

if __name__ == "__main__":
    pages_total = 1000
    pages_w_fract = 0.4
    pages_r_fract = 1-pages_w_fract
    access_total = 5000
    access_w_fract = 0.4
    access_r_fract = 1-access_w_fract
    zipf_w = 0.5
    zipf_r = 1.0
    
    generate_zipf_file(
        pages_total,
        int(pages_total*pages_w_fract), int(access_total*access_w_fract), zipf_w,
        int(pages_total*pages_r_fract), int(access_total*access_r_fract), zipf_r,
        True, "zipf_accesses.csv")

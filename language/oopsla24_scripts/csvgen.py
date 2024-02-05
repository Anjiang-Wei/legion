#!/usr/bin/env python3

# https://raw.githubusercontent.com/StanfordLegion/legion/papers/index-launch-sc21/language/sc21_scripts/parse_results.py

import csv
import glob
import os
import re
import sys
import pprint
from statistics import median

repeat = 20
# swp_2_0_0/out_2_0_0_c_1_1000000_2000000_1414_1415_4_2_r1.log
# node, tileidx, ratioidx, c_o, dim, tilestart, tilecurrent, domainx, domainy, partx, party, repetition

_filename_re = re.compile(r'swp_([0-9]+)_([0-9]+)_([0-9]+)/out_([0-9]+)_([0-9]+)_([0-9]+)_(c|o)_(1|2)_([0-9]+)_([0-9]+)_([0-9]+)_([0-9]+)_([0-9]+)_([0-9]+)_r([0-9]+)[.]log')
def parse_basename(filename):
    match = re.match(_filename_re, filename)
    assert match is not None, filename
    assert len(match.groups()) == 15
    node, tileidx, ratioidx, node_, tileidx_, ratioidx_, c_o, dim, tilestart, tilecurrent, domainx, domainy, partx, party, repetition = match.groups()
    assert node == node_ and tileidx == tileidx_ and ratioidx == ratioidx_, filename
    node, tileidx, ratioidx, c_o, dim, tilestart, tilecurrent, domainx, domainy, partx, party, repetition = list(map(int, \
    (node, tileidx, ratioidx, c_o, dim, tilestart, tilecurrent, domainx, domainy, partx, party, repetition)))
    assert node * 4 == px * py, filename
    return (node, tileidx, ratioidx, c_o, dim, tilestart, tilecurrent, domainx, domainy, partx, party, repetition)

_content_re = re.compile(r'^ELAPSED TIME = +([0-9.]+) s$', re.MULTILINE)
def parse_content(path):
    with open(path, 'r') as f:
        content = f.read()
        match = re.search(_content_re, content)
        if match is None:
            print(f"Error parsing {path}")
            return 0
        assert len(match.groups()) == 1
        return float(match.groups()[0])

def compute_average(content):
    for key in content:
        if len(content[key]) != repeat:
            print(f"key = {key}, != repeat = {repeat}")
        content[key] = median(content[key])
    # sort the result by node number, then tileidx
    new_content = {key: val for key, val in sorted(content.items(), key = lambda x: (x[0][0], x[0][1]))}
    res = []
    for k in new_content.keys():
        res.append(list(k) + [new_content[k]])
    return res
    
def main():
    paths = glob.glob('*/*.log')
    res = {}
    for path in paths:
        # remove the repetition number
        file_id = parse_basename(path)[:-1]
        if file_id not in res:
            res[file_id] = []
        res[file_id].append(parse_content(path))
    avg = compute_average(res)
    # pprint.pprint(avg)
    
    out = csv.writer(open("all.csv", "w"))
    out.writerow(['node', 'tileidx', 'ratioidx', 'c_o', 'dim', 'tilestart', 'tilecurrent', 'domainx', 'domainy', 'partx', 'party', 'time'])
    out.writerows(avg)

if __name__ == '__main__':
    main()

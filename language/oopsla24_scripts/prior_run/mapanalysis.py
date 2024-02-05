#!/usr/bin/env python3

# https://raw.githubusercontent.com/StanfordLegion/legion/papers/index-launch-sc21/language/sc21_scripts/parse_results.py

import csv
import glob
import os
import re
import sys
import pprint
from statistics import median

repeat = 5
# map_8_32_8/out_32_8_8_4_map_r1.log
# map_8_32_8/out_32_8_8_4_ori_r5.log

_filename_re = re.compile(r'map_([0-9]+)_([0-9]+)_([0-9]+)/out_([0-9]+)_([0-9]+)_([0-9]+)_([0-9]+)_(ori|map)_r([0-9]+)[.]log')
def parse_basename(filename):
    match = re.match(_filename_re, filename)
    assert match is not None
    assert len(match.groups()) == 9
    ori_map = match.groups()[-2]
    left_groups = match.groups()[:-2] + match.groups()[-1:]
    node_num, domain_x, domain_y, domain_x_, domain_y_, gpu_x, gpu_y, repetition = list(map(int, left_groups))
    assert domain_x == domain_x_
    assert domain_y == domain_y_
    assert node_num * 4 == gpu_x * gpu_y
    if ori_map == "map":
        tag = "o"
    elif ori_map == "ori":
        tag = "c"
    else:
        assert False, ori_map
    return (node_num, domain_x, domain_y, gpu_x, gpu_y, repetition, tag)

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
            print("content[key] = {content[key]}, != repeat = {repeat}")
        content[key] = median(content[key])
    # sort the result by node number, then domain_x
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
        file_id = parse_basename(path)[:-2] + tuple(parse_basename(path)[-1])
        if file_id not in res:
            res[file_id] = []
        res[file_id].append(parse_content(path))
    avg = compute_average(res)
    pprint.pprint(avg)
    
    out = csv.writer(open("map.csv", "w"))
    out.writerow(['node_num', 'domain_x', 'domain_y', 'gpu_x', 'gpu_y', 'tag', 'time'])
    out.writerows(avg)

if __name__ == '__main__':
    main()

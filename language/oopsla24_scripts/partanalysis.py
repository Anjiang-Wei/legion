#!/usr/bin/env python3

# https://raw.githubusercontent.com/StanfordLegion/legion/papers/index-launch-sc21/language/sc21_scripts/parse_results.py

import csv
import glob
import os
import re
import sys
import pprint

# part_1_16/out_1_16_1_16_r5.log

_filename_re = re.compile(r'out_([0-9]+)_([0-9]+)_([0-9]+)_([0-9]+)_r([0-9]+)[.]log')
def parse_basename(filename):
    match = re.match(_filename_re, filename)
    assert match is not None
    assert len(match.groups()) == 5
    # NX, NY, NTX, NTY, repetition
    return tuple(map(int, match.groups()))

_content_re = re.compile(r'^ELAPSED TIME = +([0-9.]+) s$', re.MULTILINE)
def parse_content(path):
    with open(path, 'r') as f:
        content = f.read()
        match = re.search(_content_re, content)
        if match is None:
            print(f"Error parsing {path}")
            exit(1)
        assert len(match.groups()) == 1
        return float(match.groups()[0])

def compute_average(content):
    for key in content:
        content[key] = sum(content[key]) / len(content[key])
    # sort the result by key[0]*key[1] (total GPU numbers), key[0] (NX)
    new_content = {key: val for key, val in sorted(content.items(), key = lambda x: (x[0][0] * x[0][1], x[0][0]))}
    res = []
    for k in new_content.keys():
        res.append(list(k) + [new_content[k]])
    return res
    
def main():
    paths = glob.glob('*/*.log')
    res = {}
    for path in paths:
        if parse_basename(os.path.basename(path)) not in res:
            res[parse_basename(os.path.basename(path))] = []
        res[parse_basename(os.path.basename(path))].append(parse_content(path))
    avg = compute_average(res)
    pprint.pprint(avg)
    
    out = csv.writer(sys.stdout)
    out.writerow(['NX', 'NY', 'NTX', 'NTY', 'time'])
    out.writerows(avg)

if __name__ == '__main__':
    main()

#!/usr/bin/env python3

# https://raw.githubusercontent.com/StanfordLegion/legion/papers/index-launch-sc21/language/sc21_scripts/parse_results.py

import csv
import glob
import os
import re
import sys

# part_1_16/out_1_16_1_16_r5.log

_filename_re = re.compile(r'out_([0-9]+)_([0-9]+)_([0-9]+)_([0-9]+)_r([0-9]+)[.]log')
def parse_basename(filename):
    match = re.match(_filename_re, filename)
    assert match is not None
    assert len(match.groups) == 5
    # NX, NY, NTX, NTY, repetition
    return list(map(int, match.groups()))

_content_re = re.compile(r'^ELAPSED TIME = +([0-9.]+) s$', re.MULTILINE)
def parse_content(path):
    with open(path, 'r') as f:
        content = f.read()
        match = re.search(_content_re, content)
        if match is None:
            print(f"Error parsing {path}")
            exit(1)
        assert len(match.groups) == 1
        return float(match.groups()[0])

def compute_average(content):
    pass
    
def main():
    paths = glob.glob('*/*.log')
    content = [parse_basename(os.path.basename(path)) + parse_content(path) for path in paths]
    print(content)
    # content.sort(key=lambda row: (row[0], int(row[1]), int(row[2]), int(row[3])))

    # average = compute_average(content)
    # # with open(out_filename, 'w') as f:
    # out = csv.writer(sys.stdout)# , dialect='excel-tab') # f)
    # # out.writerow(['system', 'nodes', 'procs_per_node', 'rep', 'elapsed_time', 'gflops'])
    # out.writerow(['system', 'nodes', 'procs_per_node', 'rep', 'average_time'])
    # out.writerows(average)

if __name__ == '__main__':
    main()

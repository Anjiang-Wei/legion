#!/usr/bin/env python3

# https://raw.githubusercontent.com/StanfordLegion/legion/papers/index-launch-sc21/language/sc21_scripts/parse_results.py

import csv
import glob
import os
import re
import sys

_filename_re = re.compile(r'out_([0-9]+)x([0-9]+)_r([0-9]+)[.]log')
def parse_basename(filename):
    match = re.match(_filename_re, filename)
    assert match is not None
    return match.groups()

_content_re = re.compile(r'^ELAPSED TIME = +([0-9.]+) s$', re.MULTILINE)
def parse_content(path):
    with open(path, 'r') as f:
        content = f.read()
        match = re.search(_content_re, content)
        if match is None:
            print(f"Error parsing {path}")
            return ('ERROR',)
        return match.groups()

# # GFLOPS = 485417.560 GFLOPS
# _content_re2 = re.compile(r'^GFLOPS = +([0-9.]+) GFLOPS$', re.MULTILINE)
# def parse_content2(path):
#     with open(path, 'r') as f:
#         content = f.read()
#         match = re.search(_content_re2, content)
#         if match is None:
#             return ('ERROR',)
#         return match.groups()
def compute_average(content):
    global repetition
    res = []
    cur = []
    err_num = 0
    for item in content:
        if item[:3] == cur[:3]: # [system, nodes, procs_per_node] the same
            if cur[-1] != "ERROR":
                cur[-2] += 1 # one more repetitions
                cur[-1] += item[-1] # float??
            else:
                err_num += 1
        else:
            if cur != []:
                cur[-1] = cur[-1] / cur[-2] # time averaged by the number of repetitions
                res.append(cur)
            cur = list(item) # instantiate cur with the new row
            cur[-2] = 1 # first repetition
    print(f"{err_num} errors detected")
    return res

def main():
    paths = glob.glob('*/*.log')
    # content = [(os.path.dirname(path),) + parse_basename(os.path.basename(path)) + parse_content(path) + parse_content2(path) for path in paths]
    content = [(os.path.dirname(path),) + parse_basename(os.path.basename(path)) + parse_content(path) for path in paths]
    content.sort(key=lambda row: (row[0], int(row[1]), int(row[2]), int(row[3])))

    average = compute_average(content)
    # with open(out_filename, 'w') as f:
    out = csv.writer(sys.stdout)# , dialect='excel-tab') # f)
    # out.writerow(['system', 'nodes', 'procs_per_node', 'rep', 'elapsed_time', 'gflops'])
    out.writerow(['system', 'nodes', 'procs_per_node', 'rep', 'average_time'])
    out.writerows(average)

if __name__ == '__main__':
    main()

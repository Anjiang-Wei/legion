#!/usr/bin/env python3

# https://raw.githubusercontent.com/StanfordLegion/legion/papers/index-launch-sc21/language/sc21_scripts/parse_results.py

import csv
import glob
import os
import re

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

def main():
    paths = glob.glob('*/*.log')
    # content = [(os.path.dirname(path),) + parse_basename(os.path.basename(path)) + parse_content(path) + parse_content2(path) for path in paths]
    content = [(os.path.dirname(path),) + parse_basename(os.path.basename(path)) + parse_content(path) for path in paths]
    content.sort(key=lambda row: (row[0], int(row[1]), int(row[2]), int(row[3])))

    import sys
    # with open(out_filename, 'w') as f:
    out = csv.writer(sys.stdout)# , dialect='excel-tab') # f)
    # out.writerow(['system', 'nodes', 'procs_per_node', 'rep', 'elapsed_time', 'gflops'])
    out.writerow(['system', 'nodes', 'procs_per_node', 'rep', 'elapsed_time'])
    out.writerows(content)

if __name__ == '__main__':
    main()

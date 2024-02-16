import sys
import pandas as pd
import pprint
import math
from matplotlib.backends.backend_pdf import PdfPages
import matplotlib.pyplot as plt

# node,tileidx,ratioidx,c_o,dim,tilestart,tilecurrent,domainx,domainy,partx,party,time
# 1,0,0,o,1,1000000,1000000,1000,1000,2,2,0.029
enable_o1 = False
enable_c2 = False

def calc_stats(group):
    # print(group)
    assert len(group) >= 2 and len(group) <= 4
    # node = group['node'].values[0]
    tilestart = group['tilestart'].values[0]
    gridsize = tilestart * tilestart
    o1 = group.loc[(group['c_o'] == 'o') & (group['dim'] == 1), 'time'].values[0]
    c1 = group.loc[(group['c_o'] == 'c') & (group['dim'] == 1), 'time'].values[0]
    if len(group.loc[(group['c_o'] == 'o') & (group['dim'] == 2), 'time'].values) > 0:
        o2 = group.loc[(group['c_o'] == 'o') & (group['dim'] == 2), 'time'].values[0]
    else:
        o2 = o1
    if len(group.loc[(group['c_o'] == 'c') & (group['dim'] == 2), 'time'].values) > 0:
        c2 = group.loc[(group['c_o'] == 'c') & (group['dim'] == 2), 'time'].values[0]
    else:
        c2 = c1
    o1, o2, c1, c2 = gridsize / o1, gridsize / o2, gridsize / c1, gridsize / c2
    return pd.Series({'o1': o1, 'o2': o2, 'c1': c1, 'c2': c2})

def pdf_ratio(df, tile_first):
    for (node, tileidx), group in df.groupby(['node', 'tileidx']):
        if tile_first:
            pdf_fname = f"t{tileidx}_n{node}.pdf"
        else:
            pdf_fname = f"n{node}_t{tileidx}.pdf"
        # print(f"node = {node}, tileidx={tileidx}\n{group}")
        with PdfPages('ratio/' + pdf_fname) as pdf:
            plt.figure()
            if enable_o1:
                plt.plot((2**group['ratioidx']).astype(str), group['o1'], marker='o', label='o1')
            plt.plot((2**group['ratioidx']).astype(str), group['o2'], marker='x', label='o2')
            plt.plot((2**group['ratioidx']).astype(str), group['c1'], marker='s', label='c1')
            if enable_c2:
                plt.plot((2**group['ratioidx']).astype(str), group['c2'], marker='D', label='c2')
            plt.title(f'Throughput for Node {node}, Tile {tileidx}')
            plt.xlabel('(dy:dx)')
            plt.ylabel('Throughput')
            # plt.ylim(-5, 125)
            plt.legend()
            plt.xticks(rotation=45)
            plt.grid(True)
            pdf.savefig()
            plt.close()

def pdf_node(df, tile_first = True):
    for (ratioidx, tileidx), group in df.groupby(['ratioidx', 'tileidx']):
        # print(f"dy:dx = {ratioidx}, tileidx = {tileidx}\n{group}")
        if tile_first:
            pdf_fname = f"t{tileidx}_r{ratioidx}.pdf"
        else:
            pdf_fname = f"r{ratioidx}_t{tileidx}.pdf"
        with PdfPages("node/" + pdf_fname) as pdf:
            plt.figure()
            if enable_o1:
                plt.plot(group['node'].astype(str), group['o1'], marker='o', label='o1')
            plt.plot(group['node'].astype(str), group['o2'], marker='x', label='o2')
            plt.plot(group['node'].astype(str), group['c1'], marker='s', label='c1')
            if enable_c2:
                plt.plot(group['node'].astype(str), group['c2'], marker='D', label='c2')
            plt.title(f'Throughput Trends for ratio {2**ratioidx}, tile {tileidx}')
            plt.xlabel('node')
            plt.ylabel('Throughput')
            # plt.ylim(-5, 125)
            plt.legend()
            plt.xticks(rotation=45)
            plt.grid(True)
            pdf.savefig()
            plt.close()

def pdf_tile(df, node_first = True):
    for (node, ratioidx), group in df.groupby(['node', 'ratioidx']):
        # print(f"node = {node}, dx = {dx}, dy = {dy}\n{group}")
        if node_first:
            pdf_fname = f"n{node}_r{ratioidx}.pdf"
        else:
            pdf_fname = f"r{ratioidx}_n{node}.pdf"
        with PdfPages("tile/" + pdf_fname) as pdf:
            plt.figure()
            if enable_o1:
                plt.plot(group['tileidx'].astype(str), group['o1'], marker='o', label='o1')
            plt.plot(group['tileidx'].astype(str), group['o2'], marker='x', label='o2')
            plt.plot(group['tileidx'].astype(str), group['c1'], marker='s', label='c1')
            if enable_c2:
                plt.plot(group['tileidx'].astype(str), group['c2'], marker='D', label='c2')
            plt.title(f'Throughput Trends for Node = {node}, ratio = {2**ratioidx}')
            plt.xlabel('tile')
            plt.ylabel('Throughput')
            # plt.ylim(-5, 125)
            plt.legend()
            plt.xticks(rotation=45)
            plt.grid(True)
            pdf.savefig()
            plt.close()


def main(fname):
    df = pd.read_csv(fname)
    # node,tileidx,ratioidx,c_o,dim
    grouped = df.groupby(['node', 'tileidx', 'ratioidx'])
    imp_df = grouped.apply(calc_stats).reset_index()
    print(imp_df)
    pdf_ratio(imp_df, tile_first = True)
    pdf_ratio(imp_df, tile_first = False)
    pdf_node(imp_df, tile_first = True)
    pdf_node(imp_df, tile_first = False)
    pdf_tile(imp_df, node_first = True)
    pdf_tile(imp_df, node_first = False)


if __name__ == "__main__":
    fname = sys.argv[1]
    main(fname)

import sys
import pandas as pd
import pprint
import math
from matplotlib.backends.backend_pdf import PdfPages
import matplotlib.pyplot as plt

# node,dx,dy,tile,px,py,c_o,dim,time
# 1,1,256,1250,1,4,o,1,0.254

thoughput = True

def calc_improvement(group):
    assert len(group) == 2 or len(group) == 3
    node = group['node'].values[0]
    o1 = group.loc[(group['c_o'] == 'o') & (group['dim'] == 1), 'time'].values[0]
    t_o1 = group.loc[(group['c_o'] == 'o') & (group['dim'] == 1), 'tile'].values[0]
    c1 = group.loc[(group['c_o'] == 'c') & (group['dim'] == 1), 'time'].values[0]
    t_c1 = group.loc[(group['c_o'] == 'c') & (group['dim'] == 1), 'tile'].values[0]
    assert t_o1 == t_c1
    tile = t_o1
    if len(group) == 2:
        o2 = o1
    else:
        o2 = group.loc[(group['c_o'] == 'o') & (group['dim'] == 2), 'time'].values[0]
    o1, o2, c1 = tile * tile / o1 / node, tile * tile / o2 / node, tile * tile / c1 / node
    # print(f"o1_imp = {o1_imp}, o2_imp = {o2_imp}")
    return pd.Series({'o1': o1, 'o2': o2, 'c1': c1})

def compute_tile_idx(row):
    node = row['node']
    tile = row['tile']
    res = {
        1: [50, 100, 250, 500, 750, 1000, 1250],
    }
    # {1: [50, 100, 250, 500, 750, 1000, 1250],
    # 2: [70, 141, 353, 707, 1060, 1414, 1767],
    # 4: [100, 200, 500, 1000, 1500, 2000, 2500],
    # 8: [141, 282, 707, 1414, 2121, 2828, 3535],
    # 16: [200, 400, 1000, 2000, 3000, 4000, 5000],
    # 32: [282, 565, 1414, 2828, 4242, 5656, 7071],
    # 64: [400, 800, 2000, 4000, 6000, 8000, 10000]}
    idx = 1
    for iternode in [2, 4, 8, 16, 32, 64]:
        res[iternode] = [math.floor(item * (2 ** (0.5 * idx))) for item in res[1]]
        idx += 1
    assert node in res.keys(), f"node = {node}"
    assert tile in res[node], f"node = {node}, tile = {tile}"
    return res[node].index(tile)

def pdf_node_tile(df, tile_first):
    for (node, tile, tileidx), group in df.groupby(['node', 'tile', 'tileidx']):
        if tile_first:
            pdf_fname = f"t{tileidx}_n{node}.pdf"
        else:
            pdf_fname = f"n{node}_t{tile}.pdf"
        print(f"node = {node}, tile = {tile}, tileidx={tileidx}\n{group}")
        with PdfPages('snode/' + pdf_fname) as pdf:
            plt.figure()
            plt.plot(group['dx'].astype(str) + ',' + group['dy'].astype(str), group['o1'], marker='o', label='o1')
            plt.plot(group['dx'].astype(str) + ',' + group['dy'].astype(str), group['o2'], marker='x', label='o2')
            plt.plot(group['dx'].astype(str) + ',' + group['dy'].astype(str), group['c1'], marker='s', label='c1')
            plt.title(f'Throughput for Node {node}, Tile {tile}')
            plt.xlabel('(dx,dy)')
            plt.ylabel('Throughput')
            # plt.ylim(-5, 125)
            plt.legend()
            plt.xticks(rotation=45)
            plt.grid(True)
            pdf.savefig()
            plt.close()

def pdf_dx_dy(df, tile_first = True):
    for (dx, dy, tileidx), group in df.groupby(['dx', 'dy', 'tileidx']):
        print(f"dx = {dx}, dy = {dy}, tileidx = {tileidx}\n{group}")
        if tile_first:
            pdf_fname = f"t{tileidx}_dx{dx}_dy{dy}.pdf"
        else:
            pdf_fname = f"dx{dx}_dy{dy}_t{tileidx}.pdf"
        with PdfPages("sdx/" + pdf_fname) as pdf:
            plt.figure()
            plt.plot(group['node'].astype(str) + ',' + group['tile'].astype(str), group['o1'], marker='o', label='o1')
            plt.plot(group['node'].astype(str) + ',' + group['tile'].astype(str), group['o2'], marker='x', label='o2')
            plt.plot(group['node'].astype(str) + ',' + group['tile'].astype(str), group['c1'], marker='s', label='c1')
            plt.title(f'Throughput Trends for (dx,dy) = ({dx},{dy})')
            plt.xlabel('(node,tile)')
            plt.ylabel('Throughput')
            # plt.ylim(-5, 125)
            plt.legend()
            plt.xticks(rotation=45)
            plt.grid(True)
            pdf.savefig()
            plt.close()

def pdf_tile_trend(df, node_first = True):
    for (node, dx, dy), group in df.groupby(['node', 'dx', 'dy']):
        print(f"node = {node}, dx = {dx}, dy = {dy}\n{group}")
        if node_first:
            pdf_fname = f"n{node}_dx{dx}_dy{dy}.pdf"
        else:
            pdf_fname = f"dx{dx}_dy{dy}_n{node}.pdf"
        with PdfPages("stile/" + pdf_fname) as pdf:
            plt.figure()
            plt.plot(group['tile'].astype(str), group['o1'], marker='o', label='o1')
            plt.plot(group['tile'].astype(str), group['o2'], marker='x', label='o2')
            plt.plot(group['tile'].astype(str), group['c1'], marker='s', label='c1')
            plt.title(f'Throughput Trends for Node = {node}, (dx,dy) = ({dx},{dy})')
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
    grouped = df.groupby(['node', 'tile', 'dx', 'dy'])
    imp_df = grouped.apply(calc_improvement).reset_index()
    imp_df['tileidx'] = imp_df.apply(compute_tile_idx, axis=1)
    pdf_node_tile(imp_df, tile_first = True)
    pdf_node_tile(imp_df, tile_first = False)
    # pdf_dx_dy(imp_df, tile_first = True)
    # pdf_dx_dy(imp_df, tile_first = False)
    # pdf_tile_trend(imp_df, node_first = True)
    # pdf_tile_trend(imp_df, node_first = False)


if __name__ == "__main__":
    fname = sys.argv[1]
    main(fname)

import sys
import pandas as pd
from matplotlib.backends.backend_pdf import PdfPages
import matplotlib.pyplot as plt

# node,dx,dy,tile,px,py,c_o,dim,time
# 1,1,256,1250,1,4,o,1,0.254

def calc_improvement(group):
    assert len(group) == 2 or len(group) == 3
    o1 = group.loc[(group['c_o'] == 'o') & (group['dim'] == 1), 'time'].values[0]
    c1 = group.loc[(group['c_o'] == 'c') & (group['dim'] == 1), 'time'].values[0]
    if len(group) == 2:
        o2 = o1
    else:
        o2 = group.loc[(group['c_o'] == 'o') & (group['dim'] == 2), 'time'].values[0]
    o1_imp = (c1 - o1) / c1 * 100
    o2_imp = (c1 - o2) / c1 * 100
    print(f"o1_imp = {o1_imp}, o2_imp = {o2_imp}")
    return pd.Series({'o1_imp': o1_imp, 'o2_imp': o2_imp})


def dump_to_pdf(df):
    for (node, tile), group in df.groupby(['node', 'tile']):
        pdf_fname = f"n{node}_t{tile}.pdf"
        print(f"node = {node}, tile = {tile}, group = {group}")
        with PdfPages(pdf_fname) as pdf:
            plt.figure()
            plt.plot(group['dx'].astype(str) + ',' + group['dy'].astype(str), group['o1_imp'], marker='o', label='o1 Improvement')
            plt.plot(group['dx'].astype(str) + ',' + group['dy'].astype(str), group['o2_imp'], marker='x', label='o2 Improvement')
            plt.title(f'Improvement Trends for Node {node}, Tile {tile}')
            plt.xlabel('(dx,dy)')
            plt.ylabel('Improvement (%)')
            plt.legend()
            plt.xticks(rotation=45)
            plt.grid(True)
            pdf.savefig()
            plt.close()


def main(fname):
    df = pd.read_csv(fname)
    grouped = df.groupby(['node', 'tile', 'dx', 'dy'])
    imp_df = grouped.apply(calc_improvement).reset_index()
    dump_to_pdf(imp_df)


if __name__ == "__main__":
    fname = sys.argv[1]
    main(fname)
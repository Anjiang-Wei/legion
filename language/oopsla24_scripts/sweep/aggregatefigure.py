import pandas as pd
import numpy as np
import matplotlib
import matplotlib.pyplot as plt

# Load the CSV file
file_path = 'all.csv'
data = pd.read_csv(file_path)

data = data[data['tileidx'] != 4]
data = data[data['node'] != 64]
data = data[data['ratioidx'] <= 5]

# Grouping the data by 'node', 'tileidx', 'ratioidx'
grouped = data.groupby(['node', 'tileidx', 'ratioidx', 'tilecurrent'])

# Function to calculate improvement percentage
def calculate_improvement(group):
    # Try to get c_o='o', dim=2, if not available, use c_o='o', dim=1
    o_time = group[(group['c_o'] == 'o') & (group['dim'] == 2)]['time']
    if group[(group['c_o'] == 'o') & (group['dim'] == 2)].empty:
        o_time = group[(group['c_o'] == 'o') & (group['dim'] == 1)]['time']

    o_time = o_time.values[0]
    
    c_time = group[(group['c_o'] == 'c') & (group['dim'] == 1)]['time'].values[0]

    grid_size = float(group['tilecurrent'].values[0]) * float(group['tilecurrent'].values[0])
    o_throughput = grid_size / o_time
    c_throughput = grid_size / c_time

    improvement = o_throughput / c_throughput
    return improvement

# Apply the function to each group
improvement_percentages = grouped.apply(calculate_improvement).reset_index()

# 1. Geometric Mean with respect to Node
improvement_percentages.groupby('node').apply(lambda x: x.prod()**(1/len(x)))

geo_mean_node = improvement_percentages.groupby('node')[0].apply(lambda x: x.prod()**(1/len(x)))

# 2. Geometric Mean with respect to Tileidx
geo_mean_tileidx = improvement_percentages.groupby('tileidx')[0].apply(lambda x: x.prod()**(1/len(x)))

# 3. Geometric Mean with respect to Ratioidx
geo_mean_ratioidx = improvement_percentages.groupby('ratioidx')[0].apply(lambda x: x.prod()**(1/len(x)))

# Function to plot the geometric means
def plot_geometric_means(data, x_label, y_label='Improvement Percentage'):
    plt.figure(figsize=(12, 8))
    tick_fontsize = 20
    title_fontsize = 25
    print(x_label, data)
    if x_label == 'Machines':
        # Define the node values and map them to integers
        nodes = [1, 2, 4, 8, 16, 32]
        node_to_int = {node: i for i, node in enumerate(nodes)}

        # Convert data to a list of tuples (integer mapping, y-value)
        data = [(node_to_int.get(node, node), (value - 1) * 100) for node, value in data.items()]

        # Plot the data
        plt.plot(*zip(*data), "--o", linewidth=3, markersize=20)  # Unzipping the data
        plt.ylim(0, 30)
        plt.yticks(range(0, 31, 10), fontsize=tick_fontsize)

        # Set custom x-ticks
        plt.xticks(range(len(nodes)), [f"{node} ({4*node})" for node in nodes], fontsize=tick_fontsize)
        plt.xlabel('Number of Nodes (GPUs)', fontsize=title_fontsize)
    elif x_label == 'Area':
        idx2position = {0: 10**6, 1: 10**7, 2: 10**8, 3: 2 * 10**8, 5: 4 * 10**8}
        data = [(idx2position[idx], (value - 1) * 100) for idx, value in data.items()]

        plt.plot(*zip(*data),  "--o", linewidth=3, markersize=20)

        # Set the x-axis to logarithmic scale
        plt.xscale('log')

        # Define the x-ticks and their labels
        xticks = [10**6, 10**7, 10**8, 2 * 10**8, 4 * 10**8]
        xticklabels = ["$10^6$", "$10^7$", "$10^8$", "$2 \\times 10^8$", "$4 \\times 10^8$"]

        plt.xticks(xticks, xticklabels, fontsize=tick_fontsize)

         # Disable minor ticks
        plt.gca().xaxis.set_minor_formatter(matplotlib.ticker.NullFormatter())
        plt.gca().xaxis.set_minor_locator(matplotlib.ticker.NullLocator())

        # Optional: Add vertical grid lines at major tick positions
        # plt.grid(True, which='major', axis='x', linestyle='--')

        plt.ylim(0, 40)
        plt.yticks(range(0, 41, 10), fontsize=tick_fontsize)
        plt.xlabel('Area of Iteration Space Per Node ($x * y / \\# nodes$)', fontsize=title_fontsize)
    
    elif x_label == 'Aspect Ratio':
        data = [(idx, (value - 1) * 100) for idx, value in data.items()]
        plt.plot(*zip(*data),  "--o", linewidth=3, markersize=20)
        # plt.ylim(0, 50)
        # plt.yticks(range(0, 51, 10), fontsize=tick_fontsize)
        # plt.xticks(range(0, 10), ["1:1", "2:1", "4:1", "8:1", "16:1", "32:1", "64:1", "128:1", "256:1", "512:1"], fontsize=tick_fontsize)
        plt.ylim(0, 30)
        plt.yticks(range(0, 31, 10), fontsize=tick_fontsize)
        plt.xticks(range(0, 6), ["1:1", "1:2", "1:4", "1:8", "1:16", "1:32"], fontsize=tick_fontsize)
        plt.xlabel('Aspect Ratio of Iteration Space ($x : y$)', fontsize=title_fontsize)

    plt.title('Improvement Percentage w.r.t. ' + x_label, fontsize=title_fontsize)
    plt.ylabel('Improvement Percentage (%)', fontsize=title_fontsize)
    # plt.grid(True)
    # plt.show()
    plt.savefig(f'{x_label.split(" ")[0].lower()}_improvement.pdf')

# Plotting
plot_geometric_means(geo_mean_node, 'Machines')
plot_geometric_means(geo_mean_tileidx, 'Area')
plot_geometric_means(geo_mean_ratioidx, 'Aspect Ratio')
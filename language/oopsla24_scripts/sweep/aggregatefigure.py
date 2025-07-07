import pandas as pd
import numpy as np
import matplotlib
import matplotlib.pyplot as plt

# Global font size configuration - matching distribution.py
TITLE_FONTSIZE = 25
LABEL_FONTSIZE = 30
TICK_FONTSIZE = 25
LEGEND_FONTSIZE = 25

# Set global matplotlib parameters for consistent styling
plt.rcParams.update({
    'font.size': TICK_FONTSIZE,
    'axes.titlesize': TITLE_FONTSIZE,
    'axes.labelsize': LABEL_FONTSIZE,
    'xtick.labelsize': TICK_FONTSIZE,
    'ytick.labelsize': TICK_FONTSIZE,
    'legend.fontsize': LEGEND_FONTSIZE,
    'figure.titlesize': TITLE_FONTSIZE
})

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
        plt.yticks(range(0, 31, 10), fontsize=TICK_FONTSIZE)

        # Set custom x-ticks - only show GPU numbers
        gpu_counts = [4*node for node in nodes]
        plt.xticks(range(len(nodes)), gpu_counts, fontsize=TICK_FONTSIZE)
        plt.xlabel('Number of GPUs', fontsize=LABEL_FONTSIZE)
    elif x_label == 'Area':
        idx2position = {0: 10**6, 1: 10**7, 2: 10**8, 3: 2 * 10**8, 5: 4 * 10**8}
        data = [(idx2position[idx], (value - 1) * 100) for idx, value in data.items()]

        plt.plot(*zip(*data),  "--o", linewidth=3, markersize=20)

        # Use log scale to properly separate the first two points
        plt.xscale('log')
        
        # Define the x-ticks and their labels with better formatting
        xticks = [10**6, 10**7, 10**8, 2 * 10**8, 4 * 10**8]
        xticklabels = ["1M", "10M", "100M", "200M", "400M"]

        plt.xticks(xticks, xticklabels, fontsize=TICK_FONTSIZE)

        plt.ylim(0, 40)
        plt.yticks(range(0, 41, 10), fontsize=TICK_FONTSIZE)
        plt.xlabel('Area of Iteration Space Per Node', fontsize=LABEL_FONTSIZE)
    
    elif x_label == 'Aspect Ratio':
        data = [(idx, (value - 1) * 100) for idx, value in data.items()]
        plt.plot(*zip(*data),  "--o", linewidth=3, markersize=20)
        plt.ylim(0, 30)
        plt.yticks(range(0, 31, 10), fontsize=TICK_FONTSIZE)
        plt.xticks(range(0, 6), ["1:1", "1:2", "1:4", "1:8", "1:16", "1:32"], fontsize=TICK_FONTSIZE)
        plt.xlabel('Aspect Ratio of Iteration Space', fontsize=LABEL_FONTSIZE)

    # Remove title as requested
    plt.ylabel('Performance Improvement (%)', fontsize=LABEL_FONTSIZE)
    
    # Add grid for better readability
    plt.grid(True, alpha=0.3, linestyle='-', linewidth=0.5)
    
    # Improve layout
    plt.tight_layout()
    
    # Save with higher DPI for better quality
    plt.savefig(f'{x_label.split(" ")[0].lower()}_improvement.pdf', dpi=300, bbox_inches='tight')
    plt.close()

# Plotting
plot_geometric_means(geo_mean_node, 'Machines')
plot_geometric_means(geo_mean_tileidx, 'Area')
plot_geometric_means(geo_mean_ratioidx, 'Aspect Ratio')

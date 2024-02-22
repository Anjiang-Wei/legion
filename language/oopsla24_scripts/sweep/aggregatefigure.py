import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

# Load the CSV file
file_path = 'all.csv'
data = pd.read_csv(file_path)

data = data[data['tileidx'] != 4]
data = data[data['node'] != 64]

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
print(improvement_percentages.groupby('node').apply(lambda x: x.prod()**(1/len(x))))

geo_mean_node = improvement_percentages.groupby('node')[0].apply(lambda x: x.prod()**(1/len(x)))

# 2. Geometric Mean with respect to Tileidx
geo_mean_tileidx = improvement_percentages.groupby('tileidx')[0].apply(lambda x: x.prod()**(1/len(x)))

# 3. Geometric Mean with respect to Ratioidx
geo_mean_ratioidx = improvement_percentages.groupby('ratioidx')[0].apply(lambda x: x.prod()**(1/len(x)))

# Function to plot the geometric means
def plot_geometric_means(data, title, x_label, y_label='Geometric Mean of Improvement %'):
    plt.figure(figsize=(10, 6))
    plt.plot(data, marker='o')
    plt.title(title)
    plt.xlabel(x_label)
    plt.ylabel(y_label)
    plt.grid(True)
    # plt.show()
    plt.savefig(f'{title}.pdf')

# Plotting
plot_geometric_means(geo_mean_node, 'Improvement Percentage Geometric Mean by Node', 'Node')
plot_geometric_means(geo_mean_tileidx, 'Improvement Percentage Geometric Mean by Tileidx', 'Tileidx')
plot_geometric_means(geo_mean_ratioidx, 'Improvement Percentage Geometric Mean by Ratioidx', 'Ratioidx')

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

# Finding the rows with smallest and biggest improvement percentages
smallest_improvement_row = improvement_percentages.loc[improvement_percentages[0].idxmin()]
biggest_improvement_row = improvement_percentages.loc[improvement_percentages[0].idxmax()]

# Display the results
print("Smallest Improvement:\n", smallest_improvement_row)
print("\nBiggest Improvement:\n", biggest_improvement_row)
print("length of data: ", len(improvement_percentages))

improvement_data = improvement_percentages[0]

# compute the geometric mean
geo_mean = improvement_data.prod()**(1/len(improvement_data))

# print the result
print("Geometric Mean: ", geo_mean)

# Plotting the histogram
# plt.figure(figsize=(10, 6))
plt.figure()
plt.hist((improvement_data - 1) * 100, bins=50, color='green', density=True)
title = 'Distribution of Improvement Percentages'
plt.title(title)
plt.xlabel('Improvement Percentage')
plt.ylabel('Percentage of Occurrences')
plt.gca().yaxis.set_major_formatter(matplotlib.ticker.PercentFormatter(1, decimals=0))
# plt.grid(True)
# plt.xlim()
# plt.show()
plt.savefig(f'distribution.pdf')

import pandas as pd
import numpy as np
import matplotlib
import matplotlib.pyplot as plt

# Global font size configuration - easily tunable
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

# Create a more professional-looking figure
fig, ax = plt.subplots(figsize=(12, 8))

# Convert to percentage for better readability
improvement_percentages_display = (improvement_data - 1) * 100

# Filter out negative improvements (below 0%)
improvement_percentages_display = improvement_percentages_display[improvement_percentages_display >= 0]

# Create histogram with improved styling
n, bins, patches = ax.hist(improvement_percentages_display, 
                          bins=50, 
                          color='steelblue', 
                          alpha=0.7, 
                          density=True,
                          edgecolor='white',
                          linewidth=0.5)

# Improved labels (without title)
ax.set_xlabel('Performance Improvement (%)', fontsize=LABEL_FONTSIZE)
ax.set_ylabel('Density', fontsize=LABEL_FONTSIZE)

# Format y-axis as percentage
ax.yaxis.set_major_formatter(matplotlib.ticker.PercentFormatter(1, decimals=0))

# Add grid for better readability
ax.grid(True, alpha=0.3, linestyle='-', linewidth=0.5)

# Improve layout
plt.tight_layout()

# Save with higher DPI for better quality
plt.savefig('distribution.pdf', dpi=300, bbox_inches='tight')
plt.close()

print(f"Figure saved as 'distribution.pdf'")
print(f"Filtered out {len((improvement_data - 1) * 100) - len(improvement_percentages_display)} negative improvement cases")
print(f"Remaining data points: {len(improvement_percentages_display)}")

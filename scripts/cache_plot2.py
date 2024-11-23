import matplotlib.pyplot as plt
import numpy as np

results = {
    "with_cache": {
        "uniform_50%": (33.112, 1.95),
        "zipfian_50%": (31.712, 1.80)
    },
    "without_cache": {
        "uniform_50%": (44.52, 1.44),
        "zipfian_50%": (44.83, 1.51)
    }
}

# Data for plotting
labels = ['Uniform Distribution (50% Cache)', 'Zipfian Distribution (50% Cache)']
cache_means = [results["with_cache"]["uniform_50%"][0], results["with_cache"]["zipfian_50%"][0]]
cache_stddevs = [results["with_cache"]["uniform_50%"][1], results["with_cache"]["zipfian_50%"][1]]
no_cache_means = [results["without_cache"]["uniform_50%"][0], results["without_cache"]["zipfian_50%"][0]]
no_cache_stddevs = [results["without_cache"]["uniform_50%"][1], results["without_cache"]["zipfian_50%"][1]]

# X-axis positions for the bars
x = np.arange(len(labels))  # label locations
width = 0.35  # width of the bars

# Plotting the bar chart
fig, ax = plt.subplots(figsize=(10, 6))

# Bars for "With Cache"
bars1 = ax.bar(x - width/2, cache_means, width, label='With Cache', yerr=cache_stddevs, capsize=5, color='skyblue')

# Bars for "Without Cache"
bars2 = ax.bar(x + width/2, no_cache_means, width, label='Without Cache', yerr=no_cache_stddevs, capsize=5, color='salmon')

# Labels and formatting
ax.set_xlabel('Distribution and Cache Configuration')
ax.set_ylabel('Average Latency (ms)')
ax.set_title('Cache Performance with Appends)')
ax.set_xticks(x)
ax.set_xticklabels(labels)
ax.legend()

# Display bar values on top of bars
# def add_bar_labels(bars):
#     for rect in bars:
#         height = rect.get_height()
#         ax.annotate(f'{height:.1f}',
#                     xy=(rect.get_x() + rect.get_width() / 2, height),
#                     xytext=(0, 3),  # Offset text above bar
#                     textcoords="offset points",
#                     ha='center', va='bottom')

# add_bar_labels(bars1)
# add_bar_labels(bars2)

def add_labels(bars, means, stddevs):
    for bar, mean, stddev in zip(bars, means, stddevs):
        height = bar.get_height()
        ax.annotate(f'{mean} ± {stddev}',
                    xy=(bar.get_x() + bar.get_width() / 2, height),
                    xytext=(0, 3),
                    textcoords="offset points",
                    ha='center', va='bottom')

add_labels(bars1, cache_means, cache_stddevs)
add_labels(bars2, no_cache_means, no_cache_stddevs)

plt.tight_layout()
plt.show()

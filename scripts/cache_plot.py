import matplotlib.pyplot as plt

# Cache sizes for x-axis labels
cache_sizes = [10, 20, 50, 100]

## Averages after taking 3 readings per point

latencies_with_cache_uniform = [39.745, 36.221, 31.407, 22.948]
stddev_with_cache_uniform = [1.54, 1.26, 1.32, 1.93]

latencies_with_cache_zipfian = [39.192, 35.565, 28.948, 19.420]
stddev_with_cache_zipfian = [1.88, 1.55, 1.58, 2.11]

latencies_without_cache_uniform = [41.410, 41.410, 41.410, 41.410]
stddev_without_cache_uniform = [1.94, 1.94, 1.94, 1.94]

latencies_without_cache_zipfian = [42.049, 42.049, 42.049, 42.049]
stddev_without_cache_zipfian = [1.81, 1.81, 1.81, 1.81]

# Cache sizes as percentages
cache_sizes = [10, 20, 50, 100]


# Plotting
plt.figure(figsize=(12, 6))

# Plot for "With Cache" - Uniform
plt.errorbar(
    cache_sizes, latencies_with_cache_uniform, yerr=stddev_with_cache_uniform,
    label="With Cache - Uniform", fmt='-o', capsize=5
)
for i, (avg, std) in enumerate(zip(latencies_with_cache_uniform, stddev_with_cache_uniform)):
    plt.annotate(f'{avg:.2f}±{std:.2f}', (cache_sizes[i], latencies_with_cache_uniform[i]))


# Plot for "With Cache" - Zipfian
plt.errorbar(
    cache_sizes, latencies_with_cache_zipfian, yerr=stddev_with_cache_zipfian,
    label="With Cache - Zipfian", fmt='-o', capsize=5
)
for i, (avg, std) in enumerate(zip(latencies_with_cache_zipfian, stddev_with_cache_zipfian)):
    plt.annotate(f'{avg:.2f}±{std:.2f}', (cache_sizes[i], latencies_with_cache_zipfian[i]))


# Plot for "Without Cache" - Uniform
plt.errorbar(
    cache_sizes, latencies_without_cache_uniform, yerr=stddev_without_cache_uniform,
    label="Without Cache - Uniform", fmt='-o', capsize=5
)
for i, (avg, std) in enumerate(zip(latencies_without_cache_uniform, stddev_without_cache_uniform)):
    plt.annotate(f'{avg:.2f}±{std:.2f}', (cache_sizes[i], latencies_without_cache_uniform[i]))


# Plot for "Without Cache" - Zipfian
plt.errorbar(
    cache_sizes, latencies_without_cache_zipfian, yerr=stddev_without_cache_zipfian,
    label="Without Cache - Zipfian", fmt='-o', capsize=5
)
for i, (avg, std) in enumerate(zip(latencies_without_cache_zipfian, stddev_without_cache_zipfian)):
    plt.annotate(f'{avg:.2f}±{std:.2f}', (cache_sizes[i], latencies_without_cache_zipfian[i]))


# Labels and title
plt.xlabel("Cache Size (%)")
plt.ylabel("Latency (in s)")
plt.title("Read Latency with and without Client-Side Caching")
plt.legend()
plt.grid(True)
plt.xticks(cache_sizes)
plt.tight_layout()

# Show plot
plt.show()


import matplotlib.pyplot as plt
import numpy as np

# Cache sizes for x-axis labels
# file_size = [5, 10, 20, 50, 75, 100]
file_size = [1, 2, 5, 10]

## Averages after taking 3 readings per point

# re_replication_time_ms = [[29, 29, 36, 24, 27], [58, 30, 45, 39, 55], [128, 107, 113, 118, 208], [301, 255, 224, 250, 173], [334, 373, 250, 329, 346], [538, 532, 419, 535, 486]]
# re_replication_std_deviation = [np.std(times) for times in re_replication_time_ms]
# re_replication_time_ms = [np.mean(times) for times in re_replication_time_ms]

# re_replication_bandwidth = [[172.4, 172, 138, 208, 185], [172, 333, 222, 256, 181], [156, 186, 176, 169, 196], [223, 200, 289, 166, 196], [295, 227, 216, 224, 201], [238, 186, 205, 185, 187]]
# re_replication_bandwidth_std_deviation = [np.std(times) for times in re_replication_bandwidth]
# re_replication_bandwidth = [np.mean(times) for times in re_replication_bandwidth]

# re_replication_bandwidth = [[172.4, 172, 138, 208, 185], [172, 333, 222, 256, 181], [156, 186, 176, 169, 196], [223, 200, 289, 166, 196], [295, 227, 216, 224, 201], [238, 186, 205, 185, 187]]
# re_replication_bandwidth_std_deviation = [np.std(times) for times in re_replication_bandwidth]
# re_replication_bandwidth = [np.mean(times) for times in re_replication_bandwidth]

multi_append_4kb = [[887, 861], [924, 905], [1071, 898], [864, 868]]
multi_append_4kb_std_deviation = [np.std(times) for times in multi_append_4kb]
multi_append_4kb = [np.mean(times) for times in multi_append_4kb]

multi_append_40kb = [[1110, 1310], [1320, 1420], [1410, 1280], [1219, 1207]]
multi_append_40kb_std_deviation = [np.std(times) for times in multi_append_40kb]
multi_append_40kb = [np.mean(times) for times in multi_append_40kb]





# Plotting
plt.figure(figsize=(12, 6))

# Plot for "Multi Append 4KB"
plt.errorbar(
    file_size, multi_append_4kb, yerr=multi_append_4kb_std_deviation,
    label="Multi Append 4KB", fmt='-o', capsize=5
)
for i, (avg, std) in enumerate(zip(multi_append_4kb, multi_append_4kb_std_deviation)):
    plt.annotate(f'{avg:.2f}±{std:.2f}', (file_size[i], multi_append_4kb[i]))

# Plot for "Multi Append 40KB"
plt.errorbar(
    file_size, multi_append_40kb, yerr=multi_append_40kb_std_deviation,
    label="Multi Append 40KB", fmt='-o', capsize=5
)
for i, (avg, std) in enumerate(zip(multi_append_40kb, multi_append_40kb_std_deviation)):
    plt.annotate(f'{avg:.2f}±{std:.2f}', (file_size[i], multi_append_40kb[i]))

# Labels and title
plt.xlabel("Number of clients")
plt.ylabel("Time (ms)")
plt.title("Multi Append Time for 1000 concurrent appends")
plt.legend()
plt.grid(True)
plt.xticks(file_size)
plt.tight_layout()

# Show plot
plt.show()





# # Plotting
# plt.figure(figsize=(12, 6))

# # Plot for "With Cache" - Uniform
# # plt.errorbar(
# #     file_size, re_replication_time_ms, yerr=re_replication_std_deviation,
# #     label="Re-replication time", fmt='-o', capsize=5
# # )
# # for i, (avg, std) in enumerate(zip(re_replication_time_ms, re_replication_std_deviation)):
# #     plt.annotate(f'{avg:.2f}±{std:.2f}', (file_size[i], re_replication_time_ms[i]))

# plt.errorbar(
#     file_size, re_replication_bandwidth, yerr=re_replication_bandwidth_std_deviation,
#     label="Re-replication bandwidth", fmt='-o', capsize=5
# )
# for i, (avg, std) in enumerate(zip(re_replication_bandwidth, re_replication_bandwidth_std_deviation)):
#     plt.annotate(f'{avg:.2f}±{std:.2f}', (file_size[i], re_replication_bandwidth[i]))



# Labels and title
# plt.xlabel("File Size (MB)")
# plt.ylabel("Re replication bandwidth (in MB/s)")
# plt.title("Re-replication bandwidth")
# plt.legend()
# plt.grid(True)
# plt.xticks(file_size)
# plt.tight_layout()

# # Show plot
# plt.show()
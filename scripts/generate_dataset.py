import os
dataset_dir = './dataset'



os.makedirs(dataset_dir, exist_ok=True)

for i in range(1, 5001):
    with open(os.path.join(dataset_dir, f"file_{i}"), "wb") as f:
        f.write(os.urandom(4000))  # 4KB of random data


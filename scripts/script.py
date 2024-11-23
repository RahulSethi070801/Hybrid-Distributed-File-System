import random
import numpy as np

with open('output.txt', 'w') as file:
    for i in range(1, 10001):
        file.write(f'create file_{i} hydfs_{i}\n')


# uniform distribution           
with open('get_uniform.txt', 'w') as file:
    for i in range(1, 20001):
        file_id = random.randint(1, 1000)
        file.write(f'get hydfs_{file_id} local_{file_id}\n')
  
  
# zipfian distribution      
def zipfian_distribution(n, alpha=1.0):
    ranks = np.arange(1, n + 1)
    weights = ranks ** (-alpha)
    weights /= weights.sum()
    return np.random.choice(ranks, size=n, p=weights)

zipfian_ids = zipfian_distribution(20000)

with open('get_zipfian.txt', 'w') as file:
    for file_id in zipfian_ids:
        file.write(f'get hydfs_{file_id} local_{file_id}\n')
import time
from concurrent.futures import ThreadPoolExecutor

def square(x):
    return x ** 2

numbers = list(range(1, 10**6))  

# Sequential Execution
start_time = time.time()
squared_numbers_seq = [x**2 for x in numbers]
end_time = time.time()
print(f"Sequential Execution Time: {end_time - start_time:.4f} seconds")

# Parallel Execution using ThreadPoolExecutor
start_time = time.time()
with ThreadPoolExecutor() as executor:
    squared_numbers_parallel = list(executor.map(square, numbers))
end_time = time.time()
print(f"Threaded Execution Time: {end_time - start_time:.4f} seconds")

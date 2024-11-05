import pandas as pd
import matplotlib.pyplot as plt

data = pd.read_csv('part_2.csv')

plt.figure(figsize=(10, 6))
plt.plot(data['Processes'], data['AverageTime(s)'], marker='o', linestyle='-')
plt.xlabel('Число процессов')
plt.ylabel('Время (с)')
plt.title('Зависимость времени выполнения от числа процессов')
plt.grid(True)
plt.savefig('average_time_vs_processes.png')
plt.close()

plt.figure(figsize=(10, 6))
plt.plot(data['Processes'], data['AverageCriterionK'], marker='o', linestyle='-')
plt.xlabel('Число процессов')
plt.ylabel('Критерий K')
plt.title('Зависимость критерия K от числа процессов')
plt.grid(True)
plt.savefig('criterion_k_vs_processes.png')
plt.close()

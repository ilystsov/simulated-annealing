import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

data = pd.read_csv('part_1.csv')

temperature_schedules = data['TemperatureSchedule'].unique()

for schedule in temperature_schedules:
    schedule_data = data[data['TemperatureSchedule'] == schedule]
    
    heatmap_time_data = schedule_data.pivot(index='Processors', columns='Tasks', values='AverageTime(s)')
    
    heatmap_time_data = heatmap_time_data.sort_index(ascending=False)
    
    plt.figure(figsize=(10, 8))
    sns.heatmap(heatmap_time_data, cmap='magma', cbar_kws={'label': 'Время (с)'}, yticklabels=heatmap_time_data.index)
    plt.title(f'Время выполнения алгоритма (с) - {schedule}')
    plt.xlabel('Количество работ')
    plt.ylabel('Количество процессоров')
    filename_time = f'final_nproc1_heatmap_time_{schedule}.png'
    plt.savefig(filename_time)
    print(f'Saved {filename_time}')
    plt.close()

    heatmap_k_data = schedule_data.pivot(index='Processors', columns='Tasks', values='AverageCriterionK')
    
    heatmap_k_data = heatmap_k_data.sort_index(ascending=False)
    
    plt.figure(figsize=(10, 8))
    sns.heatmap(heatmap_k_data, cmap='viridis', cbar_kws={'label': 'Критерий K'}, yticklabels=heatmap_k_data.index)
    plt.title(f'Критерий K - {schedule}')
    plt.xlabel('Количество работ')
    plt.ylabel('Количество процессоров')
    filename_k = f'final_nproc1_heatmap_k_{schedule}.png'
    plt.savefig(filename_k)
    print(f'Saved {filename_k}')
    plt.close()

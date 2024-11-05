#include <iostream>
#include <fstream>
#include <chrono>
#include "parallel.hpp"
#include "generate_data.hpp"

void analyze(const std::string& dataFilename, int minProcessors, int maxProcessors, int processorStep,
             int minTasks, int maxTasks, int taskStep, 
             int minProcesses, int maxProcesses, int processStep,
             double initialTemperature, int maxIterWithoutImprovement, int maxOuterIterationsWithoutImprovement, 
             int numRuns) {

    std::ofstream outFile("parallel_results_nproc1_final.csv");
    outFile << "Processors,Tasks,TemperatureSchedule,AverageTime(s),AverageCriterionK,Processes\n";

    BoltzmannSchedule tempScheduleBoltzmann(initialTemperature);
    CauchySchedule tempScheduleCauchy(initialTemperature);
    LogarithmicSchedule tempScheduleLogarithmic(initialTemperature);
    TemperatureSchedule* schedules[] = {&tempScheduleBoltzmann, &tempScheduleCauchy, &tempScheduleLogarithmic};
    const char* scheduleNames[] = {"Boltzmann", "Cauchy", "Logarithmic"};

    for (int numProcessors = minProcessors; numProcessors <= maxProcessors; numProcessors += processorStep) {
        for (int numTasks = minTasks; numTasks <= maxTasks; numTasks += taskStep) {
            generateDataToCSV(dataFilename, numProcessors, numTasks, 1, 100);

            for (int numProcesses = minProcesses; numProcesses <= maxProcesses; numProcesses += processStep) {
                for (int i = 0; i < 3; ++i) {
                    double totalDuration = 0.0;
                    double totalCriterionK = 0.0;

                    for (int run = 0; run < numRuns; ++run) {
                        double bestCriterion = 0.0;

                        auto start = std::chrono::high_resolution_clock::now();

                        SchedulingSolution* bestSolution = run_parallel_simulated_annealing(
                            dataFilename, numProcesses, schedules[i],
                            initialTemperature, maxIterWithoutImprovement, 
                            maxOuterIterationsWithoutImprovement, bestCriterion);

                        auto end = std::chrono::high_resolution_clock::now();
                        std::chrono::duration<double> duration = end - start;

                        totalDuration += duration.count();
                        totalCriterionK += bestCriterion;

                        if (bestSolution) {
                            delete bestSolution;
                        } else {
                            std::cerr << "Не удалось найти лучшее решение в запуске " << run + 1 << ".\n";
                        }
                    }

                    double averageDuration = totalDuration / numRuns;
                    double averageCriterionK = totalCriterionK / numRuns;

                    outFile << numProcessors << "," << numTasks << "," << scheduleNames[i] << ","
                            << averageDuration << "," << averageCriterionK << "," << numProcesses << "\n";

                    std::cout << "Число процессоров = " << numProcessors 
                              << ", Число задач = " << numTasks
                              << ", График температуры = " << scheduleNames[i]
                              << ", Число процессов = " << numProcesses 
                              << "\nСреднее время выполнения: " << averageDuration << " секунд"
                              << "\nСредний критерий K: " << averageCriterionK << "\n\n";
                }
            }
        }
    }

    outFile.close();
}

int main() {
    std::string dataFilename = "tasks.csv";
    int minProcessors = 2;
    int maxProcessors = 503;
    int processorStep = 100;
    int minTasks = 2500;
    int maxTasks = 22500;
    int taskStep = 2500;
    int minProcesses = 1;
    int maxProcesses = 1;
    int processStep = 1;
    double initialTemperature = 1000.0;
    int maxIterWithoutImprovement = 100;
    int maxOuterIterationsWithoutImprovement = 10;
    int numRuns = 5;

    analyze(dataFilename, minProcessors, maxProcessors, processorStep,
            minTasks, maxTasks, taskStep,
            minProcesses, maxProcesses, processStep,
            initialTemperature, maxIterWithoutImprovement, 
            maxOuterIterationsWithoutImprovement, numRuns);

    return 0;
}

#include <cmath>
#include <vector>
#include <algorithm>
#include <random>
#include <fstream>
#include <sstream>
#include <iostream>
#include <ctime>
#include <unistd.h>
#include "generate_data.hpp"

std::random_device rd;
std::mt19937 gen(rd());

class Solution {
public:
    virtual ~Solution() = default;

    virtual double evaluate() const = 0;

    virtual Solution* clone() const = 0;
};


class Mutation {
public:
    virtual ~Mutation() = default;

    virtual void apply(Solution& solution) const = 0;
};


class TemperatureSchedule {
public:
    virtual ~TemperatureSchedule() = default;

    virtual double updateTemperature(int iteration) const = 0;
};

class BoltzmannSchedule : public TemperatureSchedule {
    double T0;
public:
    explicit BoltzmannSchedule(double initialTemp) : T0(initialTemp) {}

    double updateTemperature(int iteration) const override {
        return T0 / std::log(1 + iteration);
    }
};

class CauchySchedule : public TemperatureSchedule {
    double T0;
public:
    explicit CauchySchedule(double initialTemp) : T0(initialTemp) {}

    double updateTemperature(int iteration) const override {
        return T0 / (1 + iteration);
    }
};

class LogarithmicSchedule : public TemperatureSchedule {
    double T0;
public:
    explicit LogarithmicSchedule(double initialTemp) : T0(initialTemp) {}

    double updateTemperature(int iteration) const override {
        return T0 * std::log(1 + iteration) / (1 + iteration);
    }
};

class SimulatedAnnealing {
    Solution* currentSolution;
    Mutation* mutation;
    TemperatureSchedule* tempSchedule;
    Solution* bestSolution;
    double T0;
    int maxIterWithoutImprovement;
    int currentIteration;
    int iterationsWithoutImprovement;

public:
    SimulatedAnnealing(Solution* initialSolution, Mutation* mut, TemperatureSchedule* tempSched, double initialTemp, int maxIterWithoutImp)
        : currentSolution(initialSolution->clone()), mutation(mut), tempSchedule(tempSched), T0(initialTemp), maxIterWithoutImprovement(maxIterWithoutImp), currentIteration(0), iterationsWithoutImprovement(0) {
        bestSolution = currentSolution->clone();
    }

    ~SimulatedAnnealing() {
        delete currentSolution;
        delete bestSolution;
    }

    bool performSingleIteration(double temperature) {
        Solution* newSolution = currentSolution->clone();
        mutation->apply(*newSolution);

        double deltaF = newSolution->evaluate() - currentSolution->evaluate();
        bool improved = false;

        std::uniform_real_distribution<double> dist(0.0, 1.0);
        double randomValue = dist(gen);

        if (deltaF <= 0 || (std::exp(-deltaF / temperature) > randomValue)) {
            delete currentSolution;
            currentSolution = newSolution;

            if (currentSolution->evaluate() < bestSolution->evaluate()) {
                delete bestSolution;
                bestSolution = currentSolution->clone();
                improved = true;
            }
        } else {
            delete newSolution;
        }

        return improved;
    }


    void run() {
        iterationsWithoutImprovement = 0;
        double temperature = T0;

        while (iterationsWithoutImprovement < maxIterWithoutImprovement) {
            for (int i = 0; i < 50; ++i) {
                bool improved = performSingleIteration(temperature);
                if (improved) {
                    iterationsWithoutImprovement = 0;
                } else {
                    ++iterationsWithoutImprovement;
                }

                if (iterationsWithoutImprovement >= maxIterWithoutImprovement) {
                    break;
                }
            }

            ++currentIteration;
            temperature = tempSchedule->updateTemperature(currentIteration);

            if (iterationsWithoutImprovement >= maxIterWithoutImprovement) {
                break;
            }
        }
    }

    Solution* getBestSolution() const {
        return bestSolution;
    }

};


class SchedulingSolution : public Solution {
    std::vector<std::vector<int>> schedule;
    std::vector<int> taskTimes;

public:
    SchedulingSolution(int numProcessors, const std::vector<int>& times) 
        : schedule(numProcessors), taskTimes(times) {}

    double evaluate() const override {
        double totalWaitTime = 0.0;

        for (const auto& processorTasks : schedule) {
            int completionTime = 0;

            for (int task : processorTasks) {
                completionTime += taskTimes[task];
                totalWaitTime += completionTime;
            }
        }
        return totalWaitTime;
    }

    SchedulingSolution* clone() const override {
        SchedulingSolution* newSolution = new SchedulingSolution(schedule.size(), taskTimes);
        newSolution->schedule = schedule;
        return newSolution;
    }


    std::vector<std::vector<int>>& getSchedule() {
        return schedule;
    }

    void addTask(int processor, int task) {
        schedule[processor].push_back(task);
    }


void serialize(int fd) const {
        int numProcessors = schedule.size();
        write(fd, &numProcessors, sizeof(int));

        int taskTimesSize = taskTimes.size();
        write(fd, &taskTimesSize, sizeof(int));

        write(fd, taskTimes.data(), taskTimesSize * sizeof(int));

        for (const auto& tasks : schedule) {
            int tasksSize = tasks.size();
            write(fd, &tasksSize, sizeof(int));

            if (tasksSize > 0) {
                write(fd, tasks.data(), tasksSize * sizeof(int));
            }
        }
    }

    static SchedulingSolution* deserialize(int fd) {
        int numProcessors;
        read(fd, &numProcessors, sizeof(int));

        int taskTimesSize;
        read(fd, &taskTimesSize, sizeof(int));

        std::vector<int> taskTimes(taskTimesSize);
        read(fd, taskTimes.data(), taskTimesSize * sizeof(int));

        SchedulingSolution* solution = new SchedulingSolution(numProcessors, taskTimes);

        for (int i = 0; i < numProcessors; ++i) {
            int tasksSize;
            read(fd, &tasksSize, sizeof(int));

            std::vector<int> tasks(tasksSize);
            if (tasksSize > 0) {
                read(fd, tasks.data(), tasksSize * sizeof(int));
            }
            solution->schedule[i] = tasks;
        }
        return solution;
    }


    void printSchedule() const {
    std::cout << "Оптимальное распределение задач по процессорам:\n";
    for (size_t i = 0; i < schedule.size(); ++i) {
        std::cout << "Процессор " << i << ": ";
        for (int task : schedule[i]) {
            for (int j = 0; j < taskTimes[task]; ++j) {
                std::cout << task;
            }
            std::cout << " ";
        }
        std::cout << "\n";
    }
}

};

class SchedulingMutation : public Mutation {
public:
    void apply(Solution& solution) const override {
        SchedulingSolution& schedSolution = dynamic_cast<SchedulingSolution&>(solution);
        auto& schedule = schedSolution.getSchedule();

        int numProcessors = schedule.size();


        std::uniform_int_distribution<int> procDist(0, numProcessors - 1);
        int fromProc = procDist(gen); 

        if (!schedule[fromProc].empty()) {
            std::uniform_int_distribution<int> taskDist(0, schedule[fromProc].size() - 1);
            int taskIdx = taskDist(gen);
            int task = schedule[fromProc][taskIdx];
            schedule[fromProc].erase(schedule[fromProc].begin() + taskIdx);

            int toProc = procDist(gen);
            schedule[toProc].push_back(task);
        }
    }
};


SchedulingSolution* createInitialSolution(int numProcessors, const std::vector<int>& taskTimes) {
    SchedulingSolution* initialSolution = new SchedulingSolution(numProcessors, taskTimes);
    int taskCount = taskTimes.size();

    for (int i = 0; i < taskCount; ++i) {
        initialSolution->addTask(i % numProcessors, i);
    }

    return initialSolution;
}

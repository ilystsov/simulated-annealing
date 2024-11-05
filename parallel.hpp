#include <cmath>
#include <vector>
#include <algorithm>
#include <random>
#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>
#include <ctime>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "simulated_annealing.hpp"

void initialize_random_generator() {
    unsigned seed = rd() ^ static_cast<unsigned>(std::chrono::high_resolution_clock::now().time_since_epoch().count()) ^ static_cast<unsigned>(getpid());
    gen.seed(seed);
}

void runWorker(int fd, TemperatureSchedule* tempSchedule, double initialTemperature, int maxIterWithoutImprovement) {
    initialize_random_generator();
    SchedulingSolution* initialSolution = SchedulingSolution::deserialize(fd);
    SchedulingMutation mutation;
    SimulatedAnnealing sa(initialSolution, &mutation, tempSchedule, initialTemperature, maxIterWithoutImprovement);
    sa.run();
    dynamic_cast<SchedulingSolution*>(sa.getBestSolution())->serialize(fd);
    delete initialSolution;
    exit(0);
}

SchedulingSolution* run_parallel_simulated_annealing(
    const std::string& dataFilename, 
    int numProcesses, 
    TemperatureSchedule* tempSchedule,
    double initialTemperature, 
    int maxIterWithoutImprovement, 
    int maxOuterIterationsWithoutImprovement, 
    double& bestCriterion) {

    int numProcessors, numTasks;
    std::vector<int> taskTimes;
    if (!loadDataFromCSV(dataFilename, numProcessors, numTasks, taskTimes)) {
        std::cerr << "Ошибка загрузки данных из файла.\n";
        return nullptr;
    }

    SchedulingSolution* bestSolution = createInitialSolution(numProcessors, taskTimes);
    bestCriterion = bestSolution->evaluate();
    int outerIterationsWithoutImprovement = 0;

    while (outerIterationsWithoutImprovement < maxOuterIterationsWithoutImprovement) {
        std::vector<pid_t> childPIDs;
        std::vector<int> sockets;

        for (int i = 0; i < numProcesses; ++i) {
            int sv[2];
            if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) {
                perror("socketpair");
                exit(1);
            }

            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                exit(1);
            } else if (pid == 0) {
                close(sv[0]);
                runWorker(sv[1], tempSchedule, initialTemperature, maxIterWithoutImprovement);
            } else {
                close(sv[1]);
                sockets.push_back(sv[0]);
                childPIDs.push_back(pid);
                bestSolution->serialize(sv[0]);
            }
        }

        bool improvement = false;
        for (size_t i = 0; i < childPIDs.size(); ++i) {
            SchedulingSolution* childSolution = SchedulingSolution::deserialize(sockets[i]);
            double childCriterion = childSolution->evaluate();
            if (childCriterion < bestCriterion) {
                delete bestSolution;
                bestSolution = childSolution;
                bestCriterion = childCriterion;
                improvement = true;
            } else {
                delete childSolution;
            }
            close(sockets[i]);
            waitpid(childPIDs[i], nullptr, 0);
        }

        if (improvement) {
            outerIterationsWithoutImprovement = 0;
        } else {
            outerIterationsWithoutImprovement++;
        }
    }

    return bestSolution;
}

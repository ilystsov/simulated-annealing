#ifndef GENERATE_DATA_HPP
#define GENERATE_DATA_HPP

#include <random>
#include <string>
#include <iostream>
#include <fstream>

extern std::random_device rd;
extern std::mt19937 gen;


void generateDataToCSV(const std::string& filename, int numProcessors, int numTasks, int minDuration, int maxDuration) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Не удалось открыть файл для записи: " << filename << std::endl;
        return;
    }

    file << numProcessors << "," << numTasks << "\n";

    std::uniform_int_distribution<int> durationDist(minDuration, maxDuration);

    for (int i = 0; i < numTasks; ++i) {
        int duration = durationDist(gen);
        file << duration;
        if (i < numTasks - 1) file << ",";
    }
    file << "\n";
    file.close();
}

bool loadDataFromCSV(const std::string& filename, int& numProcessors, int& numTasks, std::vector<int>& taskTimes) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Не удалось открыть файл для чтения: " << filename << std::endl;
        return false;
    }

    std::string line;
    if (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string value;

        if (std::getline(ss, value, ',')) {
            numProcessors = std::stoi(value);
        }

        if (std::getline(ss, value, ',')) {
            numTasks = std::stoi(value);
        }
    }

    taskTimes.clear();
    if (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string value;
        while (std::getline(ss, value, ',')) {
            taskTimes.push_back(std::stoi(value));
        }
    }

    file.close();
    return true;
}

#endif
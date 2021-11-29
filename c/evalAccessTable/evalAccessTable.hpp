//
// Created by dev on 26.10.21.
//
#pragma once

#include <chrono>
#include <future>
#include <iostream>

#include "../algos/Generators.hpp"

#ifndef C_EVALACCESSTABLE_HPP
#define C_EVALACCESSTABLE_HPP

#endif //C_EVALACCESSTABLE_HPP

class EvalAccessTable {
public:
    EvalAccessTable(std::string  filename, std::string  out_dir, bool do_run = true);
    const std::unordered_map<RamSize, std::pair<uint, uint>>& getValues(std::string algo);
    bool hasValues(std::string algo);
    bool hasAllValues(std::string algo);
    bool hasValue(std::string algo, RamSize ramSize);
    ramListType missingValues(std::string algo);
private:
    void runFromFilename(bool only_new = false, bool ignore_old = false, bool full_run = true, bool run_slow = false);
    void printToFile();
    void getDataFile();
    void createLists(bool ignore_last_run);

    const std::string filename;
    const std::string output_dir;
    const std::string output_file = output_dir + "output.csv";
    std::vector<Access> data;
    const std::string first_csv_line = "algo,ramSize,elements,reads,writes";
    ramListType ramSizes;
    rwListType read_write_list;
    void handleCsv(std::ifstream &filestream);
public:
    void init(bool ignore_last_run);
    template<class T>
    void runAlgorithmNonParallel(const std::string &name, T executor) {
        if (!hasAllValues(name)) {
            std::cout << name << std::endl;
            auto t1 = std::chrono::high_resolution_clock::now();
            executor.evaluateRamList(data, ramSizes, read_write_list[name]);
            printToFile();

            auto t2 = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> seconds_double = t2 - t1;
            std::cout << seconds_double.count() << " seconds" << std::endl;

        }
    }

    template<class T>
    void runAlgorithm(const std::string &name, std::function<T()> generator) {
        if (!hasAllValues(name)) {
            // PRE
            std::cout << name << std::endl;
            auto t1 = std::chrono::high_resolution_clock::now();

            // RUN
            runParallelEvictStrategy(read_write_list[name], generator, missingValues(name));

            // POST
            printToFile();
            auto t2 = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> seconds_double = t2 - t1;
            std::cout << seconds_double.count() << " seconds" << std::endl;

        }
    }

private:
    template<class T>
    void
    runParallelEvictStrategy(rwListSubType &rwList, std::function<T()> generator, ramListType missing) {

        std::unordered_map<RamSize, std::future<std::pair<uint, uint>>> pairs;
        std::vector<Access>& datacopy = data;
        auto evalFunc = [generator, &datacopy](RamSize ram_size) {
            return generator().evaluateOne(datacopy, ram_size);
        };
        std::for_each(
                missing.begin(),
                missing.end(),
                [&](uint ram_size) {
                    pairs[ram_size] = std::async(evalFunc, ram_size);
                }
        );
        std::for_each(
                pairs.begin(),
                pairs.end(),
                [&](std::pair<const RamSize, std::future<std::pair<uint, uint>>> &futPair) {
                    futPair.second.wait();
                    auto pair = futPair.second.get();
                    rwList[futPair.first] = pair;
                }
        );

    }
};
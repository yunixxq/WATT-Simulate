//
// Created by dev on 26.10.21.
//
#pragma once

#include <chrono>
#include <future>
#include <map>
#include <vector>
#include <iostream>
#include "general.hpp"
#include "../algos/EvictStrategy.hpp"

#ifndef C_EVALACCESSTABLE_HPP
#define C_EVALACCESSTABLE_HPP

#endif //C_EVALACCESSTABLE_HPP

class EvalAccessTable {
    const std::string filename;
    const std::string output_dir;
    const std::string read_file = output_dir + "reads.csv";
    const std::string write_file = output_dir + "writes.csv";
    std::map<std::string, std::vector<uInt>> y_read_list, y_write_list;
    std::vector<Access> data;
public:
    EvalAccessTable(std::string  filename, std::string  out_dir, bool do_run = true);
    const std::vector<uInt> getReads(std::string);
    const std::vector<uInt> getWrites(std::string);
private:
    void runFromFilename(bool only_new = false, bool ignore_old = false, bool full_run = true, bool run_slow = false);
    void printAlgosToFile(const std::string file, std::map<std::string, std::vector<uInt>> &algo_entries);
    void printToFile();
    void getDataFile();
    void createLists(bool ignore_last_run);
public:
    void init(bool ignore_last_run);
    template<class executor>
    void runAlgorithm(const std::string &name, std::vector<int> args = {}) {
        if (y_read_list.find(name) == y_read_list.end()) {
            std::cout << name << std::endl;
            auto t1 = std::chrono::high_resolution_clock::now();
            if constexpr(std::is_base_of<EvictStrategy, executor>::value) {
                std::vector<Access>& datacopy = data;
                auto evalFunc = [args, &datacopy](RamSize ram_size) {
                    return executor(args).evaluateOne(datacopy, ram_size);

                };
                runParallelEvictStrategy(y_read_list["X"], y_read_list[name], y_write_list[name], evalFunc);
            } else {
                executor(args).evaluateRamList(data, y_read_list["X"], y_read_list[name], y_write_list[name]);
            }
            printToFile();

            auto t2 = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> seconds_double = t2 - t1;
            std::cout << seconds_double.count() << " seconds" << std::endl;

        }
    }

private:
    static void
    runParallelEvictStrategy(std::vector<RamSize> &x_list, std::vector<uInt> &read_list,
                             std::vector<uInt> &write_list, auto& function) {

        std::vector<std::future<std::pair<uInt, uInt>>> pairs;
        for_each(
                x_list.begin(),
                x_list.end(),
                [&](uInt ram_size) {
                    pairs.push_back(std::async(function, ram_size));
                }
        );
        for_each(
                pairs.begin(),
                pairs.end(),
                [&](std::future<std::pair<uInt, uInt>> &futPair) {
                    futPair.wait();
                    auto pair = futPair.get();
                    read_list.push_back(pair.first);
                    write_list.push_back(pair.second);
                }
        );

    }
};
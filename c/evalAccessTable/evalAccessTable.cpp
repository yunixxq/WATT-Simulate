#include <string>
#include <utility>
#include <vector>
#include <fstream>
#include <iostream>
#include <cassert>
#include <map>
#include <unordered_map>
#include <filesystem>
#include "../algos/lruStackDist.hpp"
#include "../algos/staticOpt.hpp"
#include "../algos/random.hpp"
#include "../algos/lru.hpp"
#include "../algos/opt.hpp"
#include "../algos/cf_lru.hpp"
#include "../algos/lru_wsr.hpp"
#include "../algos/lru_k.hpp"
#include "../algos/lfu_k.hpp"

#include "evalAccessTable.hpp"

using namespace std;

//
// Created by dev on 05.10.21.
//
static void handleCsv(vector<string> &names, map<string, vector<uInt>> &y_list, ifstream &filestream);
static void getOrDefaultAndSet(map<int, int> &history, int new_value, int pageRef,
                               int default_value, int* value);

EvalAccessTable::EvalAccessTable(std::string  filename, std::string  out_dir, bool do_run): filename(std::move(filename)), output_dir(std::move(out_dir)){
        if(!do_run){
            return;
        }
        bool test = false;
        bool full_run = false;
        if(test) {
            runFromFilename(true, true);
        }else if (full_run){
            runFromFilename(false, true);
        }else{
            runFromFilename(false, false, false, false);
        }

    };

void EvalAccessTable::init(bool ignore_last_run){
    getDataFile();
    createLists(ignore_last_run); // this runs "lru" (lru_stack_trace)
}
void EvalAccessTable::runFromFilename(bool only_new, bool ignore_old, bool full_run, bool run_slow) {
    init(ignore_old);
    runAlgorithm<OPT>("opt");
    runAlgorithm<StaticOpt>("StaticOpt");
    if(!only_new) {
        runAlgorithm<Random>("random");
        runAlgorithm<OPT>("opt");
        if (full_run) {
            if(run_slow){
                runAlgorithm<OPT2>("opt2");
            }
            runAlgorithm<OPT3>("opt3");
            runAlgorithm<LRU>("lru_alt");
            runAlgorithm<LRU1>("lru_alt1");
            if(run_slow){
                runAlgorithm<LRU2>("lru_alt2");
            }
            runAlgorithm<LRU2a>("lru_alt2a");
            runAlgorithm<LRU2b>("lru_alt2b");
            runAlgorithm<LRU3>("lru_alt3");
            runAlgorithm<CF_LRU<10>>("cf_lru10");
            runAlgorithm<CF_LRU<20>>("cf_lru20");
            runAlgorithm<CF_LRU<70>>("cf_lru70");
            runAlgorithm<CF_LRU<80>>("cf_lru80");
            runAlgorithm<CF_LRU<90>>("cf_lru90");
            if(run_slow){
                runAlgorithm<CF_LRU<100>>("cf_lru100");
            }
            runAlgorithm<LFU_K_alt<>>("lfu_k_alt01", {1});
            runAlgorithm<LFU_K_alt<>>("lfu_k_alt02", {2});
            runAlgorithm<LFU_K_alt<>>("lfu_k_alt10", {10});
            runAlgorithm<LFU_K_alt<>>("lfu_k_alt20", {20});
            runAlgorithm<LRU_K_alt>("lru_k_alt01", {1});
            runAlgorithm<LRU_K_alt>("lru_k_alt02", {2});
            runAlgorithm<LRU_K_alt>("lru_k_alt10", {10});
            runAlgorithm<LRU_K_alt>("lru_k_alt20", {20});
        }
        runAlgorithm<CF_LRU<30>>("cf_lru30");
        runAlgorithm<CF_LRU<40>>("cf_lru40");
        runAlgorithm<CF_LRU<50>>("cf_lru50");
        runAlgorithm<CF_LRU<60>>("cf_lru60");
        runAlgorithm<LRU_WSR>("lru_wsr");

    }
    
    for(int k: {1,2,10,20}){
        runAlgorithm<LFU_K<>>("lfu_k_" + std::to_string(k), {k});
        for(int z: {-20, -10, -2, 1, 10, 20, 100}){
            runAlgorithm<LFU_K_Z<>>("lfu_k" + std::to_string(k) + "_z" + std::to_string(z), {k, z});
            runAlgorithm<LFU_K_Z2<>>("lfu2_k" + std::to_string(k) + "_z" + std::to_string(z), {k, z});
            runAlgorithm<LRU_K_Z>("lru_k" + std::to_string(k) + "_z" + std::to_string(z), {k, z});

        }
    }

    printToFile();
}

void EvalAccessTable::printToFile() {
    printAlgosToFile(read_file, y_read_list);
    printAlgosToFile(write_file, y_write_list);
}

void EvalAccessTable::printAlgosToFile(const string file, map<string, vector<uInt>> &algo_entries) {
    vector<string> names;
    for (auto &entry: algo_entries) {
        if (entry.first != "X") {
            names.push_back(entry.first);
        }
    }
    ofstream out_stream;
    out_stream.open(file);
    // print algo names
    out_stream << "X,elements";
    for (auto &name: names) {
        out_stream << "," << name;
    }
    out_stream << endl;
    // print entries
    for (uInt i = 0; i < y_read_list["X"].size(); i++) {
        out_stream << y_read_list["X"][i] << "," << data.size();
        for (auto &name: names) {
            out_stream << "," << algo_entries[name][i];
        }
        if (i != y_read_list["X"].size() - 1) {
            out_stream << endl;
        }
    }
}

void EvalAccessTable::getDataFile() {
    {
        ifstream reader;
        reader.open(filename);
        string line;
        bool firstLine = true;
        map<int, int> last_access;
        int i = 0;
        while (getline(reader, line)) {
            if (firstLine) {
                firstLine = false;
                assert("pages,is_write" == line);
                continue;
            }
            Access &access = data.emplace_back();
            stringstream ss(line);
            string value;
            getline(ss, value, ',');
            access.pageRef = stoi(value);
            getline(ss, value);
            access.write = (value.find("rue") != std::string::npos);
            access.pos = i;

            getOrDefaultAndSet(last_access, i, access.pageRef, -1, &access.lastRef);

            i++;
        }
    }
    {
        map<int, int> next_access;
        int data_size = data.size();
        for (uInt i = 0; i < data.size(); i++) {
            int pos = data_size - (i + 1);
            getOrDefaultAndSet(next_access, pos, data[pos].pageRef, data_size, &data[pos].nextRef);
        }
    }
}

void EvalAccessTable::createLists(bool ignore_last_run) {
    vector<string> r_names, w_names;
    ifstream reader, writer;
    reader.open(read_file);
    writer.open(write_file);
    if (reader.good() && writer.good() && !ignore_last_run) {
        handleCsv(r_names, y_read_list, reader);
        handleCsv(w_names, y_write_list, writer);
        assert(is_permutation(w_names.begin(), w_names.end(), r_names.begin(), r_names.end()));
        assert(y_read_list["elements"][0] == data.size());
        assert(y_write_list["elements"][0] == data.size());
        y_read_list.erase("elements");
        y_write_list.erase("elements");
        y_write_list.erase("X");
    } else {
        cout << "No old files found" << endl;
        filesystem::create_directory(output_dir);
    }
    runAlgorithm<LruStackDist>("lru");
}

const std::vector<uInt> EvalAccessTable::getReads(std::string name) {
    return y_read_list[name];
}
const std::vector<uInt> EvalAccessTable::getWrites(std::string name) {
    return y_write_list[name];
}

static void handleCsv(vector<string> &names, map<string, vector<uInt>> &y_list, ifstream &filestream) {
    string line;
    bool first_line = true;
    while (getline(filestream, line)) {
        stringstream ss(line);
        string element;
        int pos = 0;
        while (getline(ss, element, ',')) {
            if (first_line) {
                if(find(names.begin(), names.end(), element) != names.end()){
                    cout << "DUPLICATE NAME" <<endl;
                    element+=".1";
                }
                names.push_back(element);
                y_list[element];
            } else {
                auto pointer = y_list.find(names[pos]);
                assert(pointer != y_list.end());
                pointer->second.push_back(stoi(element));
            }
            pos++;
        }
        first_line = false;
    }
}


static void getOrDefaultAndSet(map<int, int> &history, int new_value, int pageRef,
                        int default_value, int* value) {
    auto element = history.find(pageRef);
    if (element == history.end()) {
        *value = default_value;
    } else {
        *value = element->second;
    }
    history[pageRef] = new_value;
}

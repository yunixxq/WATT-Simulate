#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <cassert>
#include <map>
#include <unordered_map>
#include <filesystem>
#include <future>
#include "../algos/lruStackDist.hpp"
#include "../algos/random.hpp"
#include "../algos/lru.hpp"
#include "../algos/opt.hpp"
#include "../algos/cf_lru.hpp"
#include "../algos/lru_wsr.hpp"
#include "../algos/lru_k.hpp"
#include "../algos/lfu_k.hpp"

using namespace std;

//
// Created by dev on 05.10.21.
//

class EvalAccessTable {
    const std::string filename;
    const string output_dir;
    const string read_file = output_dir + "reads.csv";
    const string write_file = output_dir + "writes.csv";
    map<string, vector<uInt>> y_read_list, y_write_list;
    vector<Access> data;


public:
    EvalAccessTable(const std::string filename, const std::string out_dir): filename(filename), output_dir(out_dir){
        runFromFilename();
    };
private:
    void runFromFilename() {
        bool only_new=false;
        bool ignore_old=false;
        bool full_run=true;
        getDataFile();
        createLists(ignore_old); // this runs "lru" (lru_stack_trace)
        if(!only_new) {
            runAlgorithm<Random>("random");
            runAlgorithm<OPT>("opt");
            if (full_run) {
                runAlgorithm<OPT2>("opt2");
                runAlgorithm<OPT3>("opt3");
                runAlgorithm<LRU>("lru_alt");
                runAlgorithm<LRU1>("lru_alt1");
                runAlgorithm<LRU2>("lru_alt2");
                runAlgorithm<LRU2a>("lru_alt2a");
                runAlgorithm<LRU2b>("lru_alt2b");
                runAlgorithm<LRU3>("lru_alt3");
                runAlgorithm<CF_LRU<10>>("cf_lru10");
                runAlgorithm<CF_LRU<20>>("cf_lru20");
                runAlgorithm<CF_LRU<70>>("cf_lru70");
                runAlgorithm<CF_LRU<80>>("cf_lru80");
                runAlgorithm<CF_LRU<90>>("cf_lru90");
                runAlgorithm<CF_LRU<100>>("cf_lru100");
            }
            runAlgorithm<CF_LRU<30>>("cf_lru30");
            runAlgorithm<CF_LRU<40>>("cf_lru40");
            runAlgorithm<CF_LRU<50>>("cf_lru50");
            runAlgorithm<CF_LRU<60>>("cf_lru60");
            runAlgorithm<LRU_WSR>("lru_wsr");

            runAlgorithm<LFU_K_ALL<1>>("lfu_k_01");
            runAlgorithm<LFU_K_ALL<2>>("lfu_k_02");
            runAlgorithm<LFU_K_ALL<10>>("lfu_k_10");
            runAlgorithm<LFU_K_ALL<20>>("lfu_k_20");
        }
        printToFile();
    }

    void printToFile() {
        printAlgosToFile(read_file, y_read_list);
        printAlgosToFile(write_file, y_write_list);
    }

    template<class  executor>
    void runAlgorithm(const string &name) {
        if (y_read_list.find(name) == y_read_list.end()) {
            std::cout << name << std::endl;
            auto t1 = std::chrono::high_resolution_clock::now();
            if constexpr(std::is_base_of<EvictStrategy, executor>::value) {
                runParallelEvictStrategy<executor>(data, y_read_list["X"], y_read_list[name], y_write_list[name]);
            } else{
                executor().evaluateRamList(data,y_read_list["X"], y_read_list[name], y_write_list[name]);
            }
            printToFile();

            auto t2 = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> seconds_double = t2 - t1;
            std::cout << seconds_double.count() << " seconds" << std::endl;

        }
    }

    template<class executor>
    static void runParallelEvictStrategy(std::vector<Access> &data, std::vector<RamSize> &x_list, std::vector<uInt> &read_list,
                                  std::vector<uInt> &write_list){
        static_assert(std::is_base_of<EvictStrategy, executor>::value);
        bool parallel=true;
        if(!parallel){
            executor().evaluateRamList(data, x_list, read_list, write_list);
        }else{
            std::vector<std::future<pair<uInt, uInt>>> pairs;
            for_each(
                    x_list.begin(),
                    x_list.end(),
                    [&](uInt ram_size){
                        pairs.push_back(async([ram_size, &data](){
                            pair<uInt, uInt> pair = executor().evaluateOne(data, ram_size);
                            return pair;
                        }));
                    }
                    );
            for_each(
                    pairs.begin(),
                    pairs.end(),
                    [&](std::future<pair<uInt, uInt>>& futPair){
                        futPair.wait();
                        auto pair = futPair.get();
                        read_list.push_back(pair.first);
                        write_list.push_back(pair.second);
                    }
                    );
        }

    }

    void printAlgosToFile(const string &file, map<string, vector<uInt>> &algo_entries) {
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

    void getDataFile() {
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

    void createLists(bool ignore_last_run) {
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

};

int main(int, char **) {
    EvalAccessTable eval("./tpcc_64_-5.csv", "./out/");
    return 0;
}

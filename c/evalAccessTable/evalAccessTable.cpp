#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <cassert>
#include <unordered_map>
#include <filesystem>
#include "../algos/lruStackDist.hpp"
#include "../algos/random.hpp"
#include "../algos/lru.hpp"
#include "../algos/opt.hpp"
#include "../algos/cf_lru.hpp"

using namespace std;

//
// Created by dev on 05.10.21.
//

class EvalAccessTable {
    const std::string filename;
    const string output_dir;
    const string read_file = output_dir + "reads.csv";
    const string write_file = output_dir + "writes.csv";
    unordered_map<string, vector<int>> y_read_list, y_write_list;
    vector<Access> data;


public:
    EvalAccessTable(const std::string filename, const std::string out_dir): filename(filename), output_dir(out_dir){
        runFromFilename();
    };
private:
    void runFromFilename() {
        getDataFile();
        createLists(); // this runs "lru" (lru_stack_trace)
        runAlgorithm<Random>("random");
        runAlgorithm<OPT>("opt");
        runAlgorithm<OPT2>("opt2");
        runAlgorithm<OPT3>("opt3");
        runAlgorithm<LRU>("lru_alt1");
        runAlgorithm<LRU2>("lru_alt2");
        runAlgorithm<LRU3>("lru_alt3");
        // runAlgorithm<CF_LRU<50>>("cf_lru");
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

            executor().evaluateRamList(data,y_read_list["X"], y_read_list[name], y_write_list[name]);
            printToFile();

            auto t2 = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> seconds_double = t2 - t1;
            std::cout << seconds_double.count() << " seconds" << std::endl;

        }
    }

    void printAlgosToFile(const string &file, unordered_map<string, vector<int>> &algo_entries) {
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
        for (int i = 0; i < y_read_list["X"].size(); i++) {
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
            unordered_map<int, int> last_access;
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
            unordered_map<int, int> next_access;
            int data_size = data.size();
            for (int i = 0; i < data.size(); i++) {
                int pos = data_size - (i + 1);
                getOrDefaultAndSet(next_access, pos, data[pos].pageRef, data_size, &data[pos].nextRef);
            }
        }
    }

    void createLists() {
        vector<string> r_names, w_names;
        ifstream reader, writer;
        reader.open(read_file);
        writer.open(write_file);
        if (reader.good() && writer.good()) {
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

    static void handleCsv(vector<string> &names, unordered_map<string, vector<int>> &y_list, ifstream &filestream) {
        string line;
        bool first_line = true;
        while (getline(filestream, line)) {
            stringstream ss(line);
            string element;
            int pos = 0;
            while (getline(ss, element, ',')) {
                if (first_line) {
                    assert(find(names.begin(), names.end(), element) == names.end());
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


    static void getOrDefaultAndSet(unordered_map<int, int> &history, int new_value, int pageRef,
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

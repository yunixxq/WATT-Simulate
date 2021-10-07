#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <cassert>
#include <unordered_map>
#include <filesystem>
#include "../algos/lruStackDist.hpp"
#include "../algos/random.hpp"

using namespace std;

//
// Created by dev on 05.10.21.
//

class EvalAccessTable {
    const std::string filename;
    const string output_dir;
    const string read_file = output_dir + "reads.csv";
    const string write_file = output_dir + "writes.csv";
    unordered_map<string, vector<unsigned int>> y_read_list, y_write_list;
    vector<Access> data;


public:
    EvalAccessTable(const std::string filename, const std::string out_dir): filename(filename), output_dir(out_dir){
        runFromFilename();
    };
private:
    void runFromFilename() {
        getDataFile();
        createLists();
        runAlgorithm("lru", lruStackDist);
        runAlgorithm("random", executeStrategy<Random>);
        printToFile();
    }

    void printToFile() {
        printAlgosToFile(read_file, y_read_list);
        printAlgosToFile(write_file, y_write_list);
    }

    void runAlgorithm(const string &name, auto& executor) {
        if (y_read_list.find(name) == y_read_list.end()) {
            executor(data, name, y_read_list["X"], y_read_list[name], y_write_list[name]);
            printToFile();
        }
    }

    void printAlgosToFile(const string &file, unordered_map<string, vector<unsigned int>> &algo_entries) {
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
            unordered_map<unsigned int, unsigned int> last_access;
            unsigned int i = 0;
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

                getOrDefaultAndSet(last_access, i, access, 0);

                i++;
            }
        }
        {
            unordered_map<unsigned int, unsigned int> next_access;
            unsigned int data_size = data.size();
            for (unsigned int i = 0; i < data.size(); i++) {
                getOrDefaultAndSet(next_access, data_size - (i + 1), data[i], data_size);
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
    }

    void handleCsv(vector<string> &names, unordered_map<string, vector<unsigned int>> &y_list, ifstream &filestream) {
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


    void getOrDefaultAndSet(unordered_map<unsigned int, unsigned int> &history, unsigned int new_value, Access &access,
                            unsigned int default_value) {
        auto element = history.find(access.pageRef);
        if (element == history.end()) {
            access.lastRef = default_value;
        } else {
            access.lastRef = element->second;
        }
        history[access.pageRef] = new_value;
    }

};

int main(int, char **) {
    EvalAccessTable eval("./tpcc_64_-5.csv", "./out/");
    return 0;
}

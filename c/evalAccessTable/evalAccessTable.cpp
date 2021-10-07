#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <cassert>
#include <unordered_map>
#include <filesystem>
#include "../algos/lruStackDist.hpp"
#include "general.hpp"
#include "../algos/random.hpp"

using namespace std;

//
// Created by dev on 05.10.21.
//

void runFromFilename(const std::string& filename);

void getOrDefaultAndSet(unordered_map<unsigned int, unsigned int> &history, unsigned int new_value, Access &access,
                        unsigned int default_value);

void handleCsv(vector<string> &names, unordered_map<string, vector<unsigned int>> &y_list, ifstream &filestream);

void createLists(const string &output_dir, const string &read_file, const string &write_file,
                 unordered_map<string, vector<unsigned int>> &y_read_list,
                 unordered_map<string, vector<unsigned int>> &y_write_list,
                 unsigned int elements);

void getDataFile(const string &filename, vector<Access> &data);

void printAlgosToFile(const string& file, const vector<unsigned int> &x_list, unsigned int elements,
                      unordered_map<string, vector<unsigned int>> &algo_entries);

int main(int , char** )
{
    std::string filename = "./tpcc_64_-5.csv";
    runFromFilename(filename);
    return 0;
}

void runFromFilename(const std::string& filename){
    vector<Access> data;
    getDataFile(filename, data);
    string output_dir = "./out/";
    string read_file = output_dir + "reads.csv";
    string write_file = output_dir + "writes.csv";

    unordered_map<string ,vector<unsigned int>> y_read_list, y_write_list;
    createLists(output_dir, read_file, write_file, y_read_list, y_write_list, data.size());
    if(y_read_list.find("lru") == y_read_list.end()){
        lruStackDist(data, y_read_list["X"], y_read_list["lru"], y_write_list["lru"]);
        printAlgosToFile(read_file, y_read_list["X"], data.size(), y_read_list);
        printAlgosToFile(write_file, y_read_list["X"], data.size(), y_write_list);
    }
    string name = "random";
    if(y_read_list.find(name) == y_read_list.end()){
        auto& x_list = y_read_list["X"];
        auto& read_list = y_read_list[name];
        auto& write_list = y_write_list[name];
        std::cout << name << std::endl;
        auto t1 = std::chrono::high_resolution_clock::now();
        for(auto& ram_size: y_read_list["X"]){
            Random rand(ram_size);
            auto pair = rand.executeStrategy(data);
            read_list.push_back(pair.first);
            write_list.push_back(pair.second);
            }
        printAlgosToFile(read_file, y_read_list["X"], data.size(), y_read_list);
        printAlgosToFile(write_file, y_read_list["X"], data.size(), y_write_list);
        auto t2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> seconds_double = t2 - t1;
        std::cout << seconds_double.count() << " seconds" <<std:: endl;

    }
    printAlgosToFile(read_file, y_read_list["X"], data.size(), y_read_list);
    printAlgosToFile(write_file, y_read_list["X"], data.size(), y_write_list);

    assert(!y_read_list["X"].empty());
}

void printAlgosToFile(const string& file, const vector<unsigned int> &x_list, unsigned int elements,
                      unordered_map<string, vector<unsigned int>> &algo_entries) {
    vector<string> names;
    for(auto& entry: algo_entries){
        if (entry.first!= "X") {
            names.push_back(entry.first);
        }
    }
    ofstream out_stream;
    out_stream.open(file);
    // print algo names
    out_stream << "X,elements";
    for(auto& name: names){
        out_stream << "," << name;
    }
    out_stream << endl;
    // print entries
    for(int i=0; i< x_list.size(); i++){
        out_stream << x_list[i] << "," << elements;
        for (auto& name: names){
            out_stream << "," << algo_entries[name][i];
        }
        if(i!= x_list.size()-1){
            out_stream << endl;
        }
    }
}

void getDataFile(const string &filename, vector<Access> &data) {
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
            access.pos=i;

            getOrDefaultAndSet(last_access, i, access, 0);

            i++;
        }
    }
    {
        unordered_map<unsigned int, unsigned int> next_access;
        for (int i = data.size()-1 ; i >= 0; i--) {
            getOrDefaultAndSet(next_access, i, data[i], data.size());
        }
    }
}

void createLists(const string &output_dir, const string &read_file, const string &write_file,
                 unordered_map<string, vector<unsigned int>> &y_read_list,
                 unordered_map<string, vector<unsigned int>> &y_write_list,
                 const unsigned int elements) {
                     vector<string> r_names, w_names;
                     ifstream reader, writer;
                     reader.open(read_file);
                     writer.open(write_file);
                     if(reader.good() && writer.good()){
                         handleCsv(r_names, y_read_list, reader);
                         handleCsv(w_names, y_write_list, writer);
                         assert(is_permutation(w_names.begin(), w_names.end(), r_names.begin(), r_names.end()));
                         assert(y_read_list["elements"][0] == elements);
                         assert(y_write_list["elements"][0] == elements);
                         y_read_list.erase("elements");
                         y_write_list.erase("elements");
                         y_write_list.erase("X");
                     }else{
                         cout << "No old files found" <<endl;
                         filesystem::create_directory(output_dir);
                     }
                 }

void handleCsv(vector<string> &names, unordered_map<string, vector<unsigned int>> &y_list, ifstream &filestream) {
    string line;
    bool first_line = true;
    while (getline(filestream, line)){
        stringstream ss(line);
        string element;
        int pos =0;
        while(getline(ss, element, ',')){
            if(first_line){
                assert(find(names.begin(), names.end(), element) == names.end());
                names.push_back(element);
                y_list[element];
            }else{
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

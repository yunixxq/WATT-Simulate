#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <cassert>
#include <unordered_map>
#include <filesystem>

using namespace std;

//
// Created by dev on 05.10.21.
//

struct Access {
    unsigned int pageRef;
    unsigned int nextRef;
    unsigned int lastRef;
    bool write;
};
void runFromFilename(const std::string& filename);

void getOrDefaultAndSet(unordered_map<unsigned int, unsigned int> &history, unsigned int new_value, Access &access,
                        unsigned int default_value);

void handleCsv(vector<string> &names, unordered_map<string, vector<unsigned int>> &y_list, ifstream &filestream);

void createLists(const string &output_dir, const string &read_file, const string &write_file,
                 unordered_map<string, vector<unsigned int>> &y_read_list,
                 unordered_map<string, vector<unsigned int>> &y_write_list,
                 unsigned int elements);

void getDataFile(const string &filename, vector<Access> &data);

void lruStackDist(vector<Access> &data, vector<unsigned int> &x_list, vector<unsigned int> &read_list,
                  vector<unsigned int> &write_list);

void printAlgosToFile(const string file, const vector<unsigned int> &x_list, unsigned int elements,
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
    }
    printAlgosToFile(read_file, y_read_list["X"], data.size(), y_read_list);
    printAlgosToFile(write_file, y_read_list["X"], data.size(), y_write_list);

    assert(y_read_list["X"].size() >0);
}

void printAlgosToFile(const string file, const vector<unsigned int> &x_list, unsigned int elements,
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

void lruStackDist(vector<Access> &data, vector<unsigned int> &x_list, vector<unsigned int> &read_list,
                  vector<unsigned int> &write_list) {
    assert(x_list.size() == read_list.size() && x_list.size() == write_list.size());
    vector<int> lruStack, lruStackDist, lru_stack_dirty, dirty_depth;

    for(auto& access: data){
        // read
        // find pos;
        auto elem = find(lruStack.begin(), lruStack.end(), access.pageRef);
        int pos = 0;
        if (elem != lruStack.end()){
            pos = elem - lruStack.begin() + 1;
            lruStack.erase(elem);
        }
        // put to pos 0;
        lruStack.emplace(lruStack.begin(), access.pageRef);
        if(lruStackDist.size()  <= pos){
            lruStackDist.resize(pos+1, 0);
            lru_stack_dirty.resize(pos+1, 0);
        }
        lruStackDist[pos] ++;
        // write
        // Idea:
        // Every written page has to be written out once.
        // it has only to be written out, if it moved down the stack to mutch in between
        // so if maxdepth between two writes is to big, it adds one write
        // se we have an extra counter, to mark, where the page went down in stack since last write.
        // when write happens, we mark the last position
        // in the End: we have to write all out => add a constant
        if (dirty_depth.size() <= access.pageRef){
            dirty_depth.resize(access.pageRef+1, 0); // clean
        }
        int prev_dirty_depth = dirty_depth[access.pageRef];
        // is fresh write? => init
        // has no prev dirty_depth: keep 0;
        // else: move down in stack;
        dirty_depth[access.pageRef] = access.write? 1 : (prev_dirty_depth == 0 ? 0 : max(pos, prev_dirty_depth));
        if (access.write) {
            if (prev_dirty_depth!= 0){
                lru_stack_dirty[max(prev_dirty_depth, pos)]++;
            }
            else{
                lru_stack_dirty[0]++; // increase constant;
            }
        }
    }

    unsigned int pages = lruStackDist[0];
    unsigned int ram_size = 2;
    do{
        if(ram_size < 20){
            ram_size ++;
        }else if(ram_size < 100){
            ram_size += 10;
        }else if(ram_size < 1000){
            ram_size += 100;
        }else if(ram_size < 10000){
            ram_size += 1000;
        }else if(ram_size < 100000){
            ram_size += 10000;
        }else{
            ram_size += 100000;
        }
        x_list.push_back(ram_size);

    }while(ram_size < pages);
    // sum it up, buttercup!
    for(auto& ram_size : x_list){
        unsigned int misses = 0, evicts =0;
        for(int i=0; i< lruStackDist.size(); i++){
            if (i > ram_size || i <= 0) {
                misses += lruStackDist[i];
            }
        }
        for(int i=0; i< lru_stack_dirty.size(); i++){
            if (i > ram_size || i <= 0) {
                evicts += lru_stack_dirty[i];
            }
        }
        read_list.push_back(misses);
        write_list.push_back(evicts);

    }
    assert(x_list.size() == read_list.size() && x_list.size() == write_list.size());
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

#include <string>
#include <utility>
#include <fstream>
#include <filesystem>
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
    // full_run runs not usefull + duplicates
    init(ignore_old);
    runAlgorithmNonParallel("StaticOpt", StaticOpt());
    runAlgorithm("opt", Opt_Generator());

    for(int k: {20, 2, 1}){
        runAlgorithm("lfu_k_" + std::to_string(k), LFU_K_Generator(k));
        for(int z: {100, 10, 1}){
            runAlgorithm("lfu_k" + std::to_string(k) + "_z" + std::to_string(z), LFU_K_Z_Generator(k, z));
            // runAlgorithm("lfu2_k" + std::to_string(k) + "_z" + std::to_string(z), LFU2_K_Z_Generator(k, z));
            runAlgorithm("lru_k" + std::to_string(k) + "_z" + std::to_string(z), LRU_K_Z_Generator(k, z));
            runAlgorithm("lfu_2k" + std::to_string(k) + "_z" + std::to_string(z) + "T", LRU_2K_Z_Generator(k, k, z, true));
            runAlgorithm("lfu_2k" + std::to_string(k) + "_z" + std::to_string(z) + "F", LRU_2K_Z_Generator(k, k, z, false));
        }
    }

    if(!only_new) {
        runAlgorithm("random", Random_Generator());
        runAlgorithm("cf_lru30", CfLRUGenerator(30));
        runAlgorithm("cf_lru40", CfLRUGenerator(40));
        runAlgorithm("cf_lru50", CfLRUGenerator(50));
        runAlgorithm("cf_lru60", CfLRUGenerator(60));
        runAlgorithm("lru_wsr", LRU_WSR_Generator());
        if (full_run) {
            runAlgorithm("lru_alt", LRU_Generator());
            runAlgorithm("lru_alt1", LRU1_Generator());
            runAlgorithm("lru_alt2a", LRU2a_Generator());
            runAlgorithm("lru_alt2b", LRU2b_Generator());
            runAlgorithm("lru_alt3", LRU3_Generator());
            runAlgorithm("cf_lru10", CfLRUGenerator(10));
            runAlgorithm("cf_lru20", CfLRUGenerator(20));
            runAlgorithm("cf_lru70", CfLRUGenerator(70));
            runAlgorithm("cf_lru80", CfLRUGenerator(80));
            runAlgorithm("cf_lru90", CfLRUGenerator(90));
            runAlgorithm("lfu_k_alt01", LFUalt_K_Generator(1));
            runAlgorithm("lfu_k_alt02", LFUalt_K_Generator(2));
            runAlgorithm("lfu_k_alt10", LFUalt_K_Generator(10));
            runAlgorithm("lfu_k_alt20", LFUalt_K_Generator(20));
            runAlgorithm("lru_k_alt01", LRUalt_K_Generator(1));
            runAlgorithm("lru_k_alt02", LRUalt_K_Generator(2));
            runAlgorithm("lru_k_alt10", LRUalt_K_Generator(10));
            runAlgorithm("lru_k_alt20", LRUalt_K_Generator(20));
            // runAlgorithm("opt3", Opt3_Generator());// broken
            if(run_slow){
                runAlgorithm("opt2", Opt2_Generator());
                runAlgorithm("lru_alt2", LRU2_Generator());
                runAlgorithm("cf_lru100", CfLRUGenerator(100));
            }
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
    runAlgorithmNonParallel("lru", LruStackDist());
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

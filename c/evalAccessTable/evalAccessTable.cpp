#include <string>
#include <utility>
#include <fstream>
#include <filesystem>
#include "evalAccessTable.hpp"

using namespace std;

//
// Created by dev on 05.10.21.
//
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
    runAlgorithm("lean20", Lean_Generator(20));
    runAlgorithm("lfu_k_real", LRU_2K_Z_real_Generator(8, 4, 1000000, 5, 5, true, 100000));
    runAlgorithm("lfu_k_real_e1", LRU_2K_Z_real_Generator(8, 4, 1000000, 5, 5, true, 1));

    for(int k: {1000, 100, 20, 2, 1}){
        for(int z: {1000, 100, 10, 1}){
            runAlgorithm("lfu_k" + std::to_string(k) + "_z" + std::to_string(z), LFU_K_Z_Generator(k, z));
            // runAlgorithm("lfu2_k" + std::to_string(k) + "_z" + std::to_string(z), LFU2_K_Z_Generator(k, z));
            runAlgorithm("lru_k" + std::to_string(k) + "_z" + std::to_string(z), LRU_K_Z_Generator(k, z));
            runAlgorithm("lfu_2k" + std::to_string(k) + "_z" + std::to_string(z) + "T", LRU_2K_Z_Generator(k, k, z, true));
            runAlgorithm("lfu_2k" + std::to_string(k) + "_z" + std::to_string(z) + "F", LRU_2K_Z_Generator(k, k, z, false));
            runAlgorithm("lfu_2k" + std::to_string(k) + "_z" + std::to_string(z) + "R", LRU_2K_Z_rand_Generator(k, k, z, 5,true));
        }
        runAlgorithm("lfu_k_" + std::to_string(k), LFU_K_Generator(k));
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
    vector<string> names;

    ofstream out_stream;
    out_stream.open(output_file);
    // print algo names
    out_stream << first_csv_line <<endl;
    // print entries
    uint elements = data.size();
    for (auto &algo: read_write_list) {
        for(auto& entry: algo.second){
            out_stream <<
                algo.first << ", " <<
                entry.first << ", " <<
                elements << ", " <<
                entry.second.first << ", " <<
                entry.second.second << endl;
        }
    }
}

void EvalAccessTable::getDataFile() {
    {
        ifstream reader;
        reader.open(filename);
        assert(reader.good());
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
            access.pid = stoi(value);
            getline(ss, value);
            access.write = (value.find("rue") != std::string::npos);
            access.pos = i;

            getOrDefaultAndSet(last_access, i, access.pid, -1, &access.lastRef);

            i++;
        }
    }
    {
        map<int, int> next_access;
        int data_size = data.size();
        for (uint i = 0; i < data.size(); i++) {
            int pos = data_size - (i + 1);
            getOrDefaultAndSet(next_access, pos, data[pos].pid, data_size, &data[pos].nextRef);
        }
    }
}

void EvalAccessTable::createLists(bool ignore_last_run) {
    ifstream reader;
    reader.open(output_file);
    if (reader.good() && !ignore_last_run) {
        handleCsv(reader);
    } else {
        cout << "No old files found" << endl;
        filesystem::create_directory(output_dir);
    }
    runAlgorithmNonParallel("lru", LruStackDist());
}

const std::unordered_map<RamSize, std::pair<uint, uint>>& EvalAccessTable::getValues(std::string name) {
    assert(hasValues(name));
    return read_write_list[name];
}
bool EvalAccessTable::hasValues(std::string algo){
    return read_write_list.contains(algo);
}

bool EvalAccessTable::hasValue(std::string algo, RamSize ramSize){
     return hasValues(algo) &&
        read_write_list[algo].contains(ramSize);
}

bool EvalAccessTable::hasAllValues(std::string algo){
    if(ramSizes.size() == 0){
        return false;
    }
    for(RamSize size: ramSizes){
        if(!hasValue(algo, size)){
            return false;
        }
    }
    return true;
}

ramListType EvalAccessTable::missingValues(std::string algo){
    ramListType missing;
    for(RamSize size: ramSizes){
        if(!hasValue(algo, size)){
            missing.emplace(size);
        }
    }
    return missing;
}

void EvalAccessTable::handleCsv(ifstream &filestream){
    string line;
    bool first_line = true;
    while (getline(filestream, line)) {
        if (first_line) {
            assert(line == first_csv_line);
            first_line = false;
            continue;
        }
        stringstream ss(line);
        string algo, ram_size, elements, reads, writes;
        assert(getline(ss, algo, ','));
        assert(getline(ss, ram_size, ','));
        assert(getline(ss, elements, ','));
        assert(getline(ss, reads, ','));
        assert(getline(ss, writes, ','));
        assert((uint) stoi(elements) == data.size());
        read_write_list[algo][stoi(ram_size)] = std::make_pair(stoi(reads), stoi(writes));
        if(!ramSizes.contains(stoi(ram_size))){
            ramSizes.emplace(stoi(ram_size));
        }
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

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

EvalAccessTable::EvalAccessTable(std::string  filename, std::string  out_dir, bool do_run, bool test, bool benchmark): filename(std::move(filename)), output_dir(std::move(out_dir)){
        if(!do_run){
            return;
        }
        runFromFilename(test, benchmark);
    };

void EvalAccessTable::init(bool ignore_last_run, int max_ram){
    getDataFile();
    if(ignore_last_run){
        max_ram = -1;
    }
    createLists(ignore_last_run, max_ram); // this runs "lru" (lru_stack_trace)
}
void EvalAccessTable::runFromFilename(bool test, bool benchmark) {
    // in case of full run (not test or benchmark)
    bool run_slow = false;
    bool full_run = false;
    // Ignore last if test run, else not!
    init(test, 100000);

    // default_compare_algos();
    // advanced_compare_algos();
    bool do_it = true;
    if(do_it) {
        advanced_with_variations_algos();

        uint KR = 1;                  // length of first list (read or access), 0 = infty
        uint epoch_size = 0;          // How many evictions are grouped to one epoch, 0 = infty, else RAMSIZE/epoch_size
        uint randSize = 8;            // How big is the random sample
        uint randSelector = 1;        // How many Elements should be evicted per sample

        uint KW = 1;                  // length of second list (write list), 0 = infty (not counted, becaus write_cost=0)
        bool write_as_read = true;  // use first lists for accesses (reads and writes) or only reads
        float write_cost = 0;         // how expensive are writes compared to reads

        float first_value = 1.0;    // dampening factor of first access
        modus mod = mod_max;        // what modus should be taken to aggregate 
        int Z = 0;                   // size of out of memory history -1 = infty

        // Sampling
        for (int randSize: {1, 2, 4, 8, 16, 32, 64}) {
            string name = "watt_sampling_";
            if (randSize < 10)
                name += "0";
            name += to_string(randSize);
            runAlgorithm(name,
                         LFU_2K_E_real_Generator(KR, KW, epoch_size, randSize, randSelector, write_as_read, write_cost,
                                                 first_value, mod, Z));
            runAlgorithm("other" + name,
                         LFU_2K_E_real_Generator(KR, KW, epoch_size, randSize, randSelector, false, write_cost,
                                                 first_value, mod, Z));
            runAlgorithm("one" + name, LFU_1K_E_real_Generator(KR, epoch_size, randSize, randSelector, write_cost, Z));
        }
        randSize = 8;

        // Read Backlog Size
        int KR_max_value = 0;
        for (int KR: {1, 2, 4, 8, 16, 32, 64, KR_max_value}) {
            string name = "watt_backlog_";
            if (KR == KR_max_value) name += "max";
            else {
                if (KR < 10) name += "0";
                name += to_string(KR);
            }

            runAlgorithm(name,
                         LFU_2K_E_real_Generator(KR, KW, epoch_size, randSize, randSelector, write_as_read, write_cost,
                                                 first_value, mod, Z));
            runAlgorithm("other" + name,
                         LFU_2K_E_real_Generator(KR, KW, epoch_size, randSize, randSelector, false, write_cost,
                                                 first_value, mod, Z));
            runAlgorithm("one" + name, LFU_1K_E_real_Generator(KR, epoch_size, randSize, randSelector, write_cost, Z));
        }
        KR = 8;

        // Epochs
        int epochs_max_value = 0;
        for (int epoch_size: {1, 2, 4, 8, 16, 32, 64, 128, epochs_max_value}) {
            string name = "watt_epoch_";
            if (epoch_size == epochs_max_value) name += "max";
            else {
                if (epoch_size < 10) name += "0";
                if (epoch_size < 100) name += "0";
                name += to_string(epoch_size);
            }
            runAlgorithm(name,
                         LFU_2K_E_real_Generator(KR, KW, epoch_size, randSize, randSelector, write_as_read, write_cost,
                                                 first_value, mod, Z));
            runAlgorithm("other" + name,
                         LFU_2K_E_real_Generator(KR, KW, epoch_size, randSize, randSelector, false, write_cost,
                                                 first_value, mod, Z));
            runAlgorithm("one" + name, LFU_1K_E_real_Generator(KR, epoch_size, randSize, randSelector, write_cost, Z));

        }
        epoch_size = 4;

        // Dampening
        for (int i = 0; i <= 100; i += 5) {
            first_value = i / 100.0;
            string name = "watt_damp_";
            if (i < 10) name += "0";
            if (i < 100) name += "0";
            name += to_string(i);
            runAlgorithm(name,
                         LFU_2K_E_real_Generator(KR, KW, epoch_size, randSize, randSelector, write_as_read, write_cost,
                                                 first_value, mod, Z));
            runAlgorithm("other" + name,
                         LFU_2K_E_real_Generator(KR, KW, epoch_size, randSize, randSelector, false, write_cost,
                                                 first_value, mod, Z));
            runAlgorithm("one" + name, LFU_1K_E_real_Generator(KR, epoch_size, randSize, randSelector, write_cost, Z));
        }
        first_value = 0.1;
        // keep history
        int Z_max_value = -1;
        for (int Z: {0, 1, 2, 4, 8, 16, 32, 64, 128, Z_max_value}) {
            string name = "watt_history_";
            if (Z == Z_max_value) name += "max";
            else if (Z == 0) name += "none";
            else {
                if (Z < 10) name += "0";
                if (Z < 100) name += "0";
                name += to_string(Z);
            }
            runAlgorithm(name,
                         LFU_2K_E_real_Generator(KR, KW, epoch_size, randSize, randSelector, write_as_read, write_cost,
                                                 first_value, mod, Z));
            runAlgorithm("other" + name,
                         LFU_2K_E_real_Generator(KR, KW, epoch_size, randSize, randSelector, false, write_cost,
                                                 first_value, mod, Z));
            runAlgorithm("one" + name, LFU_1K_E_real_Generator(KR, epoch_size, randSize, randSelector, write_cost, Z));
        }
        Z = 0;
        // avg min max
        for (modus mod: {mod_min, mod_avg, mod_median, mod_max, mod_lucas}) {
            string name = "watt_modus_";
            switch (mod) {
                case mod_avg:
                    name += "avg";
                    break;
                case mod_min:
                    name += "min";
                    break;
                case mod_max:
                    name += "max";
                    break;
                case mod_median:
                    name += "median";
                    break;
                case mod_lucas:
                    name += "lucas";
                    break;
            }
            runAlgorithm(name, LFU_2K_E_real_Generator(KR, KW, epoch_size, randSize, randSelector, write_as_read,
                                                       write_cost, first_value, mod, Z));
            runAlgorithm("other" + name,
                         LFU_2K_E_real_Generator(KR, KW, epoch_size, randSize, randSelector, false, write_cost,
                                                 first_value, mod, Z));
            runAlgorithm("one" + name, LFU_1K_E_real_Generator(KR, epoch_size, randSize, randSelector, write_cost, Z));
        }
        mod = mod_max;
        // read and writes
        write_cost = 1; // To enable writes;
        uint KW_max_value = 0;
        for (uint KW: (std::initializer_list<uint>) {1, 2, 4, 8, 16, 32, 64, KW_max_value}) {
            string name = "watt_wr_";
            if (KW == KW_max_value) {
                name += "max";
            } else {
                if (KW < 10) name += "0";
                name += to_string(KW);
            }
            runAlgorithm(name, LFU_2K_E_real_Generator(KR, KW, epoch_size, randSize, randSelector, write_as_read,
                                                       write_cost, first_value, mod, Z));
            runAlgorithm("other" + name,
                         LFU_2K_E_real_Generator(KR, KW, epoch_size, randSize, randSelector, false, write_cost,
                                                 first_value, mod, Z));
            runAlgorithm("one" + name, LFU_1K_E_real_Generator(KR, epoch_size, randSize, randSelector, write_cost, Z));
        }
        KW = 4;
        // vary write cost
        for(int wc=0; wc<= 200; wc+=5){
            string name = "watt_wc_";
            if (wc<10) name+= "0";
            if (wc<100) name+= "0";
            name+= to_string(wc);
            write_cost = wc/10.0;
            runAlgorithm(name, LFU_2K_E_real_Generator(KR, KW, epoch_size, randSize, randSelector, write_as_read, write_cost,
                                                       first_value, mod, Z));
            //runAlgorithm("other"+name, LFU_2K_E_real_Generator(KR, KW, epoch_size, randSize, randSelector, false, write_cost,
            //                                       first_value, mod, Z));
            //runAlgorithm("one"+name, LFU_1K_E_real_Generator(KR, epoch_size, randSize, randSelector, write_cost, Z));
        }
        write_cost = 1;

        runAlgorithm("WATT", LFU_2K_E_real_Generator(KR, KW, epoch_size, randSize, randSelector, write_as_read, write_cost,
                                                       first_value, mod, Z));
        runAlgorithm("OTHERWATT", LFU_2K_E_real_Generator(KR, KW, 2, randSize, randSelector, false, write_cost+1,
                                                   0, mod, Z));
        runAlgorithm("ONEWATT", LFU_1K_E_real_Generator(16, epoch_size, randSize, randSelector, write_cost, Z));

        // Test Correlations 1 = randSize, 2= KR, 3 = epochs, 4= dampening, 5 = agg_func, 6 = kw

        // 1
        for (int randSize: {1, 2, 4, 8, 16, 32, 64}) {
            string name = "TEST_sampling_";
            if (randSize < 10)
                name += "0";
            name += to_string(randSize);
            // 2
            for (int KR: {1, 2, 4, 8, 16, 32, 64, KR_max_value}) {
                string name_inner = name + "_backlog_";
                if (KR == KR_max_value) name_inner += "max";
                else {
                    if (KR < 10) name_inner += "0";
                    name_inner += to_string(KR);
                }
                runAlgorithm(name_inner,
                             LFU_2K_E_real_Generator(KR, KW, epoch_size, randSize, randSelector, write_as_read,
                                                     write_cost,
                                                     first_value, mod, Z));
            }
            // 3
            for (int epoch_size: {1, 2, 4, 8, 16, 32, 64, 128, epochs_max_value}) {
                string name_inner = name + "_epoch_";
                if (epoch_size == epochs_max_value) name_inner += "max";
                else {
                    if (epoch_size < 10) name_inner += "0";
                    if (epoch_size < 100) name_inner += "0";
                    name_inner += to_string(epoch_size);
                }
                runAlgorithm(name_inner,
                             LFU_2K_E_real_Generator(KR, KW, epoch_size, randSize, randSelector, write_as_read,
                                                     write_cost,
                                                     first_value, mod, Z));
            }
            // 4
            for (int i = 0; i <= 100; i += 5) {
                first_value = i / 100.0;
                string name_inner = name + "_damp_";
                if (i < 10) name_inner += "0";
                if (i < 100) name_inner += "0";
                name_inner += to_string(i);
                runAlgorithm(name_inner,
                             LFU_2K_E_real_Generator(KR, KW, epoch_size, randSize, randSelector, write_as_read,
                                                     write_cost,
                                                     first_value, mod, Z));
            }

            // 5
            for (modus mod: {mod_min, mod_avg, mod_median, mod_max, mod_lucas}) {
                string name_inner = name + "_modus_";
                switch (mod) {
                    case mod_avg:
                        name_inner += "avg";
                        break;
                    case mod_min:
                        name_inner += "min";
                        break;
                    case mod_max:
                        name_inner += "max";
                        break;
                    case mod_median:
                        name_inner += "median";
                        break;
                    case mod_lucas:
                        name_inner += "lucas";
                        break;
                }
                runAlgorithm(name_inner,
                             LFU_2K_E_real_Generator(KR, KW, epoch_size, randSize, randSelector, write_as_read,
                                                     write_cost, first_value, mod, Z));
            }

            // 6
            for (uint KW: (std::initializer_list<uint>) {1, 2, 4, 8, 16, 32, 64, KW_max_value}) {
                string name_inner = name + "_wr_";
                if (KW == KW_max_value) {
                    name_inner += "max";
                } else {
                    if (KW < 10) name_inner += "0";
                    name_inner += to_string(KW);
                }
                runAlgorithm(name_inner,
                             LFU_2K_E_real_Generator(KR, KW, epoch_size, randSize, randSelector, write_as_read,
                                                     write_cost, first_value, mod, Z));
            }
        }

        // 2
        for (int KR: {1, 2, 4, 8, 16, 32, 64, KR_max_value}) {
            string name = "TEST_backlog_";
            if (KR == KR_max_value) name += "max";
            else {
                if (KR < 10) name += "0";
                name += to_string(KR);
            }
            // 3
            for (int epoch_size: {1, 2, 4, 8, 16, 32, 64, 128, epochs_max_value}) {
                string name_inner = name + "_epoch_";
                if (epoch_size == epochs_max_value) name_inner += "max";
                else {
                    if (epoch_size < 10) name_inner += "0";
                    if (epoch_size < 100) name_inner += "0";
                    name_inner += to_string(epoch_size);
                }
                runAlgorithm(name_inner,
                             LFU_2K_E_real_Generator(KR, KW, epoch_size, randSize, randSelector, write_as_read,
                                                     write_cost,
                                                     first_value, mod, Z));
            }
            // 4
            for (int i = 0; i <= 100; i += 5) {
                first_value = i / 100.0;
                string name_inner = name + "_damp_";
                if (i < 10) name_inner += "0";
                if (i < 100) name_inner += "0";
                name_inner += to_string(i);
                runAlgorithm(name_inner,
                             LFU_2K_E_real_Generator(KR, KW, epoch_size, randSize, randSelector, write_as_read,
                                                     write_cost,
                                                     first_value, mod, Z));
            }

            // 5
            for (modus mod: {mod_min, mod_avg, mod_median, mod_max, mod_lucas}) {
                string name_inner = name + "_modus_";
                switch (mod) {
                    case mod_avg:
                        name_inner += "avg";
                        break;
                    case mod_min:
                        name_inner += "min";
                        break;
                    case mod_max:
                        name_inner += "max";
                        break;
                    case mod_median:
                        name_inner += "median";
                        break;
                    case mod_lucas:
                        name_inner += "lucas";
                        break;
                }
                runAlgorithm(name_inner,
                             LFU_2K_E_real_Generator(KR, KW, epoch_size, randSize, randSelector, write_as_read,
                                                     write_cost, first_value, mod, Z));
            }

            // 6
            for (uint KW: (std::initializer_list<uint>) {1, 2, 4, 8, 16, 32, 64, KW_max_value}) {
                string name_inner = name + "_wr_";
                if (KW == KW_max_value) {
                    name_inner += "max";
                } else {
                    if (KW < 10) name_inner += "0";
                    name_inner += to_string(KW);
                }
                runAlgorithm(name_inner,
                             LFU_2K_E_real_Generator(KR, KW, epoch_size, randSize, randSelector, write_as_read,
                                                     write_cost, first_value, mod, Z));
            }
        }

        // 3
        for (int epoch_size: {1, 2, 4, 8, 16, 32, 64, 128, epochs_max_value}) {
            string name = "TEST_epoch_";
            if (epoch_size == epochs_max_value) name += "max";
            else {
                if (epoch_size < 10) name += "0";
                if (epoch_size < 100) name += "0";
                name += to_string(epoch_size);
            }
            // 4
            for (int i = 0; i <= 100; i += 5) {
                first_value = i / 100.0;
                string name_inner = name + "_damp_";
                if (i < 10) name_inner += "0";
                if (i < 100) name_inner += "0";
                name_inner += to_string(i);
                runAlgorithm(name_inner,
                             LFU_2K_E_real_Generator(KR, KW, epoch_size, randSize, randSelector, write_as_read,
                                                     write_cost,
                                                     first_value, mod, Z));
            }

            // 5
            for (modus mod: {mod_min, mod_avg, mod_median, mod_max, mod_lucas}) {
                string name_inner = name + "_modus_";
                switch (mod) {
                    case mod_avg:
                        name_inner += "avg";
                        break;
                    case mod_min:
                        name_inner += "min";
                        break;
                    case mod_max:
                        name_inner += "max";
                        break;
                    case mod_median:
                        name_inner += "median";
                        break;
                    case mod_lucas:
                        name_inner += "lucas";
                        break;
                }
                runAlgorithm(name_inner,
                             LFU_2K_E_real_Generator(KR, KW, epoch_size, randSize, randSelector, write_as_read,
                                                     write_cost, first_value, mod, Z));
            }

            // 6
            for (uint KW: (std::initializer_list<uint>) {1, 2, 4, 8, 16, 32, 64, KW_max_value}) {
                string name_inner = name + "_wr_";
                if (KW == KW_max_value) {
                    name_inner += "max";
                } else {
                    if (KW < 10) name_inner += "0";
                    name_inner += to_string(KW);
                }
                runAlgorithm(name_inner,
                             LFU_2K_E_real_Generator(KR, KW, epoch_size, randSize, randSelector, write_as_read,
                                                     write_cost, first_value, mod, Z));
            }
        }
        // 4
        for (int i = 0; i <= 100; i += 5) {
            first_value = i / 100.0;
            string name = "TEST_damp_";
            if (i < 10) name += "0";
            if (i < 100) name += "0";
            name += to_string(i);
            // 5
            for (modus mod: {mod_min, mod_avg, mod_median, mod_max, mod_lucas}) {
                string name_inner = name + "_modus_";
                switch (mod) {
                    case mod_avg:
                        name_inner += "avg";
                        break;
                    case mod_min:
                        name_inner += "min";
                        break;
                    case mod_max:
                        name_inner += "max";
                        break;
                    case mod_median:
                        name_inner += "median";
                        break;
                    case mod_lucas:
                        name_inner += "lucas";
                        break;
                }
                runAlgorithm(name_inner,
                             LFU_2K_E_real_Generator(KR, KW, epoch_size, randSize, randSelector, write_as_read,
                                                     write_cost, first_value, mod, Z));
            }

            // 6
            for (uint KW: (std::initializer_list<uint>) {1, 2, 4, 8, 16, 32, 64, KW_max_value}) {
                string name_inner = name + "_wr_";
                if (KW == KW_max_value) {
                    name_inner += "max";
                } else {
                    if (KW < 10) name_inner += "0";
                    name_inner += to_string(KW);
                }
                runAlgorithm(name_inner,
                             LFU_2K_E_real_Generator(KR, KW, epoch_size, randSize, randSelector, write_as_read,
                                                     write_cost, first_value, mod, Z));
            }
        }
        // 5
        for (modus mod: {mod_min, mod_avg, mod_median, mod_max, mod_lucas}) {
            string name = "TEST_modus_";
            switch (mod) {
                case mod_avg:
                    name += "avg";
                    break;
                case mod_min:
                    name += "min";
                    break;
                case mod_max:
                    name += "max";
                    break;
                case mod_median:
                    name += "median";
                    break;
                case mod_lucas:
                    name += "lucas";
                    break;
            }
            // 6
            for (uint KW: (std::initializer_list<uint>) {1, 2, 4, 8, 16, 32, 64, KW_max_value}) {
                string name_inner = name + "_wr_";
                if (KW == KW_max_value) {
                    name_inner += "max";
                } else {
                    if (KW < 10) name_inner += "0";
                    name_inner += to_string(KW);
                }
                runAlgorithm(name_inner,
                             LFU_2K_E_real_Generator(KR, KW, epoch_size, randSize, randSelector, write_as_read,
                                                     write_cost, first_value, mod, Z));
            }
        }

    }
    else {
        if (benchmark) {
            advanced_with_variations_algos();

            for (int kr: {16, 8, 4, 2, 0}) for (int kw: {16, 8, 4, 2, 0}) for (int e: {0, 20, 10, 5, 1})
                for (int rsi: {10}) for (int rsa: {1, 5, 10}) for (int wc: {0, 1, 2, 4, 8, 2048}) {
                    if ((kw == 0 && wc != 1) || (kr == 0 && kw == 0))
                        continue;
                    string name = "kr" + to_string(kr)
                                  + "_kw" + to_string(kw)
                                  + "_e" + to_string(e)
                                  + "_rsi" + to_string(rsi)
                                  + "_rsa" + to_string(rsa)
                                  + "_wc" + to_string(wc);
                    if (kw != 0) {
                        runAlgorithm("lfu_vers2_" + name,
                                     LFU_Generator(kr, kw, e, rsi, rsa, false, wc));
                        runAlgorithm("lfu_vers3_" + name,
                                     LFU_2K_E_real2_Generator(kr, kw, e, rsi, rsa, false, wc));
                    } else {
                        runAlgorithm("lfu_vers2_" + name + "_war",
                                     LFU_Generator(kr, kw, e, rsi, rsa, true, wc));
                        runAlgorithm("lfu_vers3_" + name + "_war",
                                     LFU_2K_E_real2_Generator(kr, kw, e, rsi, rsa, true, wc));
                    }
                }
            for (int k: {32, 16, 8, 4, 2, 0})
                for (int e: {0, 20, 10, 5, 1})
                    for (int rsi: {10})
                        for (int rsa: {1, 5, 10})
                            for (int wc: {0, 1, 2, 4, 8, 2048}) {
                                string name = to_string(k)
                                              + "_e" + to_string(e)
                                              + "_rsi" + to_string(rsi)
                                              + "_rsa" + to_string(rsa)
                                              + "_wc" + to_string(wc);
                                runAlgorithm("lfu_k" + name, LFU_1K_E_real_Generator(k, e, rsi, rsa, wc, 0));
                                runAlgorithm("lfu_bla_k" + name, LFU_1K_E_Generator(k, e, rsi, rsa, wc, 0));
                            }


        }
        if (!test && !benchmark) {
            advanced_with_variations_algos();
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
                if (run_slow) {
                    runAlgorithm("opt2", Opt2_Generator());
                    runAlgorithm("lru_alt2", LRU2_Generator());
                    runAlgorithm("cf_lru100", CfLRUGenerator(100));
                }
                for (int k: {16, 8, 4, 2}) {
                    for (int z: {100, 10, 1, -1}) {
                        runAlgorithm("lfu_k" + std::to_string(k) + "_z" + std::to_string(z), LFU_K_Z_Generator(k, z));
                        // runAlgorithm("lfu2_k" + std::to_string(k) + "_z" + std::to_string(z), LFU2_K_Z_Generator(k, z));
                        runAlgorithm("lru_k" + std::to_string(k) + "_z" + std::to_string(z), LRU_K_Z_Generator(k, z));
                        runAlgorithm("lfu_2k" + std::to_string(k) + "_z" + std::to_string(z) + "_T",
                                     LFU_2K_Z_Generator(k, k, z, true));
                        runAlgorithm("lfu_2k" + std::to_string(k) + "_z" + std::to_string(z) + "_F",
                                     LFU_2K_Z_Generator(k, k, z, false));
                        runAlgorithm("lfu_2k" + std::to_string(k) + "_z" + std::to_string(z) + "_TR",
                                     LFU_2K_Z_rand_Generator(k, k, z, 5, true));
                        runAlgorithm("lfu_2k" + std::to_string(k) + "_z" + std::to_string(z) + "_FR",
                                     LFU_2K_Z_rand_Generator(k, k, z, 5, false));
                    }
                    runAlgorithm("lfu_k_real_F_e" + std::to_string(k), LFU_2K_E_real_Generator(8, 4, k, 5, 5, false));
                    runAlgorithm("lfu_k_real2_F_e" + std::to_string(k), LFU_2K_E_real_Generator(4, 8, k, 5, 5, false));
                    runAlgorithm("lfu_k_real_F_e" + std::to_string(k) + "_wc" + std::to_string(8),
                                 LFU_Generator(8, 4, k, 5, 5, false, 8));
                    runAlgorithm("lfu_k_" + std::to_string(k), LFU_K_Generator(k));
                }
            }
        }
    }
    printToFile();
}

void EvalAccessTable::advanced_with_variations_algos() {
    advanced_compare_algos();
    runAlgorithm("lean10", Lean_Generator(10));
    runAlgorithm("lean20", Lean_Generator(20));
    runAlgorithm("lean30", Lean_Generator(30));
    runAlgorithm("lean40", Lean_Generator(40));
    runAlgorithm("lean10_2", Lean_Generator2(10));
    runAlgorithm("lean20_2", Lean_Generator2(20));
    runAlgorithm("lean30_2", Lean_Generator2(30));
    runAlgorithm("lean40_2", Lean_Generator2(40));
    runAlgorithm("cf_lru30", CfLRUGenerator(30));
    runAlgorithm("cf_lru40", CfLRUGenerator(40));
    runAlgorithm("cf_lru50", CfLRUGenerator(50));
    runAlgorithm("cf_lru60", CfLRUGenerator(60));
    runAlgorithmNonParallel("StaticOpt2", StaticOpt(2));
    runAlgorithmNonParallel("StaticOpt4", StaticOpt(4));
    runAlgorithmNonParallel("StaticOpt8", StaticOpt(8));
    runAlgorithm("hyperbolic01", Hyperbolic_generator(1));
    runAlgorithm("hyperbolic05", Hyperbolic_generator(5));
    runAlgorithm("hyperbolic20", Hyperbolic_generator(20));
    /*
    runAlgorithm("lrfu0.0", LRFU_Generator(0.0, 8));
    runAlgorithm("lrfu0.2", LRFU_Generator(0.2, 8));
    runAlgorithm("lrfu0.4", LRFU_Generator(0.4, 8));
    runAlgorithm("lrfu0.6", LRFU_Generator(0.6, 8));
    runAlgorithm("lrfu0.8", LRFU_Generator(0.8, 8));
    */
}

void EvalAccessTable::advanced_compare_algos() {
    default_compare_algos();
    runAlgorithm("lean30", Lean_Generator(30));
    runAlgorithm("lean30_2", Lean_Generator2(30));
    runAlgorithm("lru_wsr", LRU_WSR_Generator());
    runAlgorithm("cf_lru50", CfLRUGenerator(50));
    runAlgorithm("hyperbolic10", Hyperbolic_generator(10));
    //runAlgorithm("lrfu1.0", LRFU_Generator(1.0, 8));

}

void EvalAccessTable::default_compare_algos() {
    runAlgorithmNonParallel("StaticOpt", StaticOpt());
    runAlgorithm("arc", ARC_Generator());
    runAlgorithm("opt", Opt_Generator());
    runAlgorithm("clock", CLOCK_Generator());
    runAlgorithm("second_chance", SECOND_CHANCE_Generator());
    runAlgorithm("random", Random_Generator());
    runAlgorithm("lru_k2_z-1", LRU_K_Z_Generator(2, -1));
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
                algo.first << "," <<
                entry.first << "," <<
                elements << "," <<
                entry.second.first << "," <<
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

void EvalAccessTable::createLists(bool ignore_last_run, int max_ram) {
    ifstream reader;
    reader.open(output_file);
    if (reader.good() && !ignore_last_run) {
        handleCsv(reader);
    } else {
        cout << "No old files found" << endl;
        filesystem::create_directory(output_dir);
    }
    runAlgorithmNonParallel("lru", LruStackDist(max_ram));
}

const rwListSubType& EvalAccessTable::getValues(std::string name) {
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
        getline(ss, algo, ',');
        getline(ss, ram_size, ',');
        getline(ss, elements, ',');
        getline(ss, reads, ',');
        getline(ss, writes, ',');
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

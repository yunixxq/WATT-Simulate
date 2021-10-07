//
// Created by dev on 06.10.21.
//
#pragma once

#ifndef C_EVICTSTRATEGY_HPP
#define C_EVICTSTRATEGY_HPP

#endif //C_EVICTSTRATEGY_HPP

#include "../evalAccessTable/general.hpp"

template<class RamType>
class EvictStrategy
{
public:
    unsigned int RAM_SIZE=0;
    explicit EvictStrategy(int ram_size): RAM_SIZE(ram_size){};
    std::unordered_map<unsigned int, RamType> ram;
    std::vector<unsigned int> dirty_in_ram;
    virtual void access(Access& access) = 0;
    virtual bool evictOne(unsigned int curr_time) = 0;
    // removes pid from strucutres, returns true if page was dirty
    bool handleRemove(unsigned int pid){
        ram.erase(pid);
        if (dirty_in_ram[pid]){
            dirty_in_ram[pid] = false;
            return true;
        }
        return false;
    }
    bool inRam(unsigned int pid){
        return ram.find(pid) != ram.end();
    }

    unsigned int dirtyPages(){
        return std::count(dirty_in_ram.begin(), dirty_in_ram.end(), true);
    }

    void handleDirty(unsigned int pid, bool write){
        checkSizes(pid);
        dirty_in_ram[pid] |= write;
    }

    void checkSizes(unsigned int pid){
        if(dirty_in_ram.size() < pid){
            dirty_in_ram.resize(pid+1, false);
        }
    }


    std::pair<unsigned int, unsigned int> executeStrategy(std::vector<Access> access_data){
        unsigned int page_misses = 0, dirty_evicts = 0;
        for(Access& single_access: access_data){
            if(!inRam(single_access.pageRef)){
                page_misses++;
                if(ram.size() >= RAM_SIZE){
                    if(evictOne(single_access.pos)){
                        dirty_evicts++;
                    }
                }
            }
            access(single_access);
        }
        return std::pair(page_misses, dirty_evicts + dirtyPages());
    }

};

template<class Strategy>
void executeStrategy(std::vector<Access> &data, const std::string &name, std::vector<unsigned int> &x_list, std::vector<unsigned int> &read_list,
                     std::vector<unsigned int> &write_list) {
    std::cout << name << std::endl;
    auto t1 = std::chrono::high_resolution_clock::now();
    for(auto& ram_size: x_list){
        auto pair = Strategy(ram_size).executeStrategy(data);
        read_list.push_back(pair.first);
        write_list.push_back(pair.second);
        }
    auto t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> seconds_double = t2 - t1;
    std::cout << seconds_double.count() << " seconds" << std::endl;
}
//
// Created by dev on 06.10.21.
//
#pragma once

#ifndef C_EVICTSTRATEGY_HPP
#define C_EVICTSTRATEGY_HPP

#endif //C_EVICTSTRATEGY_HPP

#include <map>
#include "../evalAccessTable/general.hpp"

class EvictStrategy
{
public:
    explicit EvictStrategy(){};

    virtual void evaluateRamList(std::vector<Access> &data, std::vector<RamSize> &x_list, std::vector<uInt> &read_list,
                         std::vector<uInt> &write_list) {
        for(auto& ram_size: x_list){
            reInit(ram_size);
            assert(ramSize() == 0);
            assert(RAM_SIZE == ram_size);
            assert(dirty_in_ram.size() == 0);
            assert(in_ram.size() == 0);
            auto pair = executeStrategy(data);
            read_list.push_back(pair.first);
            write_list.push_back(pair.second);
        }
    }
protected:
    virtual void reInit(RamSize ram_size){
        RAM_SIZE = ram_size;
        dirty_in_ram.clear();
        in_ram.clear();
    }
    virtual RamSize ramSize() = 0;
    std::pair<uInt, uInt> executeStrategy(std::vector<Access> access_data){
        uInt page_misses = 0, dirty_evicts = 0;
        for(Access& single_access: access_data){
            checkSizes(single_access.pageRef);
            if(!in_ram[single_access.pageRef]){
                page_misses++;
                if(ramSize() >= RAM_SIZE){
                    PID pid = evictOne(single_access.pos);
                    if(postRemove(pid)){
                        dirty_evicts++;
                    }
                }
            }
            access(single_access);
            dirty_in_ram[single_access.pageRef] = dirty_in_ram[single_access.pageRef] || single_access.write;
            in_ram[single_access.pageRef] = true;
        }
        return std::pair(page_misses, dirty_evicts + dirtyPages());
    }

    RamSize RAM_SIZE=0;
    std::vector<bool> dirty_in_ram;
    std::vector<bool> in_ram;


    virtual void access(Access& access) = 0;
    virtual PID evictOne(RefTime curr_time) = 0;
    // removes pid from strucutres, returns true if page was dirty

    int dirtyPages(){
        return std::count(dirty_in_ram.begin(), dirty_in_ram.end(), true);
    }

    void checkSizes(PID pid){
        if(dirty_in_ram.size() < pid){
            dirty_in_ram.resize(pid+1, false);
        }
        if(in_ram.size() < pid){
            in_ram.resize(pid+1, false);
        }
    }

    bool postRemove(PID pid){
        in_ram[pid]=false;
        if (dirty_in_ram[pid]){
            dirty_in_ram[pid] = false;
            return true;
        }
        return false;
    }
};

template<class Container>
class EvictStrategyContainer: public EvictStrategy{
protected:
    Container ram;
    RamSize ramSize() override{
        return ram.size();
    }
    void reInit(RamSize ram_size) override{
        EvictStrategy::reInit(ram_size);
        ram.clear();
    }

    PID removeCandidate(typename Container::iterator candidate){
        PID pid = candidate->pageRef;
        ram.erase(candidate);
        return pid;
    }
    PID removeCandidatePidFirst(typename Container::iterator candidate){
        PID pid = candidate->first;
        ram.erase(candidate);
        return pid;
    }
    PID removeCandidatePidSecond(typename Container::iterator candidate){
        PID pid = candidate->second;
        ram.erase(candidate);
        return pid;
    }
    template<typename T>
    typename std::vector<T>::iterator findInVector(PID pid, std::vector<T>& ram){
        return std::find_if(ram.begin(), ram.end(), [pid](const auto& elem) { return elem.first == pid; });
    }
    typename std::vector<Access>::iterator findInVector(PID pid, std::vector<Access>& ram){
        return std::find_if(ram.begin(), ram.end(), [pid](const Access& elem) { return elem.pageRef == pid; });
    }
    template<typename T1, typename T2, typename T3>
    typename std::map<T1, T2, T3>::iterator findInMap(PID pid, std::map<T1, T2, T3>& ram){
        return std::find_if(ram.begin(), ram.end(), [pid](const auto& elem) { return elem.first == pid; });
    }

    template<typename T1, typename T2>
    bool handleRemoveMap(typename std::map<T1, T2>::iterator it, std::map<T1, T2>& ram){
        int pid = it->second;
        ram.erase(it);
        return postRemove(pid);
    }
    template<typename T>
    bool handleRemoveVector(int pid, std::vector<T>& ram){
        ram.erase(findInVector(pid, ram));
        return postRemove(pid);
    }

    template<typename T1, typename T2>
    bool inRamUnorderedMap(T1 pid, std::unordered_map<T1, T2>& ram){
        return ram.find(pid) != ram.end();
    }

    template<typename T>
    bool inRamVector(int pid, std::vector<T>& ram){
        return findInVector(pid, ram) != ram.end();
    }
    template<typename T1, typename T2, typename T3>
    bool inRamMap(T1 pid, std::map<T1, T2, T3>& ram){
        return findInMap(pid, ram) != ram.end();
    }
    static bool compare_second(const std::pair<int, int>& l, const std::pair<int, int>& r) { return l.second < r.second; };
    template<typename T>
    static bool comparePairPos(const std::pair<T, Access>& l, const std::pair<T,  Access>& r) { return l.second.pos < r.second.pos; };
    static bool comparePos(const Access& l, const Access& r) { return l.pos < r.pos; };
    static auto compare_next(const Access& l, const Access& r) { return l.nextRef < r.nextRef; };
    static auto compare_pos_writeBigger(const Access& l, const Access& r) {
        if(l.write == r.write){
            return l.pos < r.pos;
        }
        if(l.write){
            return l.write < r.write;
        }
    };

};


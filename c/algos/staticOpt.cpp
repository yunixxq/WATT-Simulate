//
// Created by dev on 06.10.21.
//

#include <filesystem>
#include <cassert>
#include <iostream>
#include <vector>
#include <numeric>
#include "staticOpt.hpp"

void StaticOpt::evaluateRamList(const std::vector<Access> &data,
                                std::vector<RamSize> &x_list,
                                [[maybe_unused]] std::vector<uInt> &read_list,
                                [[maybe_unused]]std::vector<uInt> &write_list) {
    std::vector<int> reads, writes;
    std::vector<PID> pageIds;
    PID maxPageId = 0;
    for(auto &access: data){
        if(maxPageId < access.pageRef){
            maxPageId = access.pageRef;
        }
    }
    reads.resize(maxPageId + 1);
    writes.resize(maxPageId + 1);

    for(PID i = 0; i < maxPageId; i++){
        reads[i] = 0;
        writes[i] = 0;
    }

    for (auto &access: data) {
        reads[access.pageRef]++;
        if(access.write){
            writes[access.pageRef]++;
        }
    }
    double write_cost = 1;
    double read_cost = 1;

    for(PID i = 0; i < reads.size(); i++){
        if(reads[i] == 0 && writes[i] == 0){
            continue;
        }
        pageIds.emplace_back(i);
    }
    std::sort(pageIds.begin(), pageIds.end(),
              [reads, writes, read_cost, write_cost](PID first, PID second) {
        double cost_first, cost_second;
        cost_first = reads[first]*read_cost + writes[first]*write_cost;
        cost_second = reads[second]*read_cost + writes[second] * write_cost;
        if(cost_first != cost_second){
            return cost_first >cost_second;
        }else{
            return first > second;
        }
    });
    /*
    std::cout << "PIDS: ";
    for(PID i = 0; i < pageIds.size(); i++){
       std::cout << pageIds[i] << ", ";
    }
    std::cout << std::endl;
    */
    for(RamSize ram: x_list){
        uInt pageMisses = 0, dirtyEvicts = 0;
        if(pageIds.size() > ram){
            for(uInt pos = 0; pos < pageIds.size(); pos++){
                if(pos < ram -1){
                    pageMisses++;
                    if(writes[pageIds[pos]] > 0){
                        dirtyEvicts++;
                    }
                }else{
                    pageMisses += reads[pageIds[pos]];
                    dirtyEvicts += writes[pageIds[pos]];
                }
            }

        }else {
            for (uInt pos = 0; pos < pageIds.size(); pos++) {
                pageMisses++;
                if (writes[pageIds[pos]] > 0) {
                    dirtyEvicts++;
                }
            }
        }
        read_list.emplace_back(pageMisses);
        write_list.emplace_back(dirtyEvicts);
    }
}
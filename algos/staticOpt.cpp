//
// Created by dev on 06.10.21.
//

#include "staticOpt.hpp"
void StaticOpt::evaluateRamList(const std::vector<Access> &data, const ramListType &ramList,
                                rwListSubType &readWriteList) {
    std::vector<int> reads, writes;
    std::vector<PID> pageIds;
    PID maxPageId = 0;
    for(auto &access: data){
        if(maxPageId < access.pid){
            maxPageId = access.pid;
        }
    }
    reads.resize(maxPageId + 1, 0);
    writes.resize(maxPageId + 1, 0);

    for (auto &access: data) {
        reads[access.pid]++;
        if(access.write){
            writes[access.pid]++;
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

    for(RamSize ram_size: ramList){
        uint pageMisses = 0, dirtyEvicts = 0;
        if(pageIds.size() > ram_size){
            for(uint pos = 0; pos < pageIds.size(); pos++){
                if(pos < ram_size -1){
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
            for (uint pos = 0; pos < pageIds.size(); pos++) {
                pageMisses++;
                if (writes[pageIds[pos]] > 0) {
                    dirtyEvicts++;
                }
            }
        }
        readWriteList[ram_size] = std::make_pair(pageMisses, dirtyEvicts);
    }
}
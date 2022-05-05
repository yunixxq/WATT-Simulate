//
// Created by dev on 06.10.21.
//

#include "staticOpt.hpp"
void StaticOpt::evaluateRamList(const std::vector<Access> &data, const ramListType &ramList,
                                rwListSubType &readWriteList) {
    std::vector<int> costs, reads, writes;
    std::vector<PID> pageIds;
    PID maxPageId = 0;
    for(auto &access: data){
        if(maxPageId < access.pid){
            maxPageId = access.pid;
        }
    }
    costs.resize(maxPageId + 1, 0);
    reads.resize(maxPageId + 1, 0);
    writes.resize(maxPageId + 1, 0);

    for (auto &access: data) {
        if(reads[access.pid]!=0){
            costs[access.pid]++;
        }
        reads[access.pid]++;
        if(access.write){
            if(writes[access.pid]!=0){
                costs[access.pid]+= write_cost;
            }
            writes[access.pid]++;
        }
    }

    for(PID i = 0; i < costs.size(); i++){
        if(reads[i] == 0 && writes[i] == 0){
            continue;
        }
        pageIds.emplace_back(i);
    }
    std::sort(pageIds.begin(), pageIds.end(),
              [&costs, &writes](PID first, PID second) {
        if (costs[first] == costs[second])
            return writes[first]> writes[second];
        return costs[first] > costs[second];
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
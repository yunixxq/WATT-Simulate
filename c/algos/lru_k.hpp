//
// Created by dev on 06.10.21.
//

#include "EvictStrategy.hpp"
#include <random>
#include <list>

using namespace std;

template<int K>
struct LRU_K_ALL: public EvictStrategyContainer<unordered_map<PID, list<RefTime>>> {
    void access(Access& access) override{
        list<RefTime>& hist = ram[access.pageRef];
        hist.push_front(access.pos);
        if(hist.size() > K){
            hist.resize(K);
        }
        assert(*hist.begin() == access.pos);
    };
    PID evictOne(RefTime) override{
        //auto candidate = std::min_element(ram.begin(), ram.end(), compare);
        unordered_map<PID, list<RefTime>>::iterator candidate = ram.begin(), runner = ram.begin();

        while(runner!= ram.end()){
            if(older_than(runner->second, candidate->second)){
                candidate = runner;
            }
            runner++;
        }// */
        PID pid = candidate->first;
        ram.erase(candidate);
        return pid;
    }
    static bool older_than(const list<RefTime>& l, const list<RefTime>& r) {
        if(l.size()== r.size()){
            return *(l.rbegin()) < *(r.rbegin()); // higher is younger
        }else{
            return l.size() < r.size(); // bigger is better
        }
    };

};
//
// Created by dev on 21.03.22.
//

#include "EvictStrategy.hpp"
#include <functional>


struct ARC: public EvictStrategyHashList<PID>{
    using upper = EvictStrategyHashList<PID>;
public:
    explicit ARC(): upper(){}
protected:
    using hash_map = std::unordered_map<PID, std::list<PID>::iterator>;
    std::list<PID> fresh_ram, fresh_ram_history, ram_history;
    hash_map fresh_ram_fast_finder, fresh_ram_history_fast_finder, ram_history_fast_finder;
    uint p = 0; // Size of fresh ram
    std::list<PID>& T1 = fresh_ram, T2 = ram, B1 = fresh_ram_history, B2 = ram_history;
    hash_map& T1_p = fresh_ram_fast_finder, T2_p = fast_finder, B1_p = fresh_ram_history_fast_finder, B2_p = ram_history_fast_finder;

    void reInit(RamSize ram_size) override{
        upper::reInit(ram_size);
        T1.clear();
        B1.clear();
        B2.clear();
        T1_p.clear();
        B1_p.clear();
        B2_p.clear();
        p=0;
    }
    void access(const Access& access) override{
        if(upper::in_ram[access.pid]){
            // Cache hit; Move to not_so_fresh_ram;
            if (T2_p.find(access.pid) != T2_p.end()) {
                // in hot ram
                // Put to front;
                T2_p[access.pid] = updateElement(T2_p[access.pid], access, T2);
            } else {
                // in fresh ram
                // Put to hot ram
                assert(T1_p.find(access.pid) != T1_p.end());

                T1.erase(T1_p[access.pid]);
                T1_p.erase(access.pid);
                T2_p[access.pid] = insertElement(access, T2);
            }

        }else if (B1_p.find(access.pid) != B1_p.end()){ // Case II
            // Hit in fresh ram History;
            // Adapt (done in evict); move to Hot ram
            B1.erase(B1_p[access.pid]);
            B1_p.erase(access.pid);
            T2_p[access.pid] = insertElement(access, T2);
        } else if (B2_p.find(access.pid) != B2_p.end()){ // Case III
            // Hit hot ram History;
            // Adapt (done in evict); move to Hot ram
            B2.erase(B2_p[access.pid]);
            B2_p.erase(access.pid);
            T2_p[access.pid] = insertElement(access, T2);
        } else { // Case IV
            // Cache + History Miss
            // Insert to fresh ram;
            T1_p[access.pid] = insertElement(access, T1);
        }
    };
    PID evictOne(Access access) override{
        auto evict = [&](
                std::list<PID>& main, std::list<PID>& history,
                hash_map& main_finder, hash_map& history_finder){
            PID pid = deleteLast(main, main_finder);
            history_finder[pid] = insertElementHelper(pid, history);
            return pid;
        };
        auto evictT1 = [&](){
            return evict(T1, B1, T1_p, B1_p);
        };
        auto evictT2 = [&](){
            return evict(T2, B2, T2_p, B2_p);
        };
        auto replace = [&](bool equal = false){
            if(T1_p.size() > 0 && (T1_p.size() > p || (equal && T1_p.size() == p))){
                return evictT1();
            }else{
                return evictT2();
            }
        };

        // Replace (Access is not in current ram, check for cases)
        if (B1_p.find(access.pid) != B1_p.end()){ // Case II
            // Hit in fresh ram History;
            // Adapt p= min{p+delta_1,c}; delta_1 = {1 (b1>= b2) | b2/b1 sonst}; replace(p);
            uint b2 = B2_p.size(), b1 = B1_p.size();
            uint inner = (b1 >= b2) ? 1 : b2/b1;
            p=std::min(p + inner, RAM_SIZE);
            return replace();

        } else if (B2_p.find(access.pid) != B2_p.end()){ // Case III
            // Hit hot ram History;
            // Adapt p= max{p-delta_2,0}; delta_2 = {1 (b2>= b1) | b1/b2 sonst}; replace(p)
            uint b2 = B2_p.size(), b1 = B1_p.size();
            uint inner = (b2 >= b1) ? 1 : b1/b2;
            p=std::max(p - inner, (uint)0);
            return replace(true);


        } else { // Case IV
            // Cache + History Miss
            if((T1_p.size() + B1_p.size()) == RAM_SIZE){
                if(T1_p.size() != RAM_SIZE){
                    deleteLast(B1, B1_p);
                    return replace();
                }else {
                    return deleteLast(T1, T1_p);
                }
            }else if(T2_p.size() + B2_p.size() + T1_p.size() + B1_p.size() >= RAM_SIZE) {
                if(T2_p.size() + B2_p.size() + T1_p.size() + B1_p.size() >= 2* RAM_SIZE){
                    deleteLast(B2, B2_p);
                }
                return replace();
            }
        }
        assert(false);
        std::cout << "ERROR!" << std::endl;
        return 0;
    }

    /**
     * Algorithm specific Insert Function
     * @param access
     * @return Iterator for pos in RAM
     */
    std::list<PID>::iterator insertElement(const Access& access, std::list<PID>& ram) override{
        return insertElementHelper(access.pid, ram);
    }
    static std::list<PID>::iterator insertElementHelper(PID pid, std::list<PID>&ram){
        ram.push_back(pid);
        return std::prev(ram.end());
    }
    /**
     * Algorithm specific update Function
     * @param old current position in RAM
     * @param access
     * @return Iterator for pos in RAM
     */
    typename std::list<PID>::iterator updateElement(typename std::list<PID>::iterator old, const Access& access, std::list<PID>& ram) override{
        ram.erase(old);
        return insertElement(access, ram);
    }
    static PID deleteLast(std::list<PID>& ram, hash_map& pointer){
        auto min = ram.begin();
        PID pid = *min;
        pointer.erase(pid);
        ram.erase(min);
        return pid;
    }

};


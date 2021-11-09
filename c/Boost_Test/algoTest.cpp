//
// Created by dev on 13.10.21.
//

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include "../evalAccessTable/evalAccessTable.hpp"
#include "../algos/lru.hpp"
#include "../algos/lru_k.hpp"
#include "../algos/lfu_k.hpp"

EvalAccessTable init(){
    std::string file = "./../tpcc_64_-5.csv";
    EvalAccessTable test = EvalAccessTable(file, "/var/tmp", false);
    test.init(false);
    return std::move(test);
}

template<class algo>
bool compareToOther(EvalAccessTable& instance, std::string other, std::string name, std::vector<int> args = {}){
    instance.runAlgorithm<algo>(name, args);
    return (instance.getReads(name) == instance.getReads(other)) && (instance.getWrites(name) == instance.getWrites(other));

}

template<class algo>
void runAlgo(EvalAccessTable& instance, std::string name, std::vector<int> args = {}){
    instance.runAlgorithm<algo>(name, args);
}

BOOST_AUTO_TEST_SUITE(compare_algo)
    BOOST_AUTO_TEST_CASE(lru) {
        EvalAccessTable instance = init();

        BOOST_TEST(compareToOther<LRU>(instance, "lru", "lru_0"));
        BOOST_TEST(compareToOther<LRU1>(instance, "lru", "lru_1"));
        BOOST_TEST(compareToOther<LRU2>(instance, "lru", "lru_2"));
        BOOST_TEST(compareToOther<LRU2a>(instance, "lru", "lru_2a"));
        BOOST_TEST(compareToOther<LRU2b>(instance, "lru", "lru_2b"));
        BOOST_TEST(compareToOther<LRU3>(instance, "lru", "lru_3"));
    }
    BOOST_AUTO_TEST_CASE(lru_sim) {
        EvalAccessTable instance = init();

        BOOST_TEST(compareToOther<LRU_K_Z>(instance, "lru", "lru_K1_Z0", {1, 0}));
        BOOST_TEST(compareToOther<LRU_K_Z>(instance, "lru", "lru_K1_Z10", {1, 10}));
        BOOST_TEST(compareToOther<LRU_K_Z>(instance, "lru", "lru_K1_Z-10", {1, -10}));
        BOOST_TEST(compareToOther<LRU_K_alt>(instance, "lru", "lru_K_ALT_1", {1}));

        BOOST_TEST(compareToOther<LFU_K<>>(instance, "lru", "lfu_K_1", {1}));
        BOOST_TEST(compareToOther<LFU_K_Z<>>(instance, "lru", "lfu_K1_Z0", {1, 0}));
        BOOST_TEST(compareToOther<LFU_K_Z<>>(instance, "lru", "lfu_K1_Z10", {1, 10}));
        BOOST_TEST(compareToOther<LFU_K_Z<>>(instance, "lru", "lfu_K1_Z-10", {1, -10}));
        BOOST_TEST(compareToOther<LFU_K_Z2<>>(instance, "lru", "lfu2_K1_Z0", {1, 0}));
        BOOST_TEST(compareToOther<LFU_K_Z2<>>(instance, "lru", "lfu2_K1_Z10", {1, 10}));
        BOOST_TEST(compareToOther<LFU_K_Z2<>>(instance, "lru", "lfu2_K1_Z-10", {1, -10}));
        BOOST_TEST(compareToOther<LFU_K_alt<>>(instance, "lru", "lfu_K_ALT_1", {1, -10}));
        BOOST_TEST(compareToOther<LFU_K_Z_D1<>>(instance, "lru", "lfu_K1_Z0_D10", {1, 0, 10}));
        BOOST_TEST(compareToOther<LFU_K_Z_D1<>>(instance, "lru", "lfu_K1_Z10_D10", {1, 10, 10}));

    }
    BOOST_AUTO_TEST_CASE(lru_ks) {
        EvalAccessTable instance = init();
        runAlgo<LRU_K_Z>(instance, "lru_K1_Z0", {1,0});
        BOOST_TEST(compareToOther<LRU_K_alt>(instance, "lru_K1_Z0", "lru_K_ALT_1", {1}));
        runAlgo<LRU_K_Z>(instance, "lru_K2_Z0", {2,0});
        BOOST_TEST(compareToOther<LRU_K_alt>(instance, "lru_K2_Z0", "lru_K_ALT_2", {2}));
        runAlgo<LRU_K_Z>(instance, "lru_K10_Z0", {10,0});
        BOOST_TEST(compareToOther<LRU_K_alt>(instance, "lru_K10_Z0", "lru_K_ALT_10", {10}));
    }
    BOOST_AUTO_TEST_CASE(lfu_ks) {
        EvalAccessTable instance = init();
        // InRam History
        runAlgo<LFU_K<>>(instance, "lfu_K1", {1});
        BOOST_TEST(compareToOther<LFU_K_Z<>>(instance, "lfu_K1", "lfu_K1_Z0", {1, 0}));
        BOOST_TEST(compareToOther<LFU_K_Z2<>>(instance, "lfu_K1", "lfu2_K1_Z0", {1, 0}));
        runAlgo<LFU_K<>>(instance, "lfu_K2", {2});
        BOOST_TEST(compareToOther<LFU_K_Z<>>(instance, "lfu_K2", "lfu_K2_Z0", {2, 0}));
        BOOST_TEST(compareToOther<LFU_K_Z2<>>(instance, "lfu_K2", "lfu2_K2_Z0", {2, 0}));
        runAlgo<LFU_K<>>(instance, "lfu_K10", {10});
        BOOST_TEST(compareToOther<LFU_K_Z<>>(instance, "lfu_K10", "lfu_K10_Z0", {10, 0}));
        BOOST_TEST(compareToOther<LFU_K_Z2<>>(instance, "lfu_K10", "lfu2_K10_Z0", {10, 0}));

        // Out of Ram History
        runAlgo<LFU_K_Z<>>(instance, "lfu_K1_Z1", {1, 1});
        BOOST_TEST(compareToOther<LFU_K_Z2<>>(instance, "lfu_K1_Z1", "lfu2_K1_Z1", {1, 1}));

        runAlgo<LFU_K_Z<>>(instance, "lfu_K1_Z10", {1, 10});
        BOOST_TEST(compareToOther<LFU_K_Z2<>>(instance, "lfu_K1_Z10", "lfu2_K1_Z10", {1, 10}));

        runAlgo<LFU_K_Z<>>(instance, "lfu_K2_Z10", {2, 10});
        BOOST_TEST(compareToOther<LFU_K_Z2<>>(instance, "lfu_K2_Z10", "lfu2_K2_Z10", {2, 10}));

        runAlgo<LFU_K_Z<>>(instance, "lfu_K2_Z-10", {2, -10});
        BOOST_TEST(compareToOther<LFU_K_Z2<>>(instance, "lfu_K2_Z-10", "lfu2_K2_Z-10", {2, -10}));

        runAlgo<LFU_K_Z<>>(instance, "lfu_K10_Z10", {10, 10});
        BOOST_TEST(compareToOther<LFU_K_Z2<>>(instance, "lfu_K10_Z10", "lfu2_K10_Z10", {10, 10}));
        BOOST_TEST(compareToOther<LFU_K_Z_D1<>>(instance, "lfu_K10_Z10", "lfu_K10_Z10_D10", {10, 10, 10}));
 }

BOOST_AUTO_TEST_SUITE_END()
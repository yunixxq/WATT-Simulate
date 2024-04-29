//
// Created by dev on 13.10.21.
//
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include "../evalAccessTable/evalAccessTable.hpp"

EvalAccessTable init(){
    std::string file = "./../tpcc_64_-5.csv";
    EvalAccessTable test = EvalAccessTable(file, "/var/tmp", false, true, true);
    test.init(true, -1, true);
    return std::move(test);
}

template<class T>
bool compareToOther(EvalAccessTable& instance, std::function<T()> generator, std::string name, std::string other){
    instance.runAlgorithm(name, generator, true, true);
    if (instance.getValues(name) == instance.getValues(other)){
        return true;
    }
    std::cout << name << std::endl;
    auto mine = instance.getValues(name);
    auto other_iterator = instance.getValues(other).begin();
    auto mine_iterator = mine.begin();
    std::cout << "this, other" << std::endl;
    while (mine_iterator != mine.end()){
        std::cout << "(" << mine_iterator->second.first << ", " << mine_iterator->second.second << "), (" << other_iterator->second.first << ", " << other_iterator->second.second << ")" <<std::endl;
        mine_iterator++;
        other_iterator++;
    }
    return false;
}

template<class T>
void runAlgo(EvalAccessTable& instance, std::function<T()> generator, std::string name){
    instance.runAlgorithm(name, generator, true, true);
}

BOOST_AUTO_TEST_SUITE(compare_algo)
    BOOST_AUTO_TEST_CASE(lru) {
        EvalAccessTable instance = init();

        BOOST_TEST(compareToOther(instance, LRU_Generator(), "lru_0", "lru"));
        BOOST_TEST(compareToOther(instance, LRU1_Generator(), "lru_1", "lru"));
        BOOST_TEST(compareToOther(instance, LRU2_Generator(), "lru_2", "lru"));
        BOOST_TEST(compareToOther(instance, LRU2a_Generator(), "lru_2a", "lru"));
        BOOST_TEST(compareToOther(instance, LRU2b_Generator(), "lru_2b", "lru"));
    }
    BOOST_AUTO_TEST_CASE(lru_sim) {
        EvalAccessTable instance = init();

        BOOST_TEST(compareToOther(instance, LRUalt_K_Generator(1), "lru_K_ALT_1", "lru"));

        for(int Z: {0,10,-10}){
            BOOST_TEST(compareToOther(instance, LRU_K_Z_Generator(1,Z), "lru_K1_Z" + std::to_string(Z),  "lru"));
            BOOST_TEST(compareToOther(instance, WATT_RO_NoRAND_OneEVICT_HISTORY_Generator(1,Z), "lfu_K1_Z" + std::to_string(Z), "lru"));
            BOOST_TEST(compareToOther(instance, WATT_NoRAND_OneEVICT_HISTORY_Generator(1, 1, Z, true, 0, true, true), "lfu2_K1_Z" + std::to_string(Z), "lru"));
            BOOST_TEST(compareToOther(instance, WATT_ScanRANDOM_OneEVICT_HISTORY_Generator(1, 1, Z, 0, true, 0, true, true), "WATT_one_K1_Z" + std::to_string(Z), "lru"));
        }
        BOOST_TEST(compareToOther(instance, WATT_RO_NoRAND_OneEVICT_Generator(1), "lfu_K1", "lru"));

    }
    BOOST_AUTO_TEST_CASE(lru_ks) {
        EvalAccessTable instance = init();
        runAlgo(instance, LRU_K_Z_Generator(1,0), "lru_K1_Z0");
        BOOST_TEST(compareToOther(instance, LRUalt_K_Generator(1), "lru_K_ALT_1", "lru_K1_Z0"));

        runAlgo(instance, LRU_K_Z_Generator(2,0), "lru_K2_Z0");
        BOOST_TEST(compareToOther(instance, LRUalt_K_Generator(2), "lru_K_ALT_2", "lru_K2_Z0"));

        runAlgo(instance, LRU_K_Z_Generator(10,0), "lru_K10_Z0");
        BOOST_TEST(compareToOther(instance, LRUalt_K_Generator(10), "lru_K_ALT_10", "lru_K10_Z0"));
    }
    BOOST_AUTO_TEST_CASE(WATT) {
        EvalAccessTable instance = init();
        // InRam History
        for(int K: {1, 2, 10}){
            int Z = 0;
            runAlgo(instance, WATT_RO_NoRAND_OneEVICT_Generator(K), "lfu_K" + std::to_string(K));
            BOOST_TEST(compareToOther(instance, WATT_RO_NoRAND_OneEVICT_HISTORY_Generator(K, Z), "lfu_K"+std::to_string(K)+"_Z"+std::to_string(Z), "lfu_K" + std::to_string(K)));
            BOOST_TEST(compareToOther(instance, WATT_NoRAND_OneEVICT_HISTORY_Generator(K, 1, Z, true, 0, true, true), "lfu_2K" + std::to_string(K) + "_0_Z" + std::to_string(Z), "lfu_K" + std::to_string(K)));
            // Out of Ram History
            for(int Z: {1,10, -10}){
                runAlgo(instance, WATT_RO_NoRAND_OneEVICT_HISTORY_Generator(K, Z), "lfu_K"+std::to_string(K)+"_Z"+std::to_string(Z));
                BOOST_TEST(compareToOther(instance, WATT_NoRAND_OneEVICT_HISTORY_Generator(K, 1, Z, true, 0, true, true), "lfu_2K" + std::to_string(K) + "_0_Z" + std::to_string(Z), "lfu_K" + std::to_string(K) + "_Z" + std::to_string(Z)));
            }
        }
 }
    BOOST_AUTO_TEST_CASE(opt) {
        EvalAccessTable instance = init();
        runAlgo(instance, Opt_Generator(), "opt");
        BOOST_TEST(compareToOther(instance,  Opt2_Generator(), "opt2", "opt"));
        BOOST_TEST(compareToOther(instance,  Opt3_Generator(), "opt3", "opt"));
    }
BOOST_AUTO_TEST_SUITE_END()
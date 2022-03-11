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
    test.init(true);
    return std::move(test);
}

template<class T>
bool compareToOther(EvalAccessTable& instance, std::function<T()> generator, std::string name, std::string other){
    instance.runAlgorithm(name, generator);
    return (instance.getValues(name) == instance.getValues(other));

}

template<class T>
void runAlgo(EvalAccessTable& instance, std::function<T()> generator, std::string name){
    instance.runAlgorithm(name, generator);
}

BOOST_AUTO_TEST_SUITE(compare_algo)
    BOOST_AUTO_TEST_CASE(lru) {
        EvalAccessTable instance = init();

        BOOST_TEST(compareToOther(instance, LRU_Generator(), "lru_0", "lru"));
        BOOST_TEST(compareToOther(instance, LRU1_Generator(), "lru_1", "lru"));
        BOOST_TEST(compareToOther(instance, LRU2_Generator(), "lru_2", "lru"));
        BOOST_TEST(compareToOther(instance, LRU2a_Generator(), "lru_2a", "lru"));
        BOOST_TEST(compareToOther(instance, LRU2b_Generator(), "lru_2b", "lru"));
        BOOST_TEST(compareToOther(instance, LRU3_Generator(), "lru_3", "lru"));
    }
    BOOST_AUTO_TEST_CASE(lru_sim) {
        EvalAccessTable instance = init();

        BOOST_TEST(compareToOther(instance, LRU_K_Z_Generator(1,0), "lru_K1_Z0",  "lru"));
        BOOST_TEST(compareToOther(instance, LRU_K_Z_Generator(1,10), "lru_K1_Z10",  "lru"));
        BOOST_TEST(compareToOther(instance, LRU_K_Z_Generator(1,-10), "lru_K1_Z-10",  "lru"));
        BOOST_TEST(compareToOther(instance, LRUalt_K_Generator(1), "lru_K_ALT_1", "lru"));

        BOOST_TEST(compareToOther(instance, LFU_K_Generator(1), "lfu_K1", "lru"));
        BOOST_TEST(compareToOther(instance, LFU_K_Z_Generator(1,0), "lfu_K1_Z0", "lru"));
        BOOST_TEST(compareToOther(instance, LFU_K_Z_Generator(1,10), "lfu_K1_Z10", "lru"));
        BOOST_TEST(compareToOther(instance, LFU_K_Z_Generator(1,-10), "lfu_K1_Z-10", "lru"));
        BOOST_TEST(compareToOther(instance, LFU2_K_Z_Generator(1, 0), "lfu2_K1_Z0", "lru"));
        BOOST_TEST(compareToOther(instance, LFU2_K_Z_Generator(1, 10), "lfu2_K1_Z10", "lru"));
        BOOST_TEST(compareToOther(instance, LFU2_K_Z_Generator(1, -10), "lfu2_K1_Z-10", "lru"));
        BOOST_TEST(compareToOther(instance, LFUalt_K_Generator(1), "lfu_K_ALT_1", "lru"));
        // BOOST_TEST(compareToOther(instance, LFU1_K_Z_D_Generator(1, 0, 10), "lfu_K1_Z0_D10", "lru"));
        // BOOST_TEST(compareToOther(instance, LFU1_K_Z_D_Generator(1, 10, 10), "lfu_K1_Z10_D10", "lru"));

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
    BOOST_AUTO_TEST_CASE(lfu_ks) {
        EvalAccessTable instance = init();
        // InRam History

        int K = 1;
        int Z = 0;
        runAlgo(instance, LFU_K_Generator(K), "lfu_K" + std::to_string(K));
        BOOST_TEST(compareToOther(instance, LFU_K_Z_Generator(K, Z), "lfu_K"+std::to_string(K)+"_Z"+std::to_string(Z), "lfu_K" + std::to_string(K)));
        BOOST_TEST(compareToOther(instance, LFU2_K_Z_Generator(K, Z), "lfu2_K" + std::to_string(K) + "_Z" + std::to_string(Z), "lfu_K" + std::to_string(K)));

        K = 2;
        Z = 0;
        runAlgo(instance, LFU_K_Generator(K), "lfu_K" + std::to_string(K));
        BOOST_TEST(compareToOther(instance, LFU_K_Z_Generator(K, Z), "lfu_K"+std::to_string(K)+"_Z"+std::to_string(Z), "lfu_K" + std::to_string(K)));
        BOOST_TEST(compareToOther(instance, LFU2_K_Z_Generator(K, Z), "lfu2_K" + std::to_string(K) + "_Z" + std::to_string(Z), "lfu_K" + std::to_string(K)));

        K = 10;
        Z = 0;
        runAlgo(instance, LFU_K_Generator(K), "lfu_K" + std::to_string(K));
        BOOST_TEST(compareToOther(instance, LFU_K_Z_Generator(K, Z), "lfu_K"+std::to_string(K)+"_Z"+std::to_string(Z), "lfu_K" + std::to_string(K)));
        BOOST_TEST(compareToOther(instance, LFU2_K_Z_Generator(K, Z), "lfu2_K" + std::to_string(K) + "_Z" + std::to_string(Z), "lfu_K" + std::to_string(K)));

        // Out of Ram History
        K = 1;
        Z = 1;
        runAlgo(instance, LFU_K_Z_Generator(K, Z), "lfu_K"+std::to_string(K)+"_Z"+std::to_string(Z));
        BOOST_TEST(compareToOther(instance, LFU2_K_Z_Generator(K, Z), "lfu2_K" + std::to_string(K) + "_Z" + std::to_string(Z), "lfu_K" + std::to_string(K) + "_Z" + std::to_string(Z)));

        K = 1;
        Z = 10;
        runAlgo(instance, LFU_K_Z_Generator(K, Z), "lfu_K"+std::to_string(K)+"_Z"+std::to_string(Z));
        BOOST_TEST(compareToOther(instance, LFU2_K_Z_Generator(K, Z), "lfu2_K" + std::to_string(K) + "_Z" + std::to_string(Z), "lfu_K" + std::to_string(K) + "_Z" + std::to_string(Z)));

        K = 2;
        Z = 10;
        runAlgo(instance, LFU_K_Z_Generator(K, Z), "lfu_K"+std::to_string(K)+"_Z"+std::to_string(Z));
        BOOST_TEST(compareToOther(instance, LFU2_K_Z_Generator(K, Z), "lfu2_K" + std::to_string(K) + "_Z" + std::to_string(Z), "lfu_K" + std::to_string(K) + "_Z" + std::to_string(Z)));

        K = 2;
        Z = -10;
        runAlgo(instance, LFU_K_Z_Generator(K, Z), "lfu_K"+std::to_string(K)+"_Z"+std::to_string(Z));
        BOOST_TEST(compareToOther(instance, LFU2_K_Z_Generator(K, Z), "lfu2_K" + std::to_string(K) + "_Z" + std::to_string(Z), "lfu_K" + std::to_string(K) + "_Z" + std::to_string(Z)));

        K = 10;
        Z = 10;
        runAlgo(instance, LFU_K_Z_Generator(K, Z), "lfu_K"+std::to_string(K)+"_Z"+std::to_string(Z));
        BOOST_TEST(compareToOther(instance, LFU2_K_Z_Generator(K, Z), "lfu2_K" + std::to_string(K) + "_Z" + std::to_string(Z), "lfu_K" + std::to_string(K) + "_Z" + std::to_string(Z)));
        // BOOST_TEST(compareToOther(instance, LFU1_K_Z_D_Generator(K, Z, 10), "lfu2_K" + std::to_string(K) + "_Z" + std::to_string(Z) + "_D10", "lfu_K" + std::to_string(K) + "_Z" + std::to_string(Z)));
 }
    BOOST_AUTO_TEST_CASE(opt) {
        EvalAccessTable instance = init();
        runAlgo(instance, Opt_Generator(), "opt");
        BOOST_TEST(compareToOther(instance,  Opt2_Generator(), "opt2", "opt"));
        BOOST_TEST(compareToOther(instance,  Opt3_Generator(), "opt3", "opt"));
    }
BOOST_AUTO_TEST_SUITE_END()
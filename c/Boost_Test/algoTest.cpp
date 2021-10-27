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
bool compareTwoAlgos(EvalAccessTable& instance, std::vector<uInt>& reads, std::vector<uInt>& writes, std::string name, std::vector<int> args = {}){
    instance.runAlgorithm<algo>(name, args);
    return (instance.getReads(name) == reads) && (instance.getWrites(name) == writes);

}

BOOST_AUTO_TEST_SUITE(compare_algo)
    BOOST_AUTO_TEST_CASE(lru) {
        EvalAccessTable test = init();

        std::vector<uInt> lru_reads, lru_writes;
        lru_reads = test.getReads("lru");
        lru_writes = test.getWrites("lru");

        BOOST_TEST(compareTwoAlgos<LRU>(test, lru_reads, lru_writes, "lru_0"));
        BOOST_TEST(compareTwoAlgos<LRU1>(test, lru_reads, lru_writes, "lru_1"));
        BOOST_TEST(compareTwoAlgos<LRU2>(test, lru_reads, lru_writes, "lru_2"));
        BOOST_TEST(compareTwoAlgos<LRU2a>(test, lru_reads, lru_writes, "lru_2a"));
        BOOST_TEST(compareTwoAlgos<LRU2b>(test, lru_reads, lru_writes, "lru_2b"));
        BOOST_TEST(compareTwoAlgos<LRU3>(test, lru_reads, lru_writes, "lru_3"));
    }
    BOOST_AUTO_TEST_CASE(lru_oter) {
        EvalAccessTable test = init();

        std::vector<uInt> lru_reads, lru_writes;
        lru_reads = test.getReads("lru");
        lru_writes = test.getWrites("lru");
        BOOST_TEST(compareTwoAlgos<LRU_K>(test, lru_reads, lru_writes, "lru_K_0", {1, 0}));
        BOOST_TEST(compareTwoAlgos<LRU_K_alt>(test, lru_reads, lru_writes, "lru_K2_0", {1}));
        // BOOST_TEST(compareTwoAlgos<LFU_K<>>(test, lru_reads, lru_writes, "lfu_K_0", {1}));
        // BOOST_TEST(compareTwoAlgos<LFU_K_Z<>>(test, lru_reads, lru_writes, "lfu_K_0", {1, 0}));

    }
BOOST_AUTO_TEST_SUITE_END()
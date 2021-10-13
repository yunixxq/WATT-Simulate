//
// Created by dev on 13.10.21.
//

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include "../algos/lfu_k.hpp"

BOOST_AUTO_TEST_SUITE(LFU_K_suite)
    BOOST_AUTO_TEST_CASE(frequencys){
        std::vector<std::list<RefTime>> testers;
        testers.push_back({1});
        testers.push_back({2,1});
        testers.push_back({3,2,1});
        testers.push_back({4,3,2,1});
        std::vector<double> result0 = {0, 1/(double)5, 2/(double)5, 3/(double)5};
        std::vector<double> result1 = {1/(double)5, 2/(double)5, 3/(double)5, 4/(double)5};
        std::vector<double> result2 = {2/(double)5, 3/(double)5, 4/(double)5, 5/(double)5};

        std::vector K_0 ={LFU_K_ALL<0,0>::get_frequency, LFU_K_ALL<1,0>::get_frequency, LFU_K_ALL<2,0>::get_frequency};
        std::vector K_1 ={LFU_K_ALL<0,1>::get_frequency, LFU_K_ALL<1,1>::get_frequency, LFU_K_ALL<2,1>::get_frequency};
        std::vector K_2 ={LFU_K_ALL<0,2>::get_frequency, LFU_K_ALL<1,2>::get_frequency, LFU_K_ALL<2,2>::get_frequency};
        for(int i=0; i<4; i++){
            double freq;
            for(auto algo: K_0) {
                freq = algo(testers[i], 6);
                BOOST_CHECK_EQUAL(freq, result0[i]);
            }
            for(auto algo: K_1) {
                freq = algo(testers[i], 6);
                BOOST_CHECK_EQUAL(freq, result1[i]);
            }
            for(auto algo: K_2) {
                freq = algo(testers[i], 6);
                BOOST_CHECK_EQUAL(freq, result2[i]);
            }
        }
    }
BOOST_AUTO_TEST_SUITE_END()
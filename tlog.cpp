#include <iostream>
#include <tuple>
#include <thread>
#include <functional>
#include <future>

#include "log11.hpp"

int main()
{

    Log11 log;

    std::thread th_a([&log](){
        for (unsigned int i = 0; i < 10000; i++)
        {
            log.info("A", i, "->", 3.14);
        }
    });


    std::thread th_b([&log](){
        for (unsigned int i = 0; i < 10000; i++)
        {
            log.info("B", i, "->", 3.14, ":)");
        }
    });


    std::thread th_c([&log](){
        for (unsigned int i = 0; i < 10000; i++)
        {
            log.info("C", i, "->", 3.14, 5, "alfa");
        }
    });

    th_a.join();
    th_b.join();
    th_c.join();

    return 0;
}

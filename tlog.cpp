#include <iostream>
#include <tuple>
#include <thread>
#include <functional>
#include <future>
#include <fstream>

#include "log11.hpp"


struct FileLog
{
    Log11 log;

    FileLog()
    {
        myout.open("mylog.txt", std::ofstream::out);

        log.setLogCall([this](std::string s){
            std::unique_lock<std::mutex> g{m};
            myout << s << std::endl;
        });        
    }

    ~FileLog()
    {
        log.close();
        myout.close();
    }

    std::ofstream myout;
    std::mutex m;
};

void test1()
{
    FileLog flog;
    std::mutex m;
 
    std::thread th_a([&](){
        for (unsigned int i = 0; i < 10000; i++)
        {
            std::unique_lock<std::mutex> g{m};
            flog.log.info("A", i, "->", 3.14);
        }
    });


    std::thread th_b([&](){
        for (unsigned int i = 0; i < 10000; i++)
        {
            std::unique_lock<std::mutex> g{m};
            flog.log.info("B", i, "->", 3.14, ":)");
        }
    });


    std::thread th_c([&](){
        for (unsigned int i = 0; i < 10000; i++)
        {
            std::unique_lock<std::mutex> g{m};
            flog.log.debug("C", i, "->", 3.14, 5, "alfa", -1);
        }
    });

    th_a.join();
    th_b.join();
    th_c.join();

    std::cerr << "test1 completed" << std::endl;
}

void test2(Log11 log)
{
    std::thread th_a([&log](){
        for (unsigned int i = 0; i < 10000; i++)
        {
            log.info("A", i, "->", 3.14);
        }
    });


    std::thread th_b([&log](){
        for (unsigned int i = 0; i < 10000; i++)
        {
            log.info("B", i, "->", 2.7, ":)");
        }
    });


    std::thread th_c([&log](){
        for (unsigned int i = 0; i < 10000; i++)
        {
            log.debug("C", i, "->", "alfa", 1982);
        }
    });

    th_a.join();
    th_b.join();
    th_c.join();

    std::cout << "DONE" << std::endl;
}


int main()
{
    test1();
    //test2(std::move(Log11()));
    return 0;
}

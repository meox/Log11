#include <iostream>
#include <tuple>
#include <thread>
#include <functional>
#include <future>
#include <fstream>

#include "log11.hpp"


struct FileLog
{
    FileLog()
    {
        myout = std::make_shared<std::ofstream>("mylog.txt", std::ofstream::out);

        log.setLogCall([this](const std::string& s){
            std::unique_lock<std::mutex> g{m};
            *myout << s << std::endl;
        });
    }

    ~FileLog()
    {
        log.close();
        myout->flush();
        myout->close();
    }

    std::mutex m;
    std::shared_ptr<std::ofstream> myout;
    Log11 log;
};

void test1()
{
    FileLog flog;
    
    std::thread th_a([&](){
        for (unsigned int i = 0; i < 10000; i++)
        {
            flog.log.info("A", i, "->", 3.14);
        }
    });

    std::thread th_b([&](){
        for (unsigned int i = 0; i < 10000; i++)
        {
            flog.log.info("B", i, "->", 3.14, ":)");
        }
    });

    std::thread th_c([&](){
        for (unsigned int i = 0; i < 10000; i++)
        {
            flog.log.info("C", i, "->", 3.14, 5, "alfa", -1);
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

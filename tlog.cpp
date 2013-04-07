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
        log.setLogInit([=](){
             out.open("mylog.txt");
        });

        log.setLogCall([=](const std::string& s){
             out << s << std::endl;
        });        
    }

    ~FileLog()
    {
        log.close();
        out.close();
    }

    std::ofstream out;
};

void test1()
{

    FileLog flog;
 
    std::thread th_a([&flog](){
        for (unsigned int i = 0; i < 10000; i++)
        {
            flog.log.info("A", i, "->", 3.14);
        }
    });


    std::thread th_b([&flog](){
        for (unsigned int i = 0; i < 10000; i++)
        {
            flog.log.info("B", i, "->", 3.14, ":)");
        }
    });


    std::thread th_c([&flog](){
        for (unsigned int i = 0; i < 10000; i++)
        {
            flog.log.debug("C", i, "->", 3.14, 5, "alfa");
        }
    });

    th_a.join();
    th_b.join();
    th_c.join();
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
            log.info("B", i, "->", 3.14, ":)");
        }
    });


    std::thread th_c([&log](){
        for (unsigned int i = 0; i < 10000; i++)
        {
            log.debug("C", i, "->", 3.14, 5, "alfa");
        }
    });

    th_a.join();
    th_b.join();
    th_c.join();
}


int main()
{
    test2(std::move(Log11()));
    return 0;
}

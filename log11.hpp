#include <iostream>
#include <thread>
#include <future>
#include <functional>
#include <tuple>
#include <sstream>
#include <fstream>
#include <vector>
#include <map>
#include <chrono>
#include <deque>
#include <array>

class Log11
{
    struct Worker
    {
        Worker() : done{false} {}
        Worker(const Worker& rhs) : done{rhs.done}, qworker{} {}

        void finish() { done = true; }

        void run()
        {
            std::chrono::milliseconds d( 100 );

            while(!done)
            {
                mw.lock();
                if(!qworker.empty())
                {
                    auto f = qworker.front();
                    qworker.pop_front();
                    mw.unlock();

                    //execute f, there is only 1 thread that execute qworker so it's implicit thread safe
                    f();
                }
                else
                {
                    mw.unlock();
                    std::this_thread::sleep_for(d);
                }
            }
        }

        void push(const std::function<void()>& f)
        {
            mw.lock();
            qworker.push_back(f);
            mw.unlock();
        }

    private:
        bool done;
        std::mutex mw;
        std::deque<std::function<void()>> qworker;
    };

public:
    enum class Level { DEBUG, INFO, WARNING, ERROR, FATAL };

    Log11() :
            isInit{false},
            c_level{Level::DEBUG},
            sep_{" "},
            fmt_date{"%Y-%m-%d %H:%M:%S"},
            worker{},
            fn_loginit{ [](){} },
            fn_logcall{ [](const std::string& t){ std::cout << t << std::endl; } }
    {
        fmt_date_len = fmt_date.size();
        thw = std::thread([=](){ worker.run(); });
    }


    Log11(const Log11& rhs) :
            isInit{rhs.isInit},
            c_level{rhs.c_level},
            sep_{rhs.sep_},
            fmt_date{rhs.fmt_date},
            worker{rhs.worker},
            fn_loginit{ rhs.fn_loginit },
            fn_logcall{ rhs.fn_logcall }
    {
        fmt_date_len = fmt_date.size();
        thw = std::thread([=](){ worker.run(); });
    }


    Log11& sep(const std::string& separator)
    {
        sep_ = separator;
        return *this;
    }

    Log11& setDateFmt(const std::string& ft)
    {
        fmt_date = ft;
        fmt_date_len = fmt_date.size();
        return *this;
    }
   
    Log11& setLevel(const Level& l)
    {
        c_level = l;
        return *this;
    }

    Level getLevel() const { return c_level; }

    template <typename ...Ts>
    void debug(Ts&&... args)
    {
        if(c_level <= Level::DEBUG)
        {
            checkInit();

            auto& st = getStr();
            writeDate(st);
            st += "DEBUG: ";
            print(st, std::forward<Ts>(args)...);
        }
    }

    template <typename ...Ts>
    void info(Ts&&... args)
    {
        if(c_level <= Level::INFO)
        {
            checkInit();

            auto& st = getStr();
            writeDate(st);
            st += "INFO: ";
            print(st, std::forward<Ts>(args)...);
        }
    }

    template <typename ...Ts>
    void warn(Ts&&... args)
    {
        if(c_level <= Level::WARNING)
        {
            checkInit();

            auto& st = getStr();
            writeDate(st);
            st += "WARN: ";
            print(st, std::forward<Ts>(args)...);
        }
    }

    template <typename ...Ts>
    void error(Ts&&... args)
    {
        if(c_level <= Level::ERROR)
        {
            checkInit();

            auto& st = getStr();
            writeDate(st);
            st += "ERROR: ";
            print(st, std::forward<Ts>(args)...);
        }
    }

    template <typename ...Ts>
    void fatal(Ts&&... args)
    {
        if(c_level <= Level::FATAL)
        {
            checkInit();

            auto& st = getStr();
            writeDate(st);
            st += "FATAL: ";
            print(st, std::forward<Ts>(args)...);
        }
    }


    template <typename Fun>
    void setLogInit(Fun f) { fn_loginit = f; }

    template <typename Fun>
    void setLogCall(Fun f) { fn_logcall = f; }

  
    virtual ~Log11()
    {
        worker.push([=]{ worker.finish(); });
        thw.join();
    }

private:
    template <typename R, typename ...Ts>
    void print(std::string& st, R&& r, Ts&&... args)
    {
        std::stringstream s;
        s << st << r << sep_;

        st = s.str();        
        print(st, std::forward<Ts>(args)...);
    }


    template <typename R>
    void print(std::string& st, R&& r)
    {
        std::stringstream s;
        s << st << r;

        st = s.str();
        build(st);
    }

    std::string& getStr()
    {
        std::hash<std::thread::id> hash_fn;
        auto id = std::this_thread::get_id();
        auto index_map = (hash_fn(id) % QUEUE_SIZE);

        std::lock_guard<std::mutex> g{mutex_map[index_map]};

        auto f_id = map_m[index_map].find(id);
        if(f_id == map_m[index_map].end())
        {
            auto nid = map_m[index_map].insert(std::pair<std::thread::id, std::string>(id, std::string{}));
            return nid.first->second;
        }
        else
        {
            return f_id->second;
        }
    }

    void writeDate(std::string& str)
    {
        time_t t;
        char buff[256];

        if(fmt_date_len > 0)
        {
            time(&t);

            if (std::strftime(buff, 256, fmt_date.c_str(), std::localtime(&t)) != 0)
            {
                str += std::string{buff} + " - ";
            }
        }
    }

    void checkInit()
    {
        if(!isInit)
        {
            isInit = true;
            fn_loginit();
        }
    }

    void build(std::string &str)
    {
        std::string tlog {str};

        // reset stream
        str.clear();

        // insert
        worker.push([=]{ fn_logcall(tlog); });
    }


private:
    typedef std::map<std::thread::id, std::string> MAP_THREAD;
    enum { QUEUE_SIZE = 10};

    bool isInit;

    Level c_level;
    std::string sep_;
    std::string fmt_date;

    Worker worker;
    std::function<void()> fn_loginit;
    std::function<void(const std::string& str)> fn_logcall;

    std::array<std::mutex, QUEUE_SIZE> mutex_map;
    std::array<MAP_THREAD, QUEUE_SIZE> map_m;

    size_t fmt_date_len;
    std::thread thw;
};

#include <unistd.h>
#include <iostream>
#include <algorithm>
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
        Worker(Worker &&rhs) : done{rhs.done}, qworker(std::move(rhs.qworker))
        {}

        void finish() { done = true; }
        void flush()
        {
            bool f = false;
            while(!f)
            {
                mw.lock(); f = qworker.empty(); mw.unlock();
                if(f) break;
                usleep(100000);
            }
        }

        void run()
        {
            //std::chrono::milliseconds d( 100 );

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
                    usleep(100000);
                    //std::this_thread::sleep_for(d);
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
    enum class Level { LEVEL_DEBUG, LEVEL_INFO, LEVEL_WARNING, LEVEL_ERROR, LEVEL_FATAL };

    Log11() :
            isInit{false},
            isClose{false},
            c_level{Level::LEVEL_DEBUG},
            sep_{" "},
            fmt_date{"%Y-%m-%d %H:%M:%S"},
            fmt_date_len{fmt_date.size()},
            worker{},
            fn_loginit{ [](){} },
            fn_logcall{ [](const std::string& t){ std::cout << t << std::endl; } },
            current_stream(&debug_stream)
    {
        thw = std::thread([=](){ worker.run(); });
    }


    /**
     * Copy constructor
     */
    Log11(const Log11& rhs) :
            isInit{rhs.isInit},
            isClose{rhs.isClose},
            c_level{rhs.c_level},
            sep_{rhs.sep_},
            fmt_date{rhs.fmt_date},
            fmt_date_len{rhs.fmt_date_len},
            worker{rhs.worker},
            fn_loginit{ rhs.fn_loginit },
            fn_logcall{ rhs.fn_logcall },
            current_stream(&debug_stream)
    {
        thw = std::thread([=](){ worker.run(); });
    }


    /**
     * Move constructor
     */
    Log11(Log11&& rhs) :
            isInit{rhs.isInit},
            isClose{rhs.isClose},
            c_level{rhs.c_level},
            sep_{std::move(rhs.sep_)},
            fmt_date{std::move(rhs.fmt_date)},
            fmt_date_len{rhs.fmt_date_len},
            worker{},
            fn_loginit{ std::move(rhs.fn_loginit) },
            fn_logcall{ std::move(rhs.fn_logcall) },
            current_stream(&debug_stream)
    {
        rhs.wait();
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
    void print_level(const std::string& tag, Ts&&... args)
    {
        checkInit();

        auto& st = getStr();
        writeDate(st);
        st += tag;
        print(st, std::forward<Ts>(args)...);
    }


    template <typename ...Ts>
    Log11& debug(Ts&&... args)
    {
        if(c_level <= Level::LEVEL_DEBUG)
        {
            print_level("DEBUG: ", std::forward<Ts>(args)...);
        }

        return *this;
    }


    template <typename ...Ts>
    Log11& info(Ts&&... args)
    {
        if(c_level <= Level::LEVEL_INFO)
        {
            print_level("INFO: ", std::forward<Ts>(args)...);
        }

        return *this;
    }


    template <typename ...Ts>
    Log11& warn(Ts&&... args)
    {
        if(c_level <= Level::LEVEL_WARNING)
        {
            print_level("WARN: ", std::forward<Ts>(args)...);
        }

        return *this;
    }


    template <typename ...Ts>
    Log11& error(Ts&&... args)
    {
        if(c_level <= Level::LEVEL_ERROR)
        {
            print_level("ERROR: ", std::forward<Ts>(args)...);
        }

        return *this;
    }


    template <typename ...Ts>
    Log11& fatal(Ts&&... args)
    {
        if(c_level <= Level::LEVEL_FATAL)
        {
            print_level("FATAL: ", std::forward<Ts>(args)...);
        }

        return *this;
    }


    Log11& debugStream()
    {
        writeStream("DEBUG: ", debug_stream);

        return *this;
    }

    Log11& infoStream()
    {
        writeStream("INFO: ", info_stream);

        return *this;
    }

    Log11& errorStream()
    {
        writeStream("ERROR: ", error_stream);

        return *this;
    }

    Log11& warnStream()
    {
        writeStream("WARN: ", warn_stream);

        return *this;
    }


    Log11& writeStream(const std::string tag, std::stringstream& pstream)
    {
        if(current_stream != &pstream)
        {
            flush_stream();            
        }

        current_stream = &pstream;
        std::string s{};
        writeDate(s);
        (*current_stream) << s << tag;
        return *this;
    }


    template<typename D>
    Log11& operator<<(const D& v)
    {
        (*current_stream) << v;
        return *this;
    }


    template <typename Fun>
    void setLogInit(Fun f) { fn_loginit = f; }

    template <typename Fun>
    void setLogCall(Fun f) { fn_logcall = f; }


    void flush() { worker.flush(); }

    void flush_stream()
    {
        auto fun_stream = [this](std::stringstream* st) {
            auto s = st->str();
            st->str("");
            st->clear();

            if (s.size() > 0)
            {
                this->build(s);
            }
        };

        auto ctn = std::vector<std::stringstream*>{&debug_stream, &warn_stream, &error_stream, &info_stream};
        std::for_each(ctn.begin(), ctn.end(), fun_stream);
    }


    void wait()
    {
        if (!isClose)
        {
            isClose = true;
            flush_stream();
            worker.push([=]{ worker.finish(); });
            thw.join();
        }
    }
  
    virtual ~Log11()
    {
        wait();
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
        flush_stream();
        std::stringstream s;
        s << st << r;

        st = s.str();
        build(st);
        st.clear();
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


    void build(const std::string &str)
    {
        // insert
        worker.push([&, str](){ fn_logcall(str); });
    }


private:
    typedef std::map<std::thread::id, std::string> MAP_THREAD;
    enum { QUEUE_SIZE = 10};

    bool isInit, isClose;

    Level c_level;
    std::string sep_;
    std::string fmt_date;
    size_t fmt_date_len;

    Worker worker;
    std::function<void()> fn_loginit;
    std::function<void(const std::string& str)> fn_logcall;

    std::array<std::mutex, QUEUE_SIZE> mutex_map;
    std::array<MAP_THREAD, QUEUE_SIZE> map_m;

    std::thread thw;

    std::stringstream debug_stream, info_stream, warn_stream, error_stream;
    std::stringstream *current_stream;
};

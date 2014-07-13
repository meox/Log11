#ifndef __LOG11HPP__
#define __LOG11HPP__


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
        Worker() : done{false}, isclosed{false}
        {
            thw = std::thread([this] { run(); });
        }

        Worker(const Worker& rhs) = delete;

        Worker(Worker &&rhs) : done{false}, isclosed{false}
        {
            rhs.close();
            qworker = std::move(rhs.qworker);
            thw = std::thread([this]{ run(); });
        }

        void finish() { push([this]{done = true;}); }
        void flush()
        {
            while([this]{
                std::unique_lock<std::mutex> g{mw};
                if (qworker.empty()) return false;
                else
                {
                    g.unlock();
                    std::this_thread::sleep_for(duration);
                    return true;
                }
            }())
            {}
        }

        void run()
        {
            while (!done)
            {
                std::unique_lock<std::mutex> g{mw};
                if (!qworker.empty())
                {
                    auto f = move(qworker.front());
                    qworker.pop_front();
                    g.unlock();

                    f();
                }
                else
                {
                    g.unlock();
                    std::this_thread::sleep_for(duration);
                }
            }
        }

        template <typename F>
        void push(F f)
        {
            std::unique_lock<std::mutex> g{mw};
            qworker.push_back(std::forward<F>(f));
        }

        void close()
        {
            std::unique_lock<std::mutex> g{_internal_mutex};
            if (!isclosed)
            {
                finish();
                thw.join();
                isclosed = true;
            }
        }

        ~Worker()
        {
            close();
        }

    private:
        bool done, isclosed;
        mutable std::mutex mw, _internal_mutex;
        std::deque<std::function<void()>> qworker;
        const std::chrono::milliseconds duration{100};
        std::thread thw;
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
            fut_loginit{std::async(std::launch::deferred, []{})},
            fn_logcall{[](std::string t){ std::cout << t << std::endl; }},
            current_stream{&debug_stream}
    {
    }


    /**
     * Delete copy constructor
     */
    Log11(const Log11& rhs) = delete;


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
            fut_loginit{std::async(std::launch::deferred, []{})},
            fn_logcall{std::move(rhs.fn_logcall)},
            current_stream{&debug_stream}
    {
        rhs.close();
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

        std::string st{};
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

    Log11& critStream()
    {
        writeStream("CRITIC: ", error_stream);
        return *this;
    }

    Log11& warnStream()
    {
        writeStream("WARN: ", warn_stream);
        return *this;
    }


    Log11& writeStream(const std::string tag, std::stringstream& pstream)
    {
        flush_stream();            

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
    void setLogInit(Fun f)
    { 
        fut_loginit = std::async(std::launch::deferred, [f]{ f(); });
    }

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


    void close()
    {
        std::unique_lock<std::mutex> g{_internal_mutex};
        if (!isClose)
        {
            isClose = true;
            flush_stream();
            worker.close();
        }
    }
  
    virtual ~Log11()
    {
        close();
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
        build(s.str());
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
            fut_loginit.wait();
            isInit = true;
        }
    }


    void build(std::string str)
    {
        // insert
        if (isInit)
        {
            worker.push([this, str]{ fn_logcall(str); });
        }
    }


private:
    bool isInit, isClose;

    Level c_level;
    std::string sep_;
    std::string fmt_date;
    size_t fmt_date_len;

    Worker worker;
    std::future<void> fut_loginit;
    std::function<void(const std::string& str)> fn_logcall;

    std::stringstream debug_stream, info_stream, warn_stream, error_stream;
    std::stringstream *current_stream;
    std::mutex _internal_mutex;
};

#endif

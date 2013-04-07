Log11
=====

A simple C++11 log thread-safe class.

## Usage

```
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
```

## Function list

  * setLevel ( const Log11::Level& l ) // set new level { DEBUG, INFO, WARN, ERROR, FATAL }
  * sep (  ) // set separator (default: " ")
  
  * debug ( ... ) // log message at debug level
  * info ( ... )  // log message at info level
  * warn ( ... )  // log message at info level
  * error ( ... ) // log message at error level
  * fatal ( ... ) // log message at fatal level

  * setLogInit(Fun f) // set the function callback used by Log11 for first call
  * setLogCall(Fun f) // set the function callback used by Log11 for log

## Advanced example
Using a file to log. We can define a simple class like this:

```
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
            log.close(); // close safety the log
            out.close(); // close the file
        }
    
        std::ofstream out;
    };
    
    int main()
    {
        FileLog flog;
        
        flog.log.info("Hello", std::string{"World!"});
        
        return 0;
    }
```
In the constructor we set our callback: setLogInit and setLogCall, after in the main function we can use the log as usual
## Compile

g++ -Wall -O3 --std=c++11 tlog.cpp -o tlog -lpthread

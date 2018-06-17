#ifndef LOGGER_H
#define LOGGER_H

#include <string>

class Logger
{
protected:
    inline Logger(){}

public:
    static Logger* make();

    enum LogLevel{
        All,
        Info,
        Warning,
        Error
    };

    inline virtual ~Logger(){}

    inline void logMessage(const LogLevel level, const std::string& message);

protected:
    virtual void logMessageInternal(const LogLevel level, const std::string& message) = 0;
};

constexpr auto logLevel = Logger::Info;

inline void Logger::logMessage(const LogLevel level, const std::string& message)
{
    if (level>=logLevel)
        logMessageInternal(level,message);
}

#endif // LOGGER_H

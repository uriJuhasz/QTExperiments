#include "Logger.h"

using std::string;

#include <QDebug>

class LoggerImpl : public Logger
{
    static string toString(const LogLevel level)
    {
        switch (level)
        {
            case Info : return "I";
        case Warning : return "W";
        case Error : return "E";
        default: return "";
        }
    }
    void logMessageInternal(const LogLevel level, const std::string &message) override
    {
        qDebug() << toString(level).c_str() << ": " << message.c_str();
    }
};

Logger* Logger::make()
{
    return new LoggerImpl();
}

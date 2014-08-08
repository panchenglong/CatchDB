#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string>

namespace catchdb
{

class Logger
{
public:
    enum LogLevel
    {
        LEVEL_OFF = 0,
        LEVEL_FATAL = 1,
        LEVEL_ERROR = 2,
        LEVEL_WARNING = 3,
        LEVEL_INFO = 4,
        LEVEL_DEBUG = 5,
        LEVEL_ALL = 6
    };

    static Logger& GetLogger();

    int open(const std::string &logFileName);
    void close();

    void setLevel(LogLevel level) { level_ = level; }
    LogLevel getLevel() const { return level_; }
    static LogLevel getLevelFromString(const char *levelString);

    void logv(LogLevel level, const char *format, va_list ap);

private:
    Logger():file_(stdout), level_(LEVEL_DEBUG){}

    ~Logger()
    { 
        if (file_ && file_ != stdout  && file_ != stderr)
            fclose(file_);
    }


    FILE *file_;
    LogLevel level_;
};

int OpenLog(const std::string &logFileName);
void CloseLog();
void SetLogLevel(Logger::LogLevel level);
void LogDebug(const char *format, ...);
void LogInfo(const char *format, ...);
void LogWarning(const char *format, ...);
void LogError(const char *format, ...);
void LogFatal(const char *format, ...);

} // namespace catchdb

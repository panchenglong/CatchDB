#include "Logger.h"
#include <string.h>
#include <sys/time.h>
#include <cassert>

#define MAX_LOG_MESSAGE_LENGTH 1024

namespace catchdb
{

namespace
{
const char *logLevelStrings[] = { "", "FATAL", "ERROR", "WARNING", "INFO", "DEBUG", "" };
} // namespace

Logger& Logger::GetLogger()
{
    static Logger logger;
    return logger;
}

int Logger::open(const std::string &logFileName)
{
    file_ = fopen(logFileName.c_str(), "a");
    if (file_ == nullptr) {
        perror("open log file");
        file_ = stderr;
        return -1;
    }

    return 0;
}

void Logger::close()
{
    if (file_ != stdout && file_ != stderr)
        fclose(file_);
}

Logger::LogLevel Logger::getLevelFromString(const char *levelString)
{
    if (strncmp("debug", levelString, 5) == 0)
        return LEVEL_DEBUG;
    if (strncmp("info", levelString, 4) == 0)
        return LEVEL_INFO;
    if (strncmp("warning", levelString, 7) == 0)
        return LEVEL_WARNING;
    if (strncmp("error", levelString, 5) == 0)
        return LEVEL_ERROR;
    if (strncmp("fatal", levelString, 5) == 0)
        return LEVEL_FATAL;
    return LEVEL_DEBUG;
}

void Logger::logv(LogLevel level, const char *format, va_list ap)
{
    if (level > level_)
        return;
 
   char buffer[MAX_LOG_MESSAGE_LENGTH];

    for (int iter = 0; iter < 2; iter++) {
        char *base;
        int bufsize;
        if (iter == 0) {
            bufsize = sizeof(buffer);
            base = buffer;
        } else {
            bufsize = 30000;
            base = new char[bufsize];
        }
        char *p = base;
        char *limit = base + bufsize;

        struct timeval now_tv;
        gettimeofday(&now_tv, NULL);
        const time_t seconds = now_tv.tv_sec;
        struct tm t;
        localtime_r(&seconds, &t);
        p += snprintf(p, limit - p,
                      "%04d/%02d/%02d-%02d:%02d:%02d.%06d [%s] \t",
                      t.tm_year + 1900,
                      t.tm_mon + 1,
                      t.tm_mday,
                      t.tm_hour,
                      t.tm_min,
                      t.tm_sec,
                      static_cast<int>(now_tv.tv_usec),
                      logLevelStrings[level]);

        // Print the message
        if (p < limit) {
            va_list backup_ap;
            va_copy(backup_ap, ap);
            p += vsnprintf(p, limit - p, format, backup_ap);
            va_end(backup_ap);
        }

        // Truncate to available space if necessary
        if (p >= limit) {
            if (iter == 0) {
                continue;       // Try again with larger buffer
            } else {
                p = limit - 1;
            }
        }

        // Add newline if necessary
        if (p == base || p[-1] != '\n') {
            *p++ = '\n';
        }

        assert(p <= limit);
        fwrite(base, 1, p - base, file_);
        fflush(file_);
        if (base != buffer) {
            delete[] base;
        }
        break;
    }
}

int OpenLog(const std::string &logFileName)
{
    return Logger::GetLogger().open(logFileName);
}

void CloseLog()
{
    Logger::GetLogger().close();
}

void SetLogLevel(Logger::LogLevel level)
{
    Logger::GetLogger().setLevel(level);
}

void LogFatal(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    Logger::GetLogger().logv(Logger::LEVEL_FATAL, format, ap);
    va_end(ap);
}

void LogError(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    Logger::GetLogger().logv(Logger::LEVEL_ERROR, format, ap);
    va_end(ap);
}

void LogWarning(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    Logger::GetLogger().logv(Logger::LEVEL_WARNING, format, ap);
    va_end(ap);
}

void LogInfo(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    Logger::GetLogger().logv(Logger::LEVEL_INFO, format, ap);
    va_end(ap);
}

void LogDebug(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    Logger::GetLogger().logv(Logger::LEVEL_DEBUG, format, ap);
    va_end(ap);
}

} // namespace catchdb

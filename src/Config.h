#pragma once

#include "Logger.h"
#include <string>
#include <memory>
#include <vector>

namespace catchdb
{
namespace
{

const Logger::LogLevel DEFAULT_LOG_LEVEL = Logger::LEVEL_DEBUG;
const std::string DEFAULT_PID_FILE = "/var/run/catchdb.pid";
const std::string DEFAULT_LOG_FILE = "./catchdb.log";
const std::string DEFAULT_PORT = "7777";
const int DEFAULT_MAX_CLIENTS = 10000;
const int DEFAULT_BACKLOG = 1024;
const std::string DEFAULT_DB_NAME = "catchdb";
const std::string DEFAULT_DB_PATH = "./catchdb/";
const int DEFAULT_CACHE_SIZE = 8;
const int DEFAULT_BLOCK_SIZE = 4;
const int DEFAULT_WRITE_BUFFER_SIZE = 4;
const int DEFAULT_COMPACTION_SPEED = 1000;

} // namespace

struct Config;
typedef std::shared_ptr<Config> ConfigPtr;

struct Config
{
    std::string dbName;
    std::string logFile;
    Logger::LogLevel logLevel;
    std::string pidFile;
    std::string port;
    int backlog;
    int maxClients;
    std::string dbPath;
    // leveldb config
    int cacheSize; // MB
    int blockSize; // KB
    int writeBufferSize; // MB
    int compactionSpeed; // MB
    bool compression;

    std::vector<std::string> bindAddresses;

    static ConfigPtr load(const std::string &configFileName);

    Config() 
        : dbName(DEFAULT_DB_NAME), 
          logFile(DEFAULT_LOG_FILE), 
          logLevel(DEFAULT_LOG_LEVEL), 
          pidFile(DEFAULT_PID_FILE),
          port(DEFAULT_PORT),
          backlog(DEFAULT_BACKLOG),
          maxClients(DEFAULT_MAX_CLIENTS),
          cacheSize(DEFAULT_CACHE_SIZE),
          blockSize(DEFAULT_BLOCK_SIZE),
          writeBufferSize(DEFAULT_WRITE_BUFFER_SIZE),
          compactionSpeed(DEFAULT_COMPACTION_SPEED),
          compression(false)
    {}
};



} // namespace catchdb

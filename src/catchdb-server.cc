#include <memory>
#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <getopt.h>
#include "Util.h"
#include "Logger.h"
#include "Config.h"
#include "EventManager.h"
#include "Networking.h"
#include "Protocol.h"
#include "Client.h"

using namespace catchdb;

namespace
{
// default values
const std::string DEFAULT_CONFIG_FILE = "./catchdb.conf";

// global variables
// std::queue<Command> commandQueue;

} // namespace

void WriteResultHandler(EventManager &em, int clientfd, void *data);
void ReadQueryHandler(EventManager &em, int clientfd, void *data);
void AcceptHandler(EventManager &em, int serverfd, void *data);

struct ServerOptions
{
    bool isDaemon;
    std::string configFile;

    ServerOptions() 
        : isDaemon(false),
          configFile(DEFAULT_CONFIG_FILE) {}
};

void PrintUsage(const std::string progName)
{
    printf("Usage:\n");
    printf("    %s [-d] [-c /path/to/catchdb.conf\n]", progName.c_str());
    printf("Options:\n");
    printf("    -d, --daemon             run as daemon\n");
    printf("    -c, --config filename    config file path\n");
}

ServerOptions ParseCommandLineOptions(int argc, char **argv)
{
    ServerOptions opts;

    const char* shortOptions = "c:dh";  
    struct option longOptions[] = {  
        { "daemon", 0, nullptr, 'd' },  
        { "help", 0, nullptr, 'h' },  
        { "config", 1, nullptr, 'c' },  
        { nullptr, 0, nullptr, 0},  
    };
    int c;
    while ((c = getopt_long(argc, argv, shortOptions, longOptions, nullptr)) != -1) {
        switch (c) {
            case '?':
                PrintUsage(argv[0]);
                exit(EXIT_FAILURE);
            case 'h':
                PrintUsage(argv[0]);
                break;
            case 'd':
                opts.isDaemon = true;
                break;
            case 'c':
                opts.configFile = optarg;
                break;
        }
    }

    return opts;
}

ConfigPtr InitConfig(const std::string &configFile)
{
    auto config = Config::load(configFile);
    if (config == nullptr) {
        fprintf(stderr, "Loading Config File %s Error\n", configFile.c_str());
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    return config;
}

void InitLogging(const std::string &logFile, Logger::LogLevel logLevel)
{
    OpenLog(logFile);
    SetLogLevel(logLevel);
}

void CheckPidFile(const std::string &pidFile)
{
    if (access(pidFile.c_str(), F_OK) == 0) {
        fprintf(stderr, "Pidfile %s already exists", pidFile.c_str());
        exit(EXIT_FAILURE);
    }
}

void CreatePidFile(const std::string &pidFile)
{
    FILE *fp = fopen(pidFile.c_str(), "w");
    if (fp) {
        fprintf(fp,"%d\n",(int)getpid());
        fclose(fp);
        LogInfo("Write Pidfile %s", pidFile.c_str());
    } else {
        LogWarning("Write Pidfile %s error", pidFile.c_str());
    }
}

void RemovePidFile(const std::string &pidFile)
{
    remove(pidFile.c_str());
}

void LogServerInfo()
{
    LogInfo("server started...");
}

// system should be configured to improve this process's max open file number
int GetOpenFileLimits()
{
    struct rlimit limit;

    if (getrlimit(RLIMIT_NOFILE, &limit) == -1) {
        LogError("Bacause %s, Unable to obtain the current NOFILE limit, assuming 1024.",
                   ErrorDescription(errno));
        return 1024;
    } else {
        struct rlimit newLimit;
        newLimit.rlim_cur = limit.rlim_max;
        newLimit.rlim_max = limit.rlim_max;
        if (setrlimit(RLIMIT_NOFILE, &newLimit) == 0) {
            return newLimit.rlim_cur;
        } else {
            return limit.rlim_cur;
        }
    }
}

std::vector<int> ListenToPort(const std::vector<std::string> &bindAddresses, 
                              const std::string &port,
                              int backlog)
{
    std::vector<int> fds;

    int fd;
    if (bindAddresses.empty()) {
        if ((fd = Listen("", port, backlog)) > 0) {
            fds.push_back(fd);
            LogInfo("Listening on 0.0.0.0:%s", port.c_str());
        }
    } else {
        for (auto &addr : bindAddresses) {
            if ((fd = Listen(addr, port, backlog)) > 0) {
                fds.push_back(fd);
                LogInfo("Listening on %s:%s", addr.c_str(), port.c_str());
            }
        }
    }

    return fds;
}

void SignalHandler(int sig)
{
    switch(sig){
        case SIGTERM:
        case SIGINT:{
//            RemovePidFile();
            break;
        }
        case SIGALRM:{

            break;
        }
    }
}

void SetupSignalHandler()
{
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    // signal(SIGINT, SignalHandler);
    // signal(SIGTERM, SignalHandler);
    // signal(SIGALRM, signal_handler);

    // setitimer
}

void WriteResultHandler(EventManager &em, int clientfd, void *data)
{
    ClientPtr client = Client::GetClient(clientfd);
    auto s = client->writeResult();
    if (s == Status::Error) {
        LogError("send error: %s. close connection %s:%d", 
                 ErrorDescription(errno),
                 client->getRemoteIPString().c_str(),
                 client->getRemotePort());
        em.delEvent(clientfd, EVENT_IN);
        Client::DestroyClient(clientfd);
        return;
    }

    if (s == Status::Progress)
        return;

    em.delEvent(clientfd, EVENT_OUT);
    Event event(EVENT_IN, ReadQueryHandler, data);
    em.addEvent(clientfd, event);
}

void ReadQueryHandler(EventManager &em, int clientfd, void *data)
{
    ClientPtr client = Client::GetClient(clientfd);
    auto s = client->processQuery(); 
    if (s == Status::Close) {
        LogError("close connection %s:%d", 
                 client->getRemoteIPString().c_str(),
                 client->getRemotePort());
        em.delEvent(clientfd, EVENT_IN);
        Client::DestroyClient(clientfd);
        return;

    } else if (s == Status::Error) {
        LogError("recv error: %s. close connection %s:%d", 
                 ErrorDescription(errno),
                 client->getRemoteIPString().c_str(),
                 client->getRemotePort());
        em.delEvent(clientfd, EVENT_IN);
        Client::DestroyClient(clientfd);
        return;
    } else if (s == Status::Progress) {
        return;
    }

    assert(s == Status::OK);

    // temporarily delete read event
    em.delEvent(clientfd, EVENT_IN);

    CatchDBPtr db = *((CatchDBPtr *)data);
    s = client->executeCommand(db);
    if (s == Status::Error) {
        // program will not reach here
    }
    Event event(EVENT_OUT, WriteResultHandler, data);
    em.addEvent(clientfd, event);
}

void AcceptHandler(EventManager &em, int serverfd, void *data)
{
    // data is CatchDBPtr

    int clientfd;
    struct sockaddr_storage sa;

    while(1) {
        socklen_t len = sizeof sa;
        clientfd = accept(serverfd, (struct sockaddr *)&sa, &len);
        if (clientfd == -1) {
            if (errno == EINTR)
                continue;
            else {
                LogError("accept: %s", ErrorDescription(errno));
                return;
            }
        }
        break;
    }

    // log connection

    char ipstr[INET6_ADDRSTRLEN];
    uint16_t port;
    if (sa.ss_family == AF_INET) {
        struct sockaddr_in *s = (struct sockaddr_in *)&sa;
        inet_ntop(AF_INET, (void*)&(s->sin_addr), ipstr, INET6_ADDRSTRLEN);
        port = ntohs(s->sin_port);
    } else {
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)&sa;
        inet_ntop(AF_INET6, (void*)&(s->sin6_addr), ipstr, INET6_ADDRSTRLEN);
        port = ntohs(s->sin6_port);
    }
    LogInfo("Accepted: %s:%d", ipstr, port);

    SetNonBlocking(clientfd);
    SetTcpNoDelay(clientfd);
    SetTcpKeepAlive(clientfd);

    (void) Client::CreateClient(clientfd, ipstr, port);

    Event e(EVENT_IN, ReadQueryHandler, data);
    if (em.addEvent(clientfd, e) == Status::Error) {
        close(clientfd);
        Client::DestroyClient(clientfd);
        LogError("Add Listening event for %s:%d Failed", ipstr, port);
    }
}

int main(int argc, char **argv)
{
    ServerOptions serverOptions = ParseCommandLineOptions(argc, argv);

    // init config
    ConfigPtr config = InitConfig(serverOptions.configFile);

    // check pidfile, if exits, then terminate process.
    CheckPidFile(config->pidFile);

    // init logging
    InitLogging(config->logFile, config->logLevel);

    // listening
    auto serverSocks = ListenToPort(config->bindAddresses, 
                                    config->port,
                                    config->backlog);
    if (serverSocks.empty()) {
        LogFatal("Cannot open server sockets");
        exit(EXIT_FAILURE);
    }

    // setup signal handler
    SetupSignalHandler();

    // logging server infomation
    LogServerInfo();

    // daemonize, using the library function
    if (serverOptions.isDaemon == true) {
        if (daemon(0, 0) == -1) {
            LogError("Daemonize error: %s", ErrorDescription(errno));
        }
    }

    // DB
    CatchDBPtr db = CatchDB::Open(config);
    if (db == nullptr) {
        LogFatal("can not open db");
        exit(EXIT_FAILURE);
    }

    // create pidfile 
    CreatePidFile(config->pidFile);

    int maxfds = GetOpenFileLimits();
    // Main Event Loop
    EventManager eventManager(maxfds);

    for (auto &fd : serverSocks) {
        Event e(EVENT_IN, AcceptHandler, &db);
        if (eventManager.addEvent(fd, e) != Status::OK) {
            LogFatal("Can't add listening event");
            exit(EXIT_FAILURE);
        }
    }

    LogInfo("Start event loop...");
    eventManager.run();

    // remove pidfile
    RemovePidFile(config->pidFile);
    return 0;
}

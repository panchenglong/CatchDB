#pragma once

#include <string>
#include <map>
#include <array>
#include <tuple>
#include <memory>
#include "HashMap.h"
#include "ZSet.h"
#include "Queue.h"
#include "Protocol.h"
#include "Status.h"
#include "Buffer.h"
#include "CatchDB.h"

namespace catchdb
{

namespace 
{
const int QUERY_BUF_SIZE = 8 * 1024;
const int REPLY_BUF_SIZE = 16 * 1024;
} // namespace

class Client;
typedef std::shared_ptr<Client> ClientPtr;

class Client
{
public:
    Client(int fd, const std::string &ipstr, uint16_t port);

    ~Client();

    // fd serves as identifier of the clients
    static ClientPtr CreateClient(int fd, const std::string &ipstr, uint16_t port);

    static void  DestroyClient(int fd);

    static ClientPtr GetClient(int fd);

    static int NumberOfClients();

    static std::map<int, ClientPtr>  clients;

    std::string getRemoteIPString() const { return ipstr_; }
    uint16_t getRemotePort() const { return port_; }

    Status processQuery();

    Status executeCommand(CatchDBPtr db);

    Status writeResult();

    // non-copyable
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

private:
    int read();

    enum class ResponseStatus { OK = 0, NotFound = 1, Error = 2, Fail = 3, ClientError = 4 };
    static std::array<const char*, 5> statusDesc;

    void addResponse(ResponseStatus status, const std::string &resp);

    void addResponse(ResponseStatus status, const Response &resp);

    int fd_;

    // peer end addr and port 
    std::string ipstr_;
    uint16_t port_;

    Buffer queryBuf_;
    RequestPtr req_;
    std::string replyBuf_;
    int writePos_;

// add some expire or lru mechanism
    std::map<std::string, QueuePtr> queues_;
    std::map<std::string, HashMapPtr> hashMaps_;
    std::map<std::string, ZSetPtr> zsets_;
};


} // namespace catchdb

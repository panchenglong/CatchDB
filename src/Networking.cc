#include "Logger.h"
#include "Util.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h> 
#include <cstring>

namespace catchdb
{

int SetTcpNoDelay(int fd)
{
    int val = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val)) == -1)
    {
        LogError("setsockopt TCP_NODELAY: %s", ErrorDescription(errno));
        return -1;
    }
    return 0;
}

int SetTcpKeepAlive(int fd)
{
    int yes = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes)) == -1) {
        LogError("setsockopt SO_KEEPALIVE: %s", ErrorDescription(errno));
        return -1;
    }
    return 0;
}

int SetNonBlocking(int fd)
{
    int oldOption = fcntl(fd, F_GETFL);
    if (oldOption == -1)
        return -1;
    int newOption = oldOption | O_NONBLOCK;
    if (fcntl(fd, F_SETFL, newOption) == -1)
        return -1;
    return 0;
}

int Listen(const std::string &ip, const std::string &port, int backlog)
{
    const char *ipstr = nullptr;
    if (!ip.empty())
        ipstr = ip.c_str();

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof (hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int status;
    if ((status = getaddrinfo(ipstr, port.c_str(), &hints, &res)) != 0) {
        LogError("getaddrinfo: %s", gai_strerror(status));
        return -1;
    }

    int sockfd;
    if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
        LogError("socket: %s", ErrorDescription(errno));
        return -1;
    }
    
    int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        LogError("setsockopt: %s", ErrorDescription(errno));
        close(sockfd);
        return -1;
    }
    
    if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        LogError("bind: %s", ErrorDescription(errno));
        close(sockfd);
        return -1;
    }

    if (listen(sockfd, backlog) == -1) {
        LogError("listen: %s", ErrorDescription(errno));
        close(sockfd);
        return -1;
    }

    SetNonBlocking(sockfd);

    freeaddrinfo(res);

    return sockfd;
}


} // namespace catchdb

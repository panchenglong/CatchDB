#pragma once

namespace catchdb
{

int SetTcpNoDelay(int fd);

int SetTcpKeepAlive(int fd);

int SetNonBlocking(int fd);

int Listen(const std::string &ip, const std::string &port, int backlog);

} // namespace catchdb

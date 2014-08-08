#include "unistd.h"
#include "Client.h"
#include "Networking.h"
#include "Util.h"
#include "KV.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <cassert>
#include <cstdio>

namespace catchdb
{

std::map<int, ClientPtr>  Client::clients;

ClientPtr Client::CreateClient(int fd, const std::string &ipstr, uint16_t port)
{
    if (clients.find(fd) != clients.end()) {
        return clients[fd];
    }

    clients[fd] = ClientPtr(new Client(fd, ipstr, port));

    return clients[fd];
}

void Client:: DestroyClient(int fd)
{
    clients.erase(fd);
}

ClientPtr Client::GetClient(int fd)
{
    return clients[fd];
}

int Client::NumberOfClients()
{
    return clients.size();
}

Status Client::processQuery()
{
    int len = read();
    if (len == -2) {
        return Status::Progress;
    } else if (len == 0) {
        return Status::Close;
    } else if (len == -1) {
        return Status::Error;
    }

    int size = queryBuf_.size();
    req_ = Request::ParseRequest(queryBuf_.data(), &size, req_);
    queryBuf_.decr(size);

    switch (req_->state) {
        case Request::State::Complete: {
            queryBuf_.reset();
            return Status::OK;
        }
        case Request::State::Partial: {
            return Status::Progress;
        }
        case Request::State::Error:
            return Status::Error;
    }
}

Status Client::executeCommand(const CatchDBPtr db)
{
    // guarantee that executeCommand is called after processQuery
    assert(req_ != nullptr);
    assert(req_->state != Request::State::Error);

    if (req_->state == Request::State::Partial)
        return Status::Progress; // harmless

    std::string cmd = req_->blocks[0];
    if (cmdMap.find(cmd) == cmdMap.end()) {
        addResponse(ResponseStatus::ClientError, "Unknown command");
        req_ = nullptr;
        return Status::OK;
    } 

    if (!BeginWith(req_->blocks[0], "multi_") &&
        req_->blocks.size() != cmdMap[cmd].numReqBlks) {
        addResponse(ResponseStatus::ClientError, "Wrong number of arguments");
        req_ = nullptr;
        return Status::OK;
    }

    Response resp;
    Status s;
    switch (cmdMap[cmd].category) {
        case Category::KV: {
            s = KV::process(db, req_, &resp);
            break;
        }
        case Category::HashMap: {
            auto name = req_->blocks[1];
            if (hashMaps_.find(name) == hashMaps_.end()) {
                hashMaps_[name] =  HashMapPtr(new HashMap(db, name));
            }
            s = hashMaps_[name]->process(req_, &resp);
            break;
        }
        case Category::Queue: {
            auto key = req_->blocks[1];
            if (queues_.find(key) == queues_.end()) {
                queues_[key] =  QueuePtr(new Queue(db, key));
            }
            s = queues_[key]->process(req_, &resp);
            break;
        }
        case Category::ZSet: {
            auto name = req_->blocks[1];
            if (zsets_.find(name) == zsets_.end()) {
                zsets_[name] =  ZSetPtr(new ZSet(db, name));
            }
            s = zsets_[name]->process(req_, &resp);
            break;

        }
    }
    ResponseStatus rs;
    switch (s) {
        case Status::Error:
            rs = ResponseStatus::Error;
            break;
        case Status::OK:
            rs = ResponseStatus::OK;
            break;
        case Status::NotFound:
            resp.clear();
            rs = ResponseStatus::NotFound;
            break;
        case Status::Empty:
            rs = ResponseStatus::NotFound;
            break;

        case Status::InvalidParameter:
            rs = ResponseStatus::ClientError;
            break;
        case Status::OutOfRange:
            resp.push_back("Out of range");
            rs = ResponseStatus::ClientError;
            break;
        case Status::NotImplemented:
            resp.clear();
            resp.push_back("Command not implemented yet");
            // fall through
        default:
            rs = ResponseStatus::Error;
            break;
    }
    addResponse(rs, resp);

    req_ = nullptr;
    return Status::OK;
}

Status Client::writeResult()
{
    int toSend = replyBuf_.size() - writePos_;
    int sent = ::send(fd_, replyBuf_.data() + writePos_, toSend, 0);
    if (sent < 0) {
        return Status::Error;
    } else if (sent < toSend) {
        return Status::Progress;
    } else {
        replyBuf_.clear();
        return Status::OK;
    }
}

/***************** private ***********************/

int Client::read()
{
    int ret = -2; // progress

    while (!queryBuf_.full()) {
        int len = ::recv(fd_, queryBuf_.tail(), queryBuf_.avail(), 0);
        if (len == -1) {
            if (errno == EINTR) {
                continue;
            } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            } else {
                return -1;
            }
        } else if (len == 0) {
            return 0;
        } else {
            ret += len;
            queryBuf_.incr(len);
        }
    }
    return ret;
}

std::array<const char*, 5> Client::statusDesc = {
    "2\nok\n", "9\nnot_found\n", "5\nerror\n",
    "4\nfail\n", "12\nclient_error\n" };

void Client::addResponse(ResponseStatus status, const std::string &resp)
{
    replyBuf_.append(statusDesc[static_cast<size_t>(status)]);
    if (!resp.empty()) {
        replyBuf_.append(std::to_string(resp.size()));
        replyBuf_.append(1, '\n');
        replyBuf_.append(resp);
        replyBuf_.append(1, '\n');
    }
    replyBuf_.append(1, '\n');
}

void Client::addResponse(ResponseStatus status, const Response &resp)
{
    replyBuf_.append(statusDesc[static_cast<size_t>(status)]);
    if (!resp.empty()) {
        for (auto &s : resp) {
            replyBuf_.append(std::to_string(s.size()));
            replyBuf_.append(1, '\n');
            replyBuf_.append(s);
            replyBuf_.append(1, '\n');
        }
    }
    replyBuf_.append(1, '\n');
}


Client::Client(int fd, const std::string &ipstr, uint16_t port)
    : fd_(fd), ipstr_(ipstr), port_(port), 
      queryBuf_(QUERY_BUF_SIZE),
      req_(nullptr), 
      writePos_(0)
{}


Client::~Client()
{
    if (fd_) {
        close(fd_);
    }
}


} // namespace catchdb

/*
 * Currently catchdb adopts SSDB Protocol
 * See http://ssdb.io/docs/protocol.html
 */

#pragma once

#include <vector>
#include <string>
#include <memory>
#include <map>

namespace catchdb
{
struct Request;
typedef std::shared_ptr<Request> RequestPtr;

struct Request
{
    enum class State { Error, Partial, Complete };

    State state;

    std::vector<std::string> blocks;

    // Parse received @data into blocks. If @req with partial state is supplied, then
    // append blocks to req and return req. Otherwise a new Request is allocated.
    // pass @length to be the size of data available, after parsing, it is the length
    // of the bytes that has been parsed.
    static RequestPtr ParseRequest(char *data, int *length, RequestPtr req);

};

enum class Category { KV, Queue, HashMap, ZSet };
enum class Property { Read, Write };
struct CmdInfo {
    Category category;
    int numReqBlks;
    Property property;
};
extern std::map<std::string, CmdInfo> cmdMap;

typedef std::vector<std::string> Response;
typedef Response *ResponsePtr;

} // namespace catchdb

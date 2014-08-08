#include "Protocol.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

namespace catchdb
{

/******************** Request **********************/

RequestPtr Request::ParseRequest(char *data, int *length, RequestPtr req)
{
    for (int i = 0; i < *length; ++i) {
        putchar(*(data + i));
    }
    putchar('\n');
    RequestPtr r;

    if (req != nullptr) {
        if (req->state != State::Partial)
            return req;
        r = req;
    } else {
        r = RequestPtr(new Request);
    }

    int len = *length;
    char *ptr = data;
    while (len > 0) {
        char *d = (char *)memchr(ptr, '\n', len);
        if (d == nullptr) {
            break;
        }
        ++d;

        int num = d - ptr;
        if(num == 1 || (num == 2 && ptr[0] == '\r')){
            // Packet received.
            len -= num;
            *length -= len;
            r->state = State::Complete;
            return r;
        }

        if (!isdigit(ptr[0]))
            goto error;

        // Size received
        int size = (int)strtol(ptr, nullptr, 10);

        if (size < 0)
            goto error;

        len -= num + size;
        ptr += num + size;
        if(len >= 1 && ptr[0] == '\n'){
            len -= 1;
            ptr += 1;
        }else if(len >= 2 && ptr[0] == '\r' && ptr[1] == '\n'){
            len -= 2;
            ptr += 2;
        }else{
            len += num + size;
            *length -= len;
            r->state = State::Partial;
            return r;
        }
        // Data received
        r->blocks.push_back(std::string(d, size));
    }

  error:
    // bad format
    r->state = State::Error;
    return r;
}

/*************** Command ********************/
std::map<std::string, CmdInfo> cmdMap = {
    // cmd name, { category, number of blocks }
    { "get", { Category::KV, 2, Property::Read } },
    { "getall", { Category::KV, 1, Property::Read } },
    { "set", { Category::KV, 3, Property::Write } },
    { "setx", { Category::KV, 4, Property::Write } },
    { "setnx", { Category::KV, 3, Property::Write } },
    { "getset", { Category::KV, 3, Property::Write } },
    { "del", { Category::KV, 2, Property::Write } },
    { "incr", { Category::KV, 3, Property::Write } },
    { "decr", { Category::KV, 3, Property::Write} },
    { "scan", { Category::KV, 4, Property::Read } },
    { "rscan", { Category::KV, 4, Property::Read } },
    { "keys", { Category::KV, 1, Property::Read } },
    { "exists", { Category::KV, 2, Property::Read } },
    // { "multi_exists", { Category::KV, 3, Property::Read } },
    // { "multi_get", { Category::KV, 3, Property::Read } },
    { "multi_set", { Category::KV, 3, Property::Write } },
    // { "multi_del", { Category::KV, 3, Property::Read } },

    { "hsize", { Category::HashMap, 2, Property::Read } },
    { "hget", { Category::HashMap, 3, Property::Read } },
    { "hset", { Category::HashMap, 4, Property::Write } },
    { "hdel", { Category::HashMap, 3, Property::Write } },
    { "hincr", { Category::HashMap, 4, Property::Write } },
    { "hdecr", { Category::HashMap, 3, Property::Write } },
    { "hclear", { Category::HashMap, 2, Property::Write } },
    { "hgetall", { Category::HashMap, 2, Property::Read } },
    { "hscan", { Category::HashMap, 5, Property::Read } },
    { "hrscan", { Category::HashMap, 5, Property::Read } },
    { "hkeys", { Category::HashMap, 2, Property::Read } },
    { "hvals", { Category::HashMap, 3, Property::Read } },
    { "hlist", { Category::HashMap, 4, Property::Read } },
    { "hexists", { Category::HashMap, 3, Property::Read } },
    { "multi_hset", { Category::HashMap, 3, Property::Write } },
    // { "multi_hsize", { Category::HashMap, 3, Property::Read } },

    { "zget", { Category::ZSet, 3, Property::Read } },
    { "zset", { Category::ZSet, 4, Property::Write } },
    { "zdel", { Category::ZSet, 3, Property::Write } },
    { "zincr", { Category::ZSet, 4, Property::Write } },
    { "zdecr", { Category::ZSet, 4, Property::Write } },
    { "zclear", { Category::ZSet, 2, Property::Write } },
    { "zscan", { Category::ZSet, 6, Property::Read } },
    { "zrscan", { Category::ZSet, 6, Property::Read } },
    { "zkeys", { Category::ZSet, 6, Property::Read } },
    { "zlist", { Category::ZSet, 4, Property::Read } },
    { "zcount", { Category::ZSet, 4, Property::Read } },
    { "zsum", { Category::ZSet, 4, Property::Read } },
    { "zavg", { Category::ZSet, 4, Property::Read } },
    { "zremrangebyrank", { Category::ZSet, 4, Property::Read } },
    { "zremrangebyscore", { Category::ZSet, 4, Property::Read } },
    { "zexists", { Category::ZSet, 3, Property::Read } },
    { "zsize", { Category::ZSet, 2, Property::Read } },
    { "zmod", { Category::ZSet, 4, Property::Write } },
    { "ztopn", { Category::ZSet, 3, Property::Read } },
    { "zgetall", { Category::ZSet, 2, Property::Read } },
    { "multi_zexists", { Category::ZSet, 3, Property::Read } },
    { "multi_zsize", { Category::ZSet, 3, Property::Read } },
    { "multi_zget", { Category::ZSet, 3, Property::Read } },
    { "multi_zset", { Category::ZSet, 3, Property::Read } },
    { "multi_zdel", { Category::ZSet, 3, Property::Read } },

    { "qsize", { Category::Queue, 2, Property::Read } },
    { "qfront", { Category::Queue, 2, Property::Read } },
    { "qback", { Category::Queue, 2, Property::Read } },
    { "qpush", { Category::Queue, 3, Property::Write } },
    { "qpush_front", { Category::Queue, 3, Property::Write } },
    { "qpush_back", { Category::Queue, 3, Property::Write } },
    { "qpop", { Category::Queue, 2, Property::Write } },
    { "qpop_front", { Category::Queue, 2, Property::Write } },
    { "qpop_back", { Category::Queue, 2, Property::Write } },
    { "qclear", { Category::Queue, 2, Property::Write } },
    { "qlist", { Category::Queue, 2, Property::Read } },
    { "qslice", { Category::Queue, 4, Property::Read } },
    { "qrange", { Category::Queue, 3, Property::Read } },
    { "qget", { Category::Queue, 3, Property::Read } } 
};


} // namespace catchdb

#pragma once

#include <string>
#include <vector>
#include <utility>
#include <tuple>
#include <memory>
#include <cstdint>
#include "CatchDB.h"
#include "Status.h"
#include "Protocol.h"

namespace catchdb
{

class HashMap
{
public:
    HashMap(const CatchDBPtr db, const std::string &hashMapName);
    ~HashMap() {}

    Status process(const RequestPtr req, ResponsePtr resp);

    Status size(const RequestPtr req, ResponsePtr resp);
    bool empty();

    Status set(const RequestPtr req, ResponsePtr resp);
    Status setM(const RequestPtr req, ResponsePtr resp);
    Status mod(const RequestPtr req, ResponsePtr resp);

    Status get(const RequestPtr req, ResponsePtr resp);

    Status del(const RequestPtr req, ResponsePtr resp);

    Status exists(const RequestPtr req, ResponsePtr resp);

    Status keys(const RequestPtr req, ResponsePtr resp);
    Status getall(const RequestPtr req, ResponsePtr resp);
    Status clear(const RequestPtr req, ResponsePtr resp);

    // non-copyable
    HashMap(const HashMap&) = delete;
    HashMap& operator=(const HashMap&) = delete;

private:
    typedef Status (HashMap::*proc_t) (const RequestPtr, ResponsePtr);
    std::map<std::string, proc_t> procMap;

    std::string encodeKey(const std::string &key);
    std::string decodeKey(const std::string &codedKey);

    CatchDBPtr db_;
    std::string name_;
    std::string keyTemplate_;
    uint64_t size_;
};

typedef std::unique_ptr<HashMap> HashMapPtr;

} // namespace catchdb

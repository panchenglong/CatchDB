#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "leveldb/db.h"
#include "leveldb/iterator.h"
#include "Util.h"
#include "Status.h"

namespace catchdb
{

struct KVPair {
    std::string key;
    std::string value;

    KVPair(const std::string &k, const std::string &v)
        : key(k), value(v) {}
};

class Iterator
{
public:
    Iterator(leveldb::DB *db, 
             const std::string &prefix, 
             const std::string &exclude);

    ~Iterator();

    Status status();

    // Get k-v pairs from key range [start, end], limited
    // to @limit pairs
    std::vector<KVPair> range(const std::string &start,
                              const std::string &end,
                              uint64_t limit);
    // Get keys from key range [start, end], limited
    // to @limit pairs
    std::vector<std::string> keys(const std::string &satrt,
                                  const std::string &end,
                                  uint64_t limit);
    // Get vaules conrresponds to  key range [start, end], 
    // limited to @limit pairs
    std::vector<std::string> values(const std::string &start,
                                    const std::string &end,
                                    uint64_t limit);

private:
    leveldb::DB *db_;
    std::string prefix_;
    std::string exclude_;
    leveldb::Iterator *it_;
};

} // namespace catchdb

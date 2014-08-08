/*
 * Record := SizeRecord | KeyScoreRecord | ScoreKeyRecord
 * SizeRecord := ['ZN' + sizeof(Name) + Name][size of ZSet]
 * KeyScoreRecord := ['ZK' + sizeof(Name) + Name + Key][Score]
 * ScoreKeyRecord := ['ZS' + sizeof(Name) + Name + Score + Key][]
 * 
 * size of sizeof(Name): 2 bytes
 * size of Score: sizeof(int64_t) = 8 bytes
 */


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

class ZSet
{
public:
    ZSet(const CatchDBPtr db, const std::string &zsetName);
    ~ZSet() {}

    Status process(const RequestPtr req, ResponsePtr resp);

    Status size(const RequestPtr req, ResponsePtr resp);

    bool empty();

    Status set(const RequestPtr req, ResponsePtr resp);
    Status setM(const RequestPtr req, ResponsePtr resp);
    Status mod(const RequestPtr req, ResponsePtr resp);
    Status get(const RequestPtr req, ResponsePtr resp);
    Status del(const RequestPtr req, ResponsePtr resp);
    Status exists(const RequestPtr req, ResponsePtr resp);
    Status topn(const RequestPtr req, ResponsePtr resp);
    Status getall(const RequestPtr req, ResponsePtr resp);

    // non-copyable
    ZSet(const ZSet&) = delete;
    ZSet& operator=(const ZSet&) = delete;

private:
    std::string encodeKey(const std::string &key);
    std::string encodeScore(int64_t score, const std::string &key);
    std::pair<std::string, int64_t> decodeScoreKey(const std::string &scoreKey);
    std::string decodeKey(const std::string &codedKey);

    typedef Status (ZSet::*proc_t) (const RequestPtr, ResponsePtr);
    std::map<std::string, proc_t> procMap;

    CatchDBPtr db_;
    std::string name_;
    std::string sizeTemplate_;
    std::string keyTemplate_;
    std::string scoreTemplate_;
    uint64_t size_;
};

typedef std::unique_ptr<ZSet> ZSetPtr;

} // namespace catchdb

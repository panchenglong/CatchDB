#include "KV.h"
#include "Iterator.h"
#include "leveldb/write_batch.h"
#include <string>
#include <map>

namespace catchdb
{

namespace KV
{

namespace 
{
const char KV_TYPE_INDENTIFIRE = 'K';
typedef Status (*proc_t) (const CatchDBPtr, const RequestPtr, ResponsePtr);
std::map<std::string, proc_t> procMap = {
    { "get", &get },
    { "getall", &getall },
    { "set", &set },
    { "del", &del },
    { "exists", &exists },
    { "multi_set", &setM },
    { "keys", &keys },
};

std::string encodeKey(const std::string &key)
{
    return 'K' + key;
}

std::string decodeKey(const std::string &codedKey)
{
    return codedKey.substr(1, std::string::npos);
}

} // namespace

Status process(const CatchDBPtr db, const RequestPtr req, ResponsePtr resp)
{
    std::string &cmd = req->blocks[0];
    if (procMap.find(cmd) == procMap.end())
        return Status::NotImplemented;
    auto func = procMap[cmd];
    return (*func)(db, req, resp);
}

Status get(const CatchDBPtr db, const RequestPtr req, ResponsePtr resp)
{
    auto key = encodeKey(req->blocks[1]);
    std::string val;
    auto s =  db->get(key, &val);
    resp->push_back(val);
    return s;
}

Status set(const CatchDBPtr db, const RequestPtr req, ResponsePtr resp)
{
    (void) resp;
    auto key = encodeKey(req->blocks[1]);
    return db->put(key, req->blocks[2]);
}

Status setM(const CatchDBPtr db, const RequestPtr req, ResponsePtr resp)
{
    (void) resp;
    int size = req->blocks.size();
    if ((size - 1) % 2) {
        resp->push_back("arguments insufficient to make up k-v pairs");
        return Status::InvalidParameter;
    }

    std::map<std::string, std::string> kvs; // to remove duplicate in a multi_set
    for (int i = 1; i < size; i += 2) {
        kvs[req->blocks[i]] = req->blocks[i + 1];
    }

    leveldb::WriteBatch batch;
    for (auto &kv : kvs) {
        auto key = encodeKey(kv.first);
        batch.Put(key, kv.second);
    }

    return db->putM(&batch);
}

Status keys(const CatchDBPtr db, const RequestPtr req, ResponsePtr resp)
{
    std::unique_ptr<Iterator> it(db->newIterator("K", "K"));
    auto keys = it->keys("K", "", 0);
    if (it->status() != Status::OK)
        return Status::Error;

    for (auto &key : keys) {
        resp->push_back(decodeKey(key));
    }
    return Status::OK;
}

Status getall(const CatchDBPtr db, const RequestPtr req, ResponsePtr resp)
{
    std::unique_ptr<Iterator> it(db->newIterator("K", "K"));
    auto kvs = it->range("K", "", 0);
    if (it->status() != Status::OK)
        return Status::Error;

    for (auto &kv : kvs) {
        resp->push_back(decodeKey(kv.key));
        resp->push_back(kv.value);
    }
    return Status::OK;
}


Status del(const CatchDBPtr db, const RequestPtr req, ResponsePtr resp)
{
    (void) resp;
    auto key = encodeKey(req->blocks[1]);
    return db->del(key);
}

Status exists(const CatchDBPtr db, const RequestPtr req, ResponsePtr resp)
{
    auto s = get(db, req, resp);
    if (s == Status::NotFound) {
        resp->push_back("no");
    } else {
        resp->push_back("yes");
    }

    return Status::OK;
}


} // namespace KV

} // namespace catchdb

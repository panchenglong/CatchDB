#include "HashMap.h"
#include "Logger.h"
#include "Util.h"
#include "Iterator.h"
#include "leveldb/db.h"
#include "leveldb/options.h"
#include "leveldb/write_batch.h"
#include <cstring>
#include <limits>
#include <stdexcept>
#include <map>
#include <memory>
#include <cstdio>

namespace catchdb
{

namespace
{
const char HASHMAP_TYPE_INDENTIFIRE = 'H';
} // namespace

HashMap::HashMap(const CatchDBPtr db, const std::string &hashMapName)
    : db_(db), name_(hashMapName), size_(0)
{
    procMap = { { "hsize", &HashMap::size },
                { "hset", &HashMap::set },
                { "hmod", &HashMap::mod },
                { "multi_hset", &HashMap::setM },
                { "hget", &HashMap::get },
                { "hdel", &HashMap::del },
                { "hexists", &HashMap::exists },
                { "hkeys", &HashMap::keys },
                { "hgetall", &HashMap::getall },
                { "hclear", &HashMap::clear } };


    keyTemplate_.append(1, HASHMAP_TYPE_INDENTIFIRE);
    uint16_t keySize = static_cast<uint16_t>(hashMapName.size());
    keyTemplate_.append((char *)&keySize, sizeof (uint16_t));
    keyTemplate_.append(hashMapName.data(), keySize);

    Response resp;
    (void) size(nullptr, &resp); // do not care whether succeed
}

Status HashMap::process(const RequestPtr req, ResponsePtr resp)
{
    std::string &cmd = req->blocks[0];
    if (procMap.find(cmd) == procMap.end())
        return Status::NotImplemented;
    auto func = procMap[cmd];
    return (this->*func)(req, resp);
}


Status HashMap::size(const RequestPtr req, ResponsePtr resp)
{
    if (size_ == 0) {
        std::string val;
        auto s = db_->get(keyTemplate_, &val);
        if (s == Status::OK) {
            size_ = *((uint64_t*) val.data());
        } else if (s == Status::NotFound){
            size_ = 0;
        } else {
            return Status::Error;
        }
    }

    resp->push_back(std::to_string(size_));
    return Status::OK;
}

bool HashMap::empty()
{
    return size_ == 0;
}

// modify existing value, suggested using exists before add
// or mod
Status HashMap::mod(const RequestPtr req, ResponsePtr resp)
{
    auto key = encodeKey(req->blocks[2]);
    return db_->put(key, req->blocks[3]);
}

Status HashMap::set(const RequestPtr req, ResponsePtr resp)
{
    auto key = encodeKey(req->blocks[2]);
    std::string val;
    auto s = db_->get(key, &val);
    uint64_t size;
    if (s == Status::NotFound) {
        size = size_ + 1;
    } else if (s == Status::OK) {
        size = size_;
    } else {
        return s;
    }

    leveldb::WriteBatch batch;
    batch.Put(key, req->blocks[3]);
    batch.Put(keyTemplate_, NumberToString(size));
    s = db_->putM(&batch);
    if (s == Status::OK) {
        size_ = size;
        return Status::OK;
    }
    return Status::Error;
}

Status HashMap::setM(const RequestPtr req, ResponsePtr resp)
{
    int size = req->blocks.size();
    if ((size - 2) % 2) {
        resp->push_back("arguments insufficient to make up k-v pairs");
        return Status::InvalidParameter;
    }

    std::map<std::string, std::string> kvs; // to remove duplicate in a multi_set
    for (int i = 2; i < size; i += 2) {
        kvs[req->blocks[i]] = req->blocks[i + 1];
    }

    int kvSize = 0;
    leveldb::WriteBatch batch;
    for (auto &kv : kvs) {
        auto key = encodeKey(kv.first);
        std::string val;
        auto s = db_->get(key, &val);
        if (s == Status::NotFound) {
            ++kvSize;
        } else if (s != Status::OK) {
            return s;
        }
        batch.Put(key, kv.second);
    }

    batch.Put(keyTemplate_, NumberToString(size_ + kvSize));
    Status s = db_->putM(&batch);
    if (s == Status::OK) {
        size_ += kvSize;
        return Status::OK;
    }
    return Status::Error;

}

Status HashMap::get(const RequestPtr req, ResponsePtr resp)
{
    auto key = encodeKey(req->blocks[2]);
    std::string val;
    auto s = db_->get(key, &val);
    resp->push_back(val);
    return s;
}

Status HashMap::getall(const RequestPtr req, ResponsePtr resp)
{
    std::unique_ptr<Iterator> it(db_->newIterator(keyTemplate_, keyTemplate_));
    auto kvs = it->range(keyTemplate_, "", 0);
    if (it->status() != Status::OK)
        return Status::Error;

    for (auto &kv : kvs) {
        resp->push_back(decodeKey(kv.key));
        resp->push_back(kv.value);
    }
    return Status::OK;
}

Status HashMap::keys(const RequestPtr req, ResponsePtr resp)
{
    std::unique_ptr<Iterator> it(db_->newIterator(keyTemplate_, keyTemplate_));
    auto keys = it->keys(keyTemplate_, "", 0);
    if (it->status() != Status::OK)
        return Status::Error;

    for (auto &key : keys) {
        resp->push_back(decodeKey(key));
    }
    return Status::OK;
}

Status HashMap::exists(const RequestPtr req, ResponsePtr resp)
{
    auto s = get(req, resp);
    resp->clear();
    if (s == Status::NotFound) {
        resp->push_back("no");
    } else {
        resp->push_back("yes");
    }

    return Status::OK;
}

Status HashMap::del(const RequestPtr req, ResponsePtr resp)
{
    (void) resp;
    auto key = encodeKey(req->blocks[2]);
    std::string val;
    auto s = db_->get(key, &val);
    if (s == Status::NotFound || s != Status::OK)
        return s;

    leveldb::WriteBatch batch;
    batch.Delete(key);
    if (size_ == 1) {
        batch.Delete(keyTemplate_);
    } else {
        batch.Put(keyTemplate_, NumberToString(size_ - 1));
    }

    s = db_->putM(&batch);
    if (s == Status::OK) {
        --size_;
        return Status::OK;
    }
    return Status::Error;
}

Status HashMap::clear(const RequestPtr req, ResponsePtr resp)
{
    (void) resp;

    std::unique_ptr<Iterator> it(db_->newIterator(keyTemplate_, keyTemplate_));
    auto keys = it->keys(keyTemplate_, "", 0);
    if (it->status() != Status::OK)
        return Status::Error;

    leveldb::WriteBatch batch;
    for (auto &key : keys) {
        batch.Delete(key);
    }
    batch.Delete(keyTemplate_);

    auto s = db_->putM(&batch);
    if (s == Status::OK) {
        size_ = 0;
    }
    return s;
}


/*************** private member functions *******************/

std::string HashMap::encodeKey(const std::string &key)
{
    std::string newKey(keyTemplate_);
    newKey.append(key);
    return newKey;
}

std::string HashMap::decodeKey(const std::string &codedKey)
{
    return codedKey.substr(keyTemplate_.size(), std::string::npos);
}

} // namespace catchdb

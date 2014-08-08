#include "ZSet.h"
#include "Logger.h"
#include "Util.h"
#include "Iterator.h"
#include "leveldb/db.h"
#include "leveldb/options.h"
#include "leveldb/write_batch.h"
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>
#include <cstdio>

namespace catchdb
{

ZSet::ZSet(const CatchDBPtr db, const std::string &zsetName)
    : db_(db), name_(zsetName), size_(0)
{
    procMap = { { "zsize", &ZSet::size },
                { "zset", &ZSet::set },
                { "zmod", &ZSet::mod },
                { "multi_zset", &ZSet::setM },
                { "zget", &ZSet::get },
                { "zdel", &ZSet::del },
                { "ztopn", &ZSet::topn },
                { "zgetall", &ZSet::getall },
                { "zexists", &ZSet::exists } };

    sizeTemplate_.append("ZN");
    uint16_t nameSize = static_cast<uint16_t>(zsetName.size());
    sizeTemplate_.append((char *)&nameSize, sizeof (uint16_t));
    sizeTemplate_.append(zsetName.data(), nameSize);
    keyTemplate_.append("ZK");
    keyTemplate_.append((char *)&nameSize, sizeof (uint16_t));
    keyTemplate_.append(zsetName.data(), nameSize);
    scoreTemplate_.append("ZS");
    scoreTemplate_.append((char *)&nameSize, sizeof (uint16_t));
    scoreTemplate_.append(zsetName.data(), nameSize);

    Response resp;
    (void) size(nullptr, &resp); // do not care whether succeed
}

Status ZSet::process(const RequestPtr req, ResponsePtr resp)
{
    std::string &cmd = req->blocks[0];
    if (procMap.find(cmd) == procMap.end())
        return Status::NotImplemented;
    auto func = procMap[cmd];
    return (this->*func)(req, resp);
}

Status ZSet::size(const RequestPtr req, ResponsePtr resp)
{
    (void) req;
    if (size_ == 0) {
        std::string val;
        auto s = db_->get(sizeTemplate_, &val);
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

bool ZSet::empty()
{
    return size_ == 0;
}

Status ZSet::set(const RequestPtr req, ResponsePtr resp)
{
    auto key = encodeKey(req->blocks[2]);
    int size;
    std::string val;
    auto s = db_->get(key, &val);
    if (s == Status::NotFound) {
        size = size_ + 1;
    } else if (s == Status::OK) {
        size = size_;
    } else {
        return s;
    }

    int64_t score;
    try {
        score = std::stoll(req->blocks[3]);
    } catch(...) {
        resp->push_back("score should be an integer");
        return Status::InvalidParameter;
    }
    auto scoreKey = encodeScore(score, req->blocks[2]);

    leveldb::WriteBatch batch;
    batch.Put(key, NumberToString(score));
    batch.Put(scoreKey, "");
    batch.Put(sizeTemplate_, NumberToString(size));
    s = db_->putM(&batch);
    if (s == Status::OK) {
        size_ = size;
        return Status::OK;
    }
    return Status::Error;
}

Status ZSet::mod(const RequestPtr req, ResponsePtr resp)
{
    auto key = encodeKey(req->blocks[2]);

    int64_t score;
    try {
        score = std::stoll(req->blocks[3]);
    } catch(...) {
        resp->push_back("score should be an integer");
        return Status::InvalidParameter;
    }
    auto scoreKey = encodeScore(score, req->blocks[2]);

    leveldb::WriteBatch batch;
    batch.Put(key, NumberToString(score));
    batch.Put(scoreKey, "");
    Status s = db_->putM(&batch);

    return s;
}


Status ZSet::setM(const RequestPtr req, ResponsePtr resp)
{
    int size = req->blocks.size();
    if ((size - 2) % 2) {
        resp->push_back("arguments insufficient to make up key-score pairs");
        return Status::InvalidParameter;
    }

    std::map<std::string, std::string> kvs; // to remove duplicate in a multi_set
    for (int i = 2; i < size; i += 2) {
        kvs[req->blocks[i]] = req->blocks[i + 1];
    }

    std::map<std::string, int64_t> kss; // key-score pairs
    for (auto &kv : kvs) {
        int64_t score;
        try {
            score = std::stoll(kv.second);
        } catch(...) {
            resp->push_back("score should be an integer");
            return Status::InvalidParameter;
        }
        kss[kv.first] = score;
    }

    int newSize = 0;
    leveldb::WriteBatch batch;
    for (auto &ks : kss) {
        auto key = encodeKey(ks.first);
        std::string val;
        auto s = db_->get(key, &val);
        if (s == Status::NotFound) {
            ++newSize;
        } else if (s != Status::OK) {
            return s;
        }

        batch.Put(key, NumberToString(ks.second));
    }
    for (auto &ks : kss) {
        auto scoreKey = encodeScore(ks.second, ks.first);
        batch.Put(scoreKey, "");
    }

    batch.Put(sizeTemplate_, NumberToString(size_ + newSize));
    Status s = db_->putM(&batch);
    if (s == Status::OK) {
        size_ += newSize;
        return Status::OK;
    }
    return Status::Error;

}

Status ZSet::get(const RequestPtr req, ResponsePtr resp)
{
    auto key = encodeKey(req->blocks[2]);
    std::string val;
    auto s = db_->get(key, &val);
    int64_t score = *((int64_t*) val.data());
    resp->push_back(std::to_string(score));
    return s;
}

Status ZSet::exists(const RequestPtr req, ResponsePtr resp)
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

Status ZSet::del(const RequestPtr req, ResponsePtr resp)
{
    (void) resp;
    auto key = encodeKey(req->blocks[2]);
    std::string val;
    auto s = db_->get(key, &val);
    if (s == Status::NotFound || s != Status::OK)
        return s;

    leveldb::WriteBatch batch;
    batch.Delete(key);
    std::string scoreKey(scoreTemplate_);
    scoreKey.append(val);
    scoreKey.append(req->blocks[2]);
    batch.Delete(scoreKey);
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

Status ZSet::topn(const RequestPtr req, ResponsePtr resp)
{
    int n;
    try {
        n = std::stoi(req->blocks[2]);
    } catch(...) {
        resp->push_back("number should be an integer");
        return Status::InvalidParameter;
    }

    std::unique_ptr<Iterator> it(db_->newIterator(scoreTemplate_));
    auto keys = it->keys(scoreTemplate_, "", n);
    if (it->status() == Status::Error)
        return Status::Error;

    for (auto &key : keys) {
        auto ks = decodeScoreKey(key);
        resp->push_back(ks.first);
        resp->push_back(std::to_string(ks.second));
    }

    return Status::OK;
}

Status ZSet::getall(const RequestPtr req, ResponsePtr resp)
{
    std::unique_ptr<Iterator> it(db_->newIterator(keyTemplate_));
    auto kvs = it->range(keyTemplate_, "", 0);
    if (it->status() == Status::Error)
        return Status::Error;

    for (auto &kv : kvs) {
        auto key = decodeKey(kv.key);
        int64_t score = *((int64_t*) kv.value.data());
        resp->push_back(key);
        resp->push_back(std::to_string(score));
    }

    return Status::OK;
}


/************ private *********************/
std::string ZSet::encodeKey(const std::string &key)
{
    std::string newKey(keyTemplate_);
    newKey.append(key);
    return newKey;
}

std::string ZSet::encodeScore(int64_t score, const std::string &key)
{
    std::string newKey(scoreTemplate_);
    newKey.append(NumberToString(score));
    newKey.append(key);
    return newKey;
}

std::pair<std::string, int64_t> ZSet::decodeScoreKey(const std::string &scoreKey)
{
    int len = scoreTemplate_.size();
    int64_t score = *((int64_t*)(scoreKey.data() + len));
    std::string key = scoreKey.substr(len + sizeof (int64_t), std::string::npos);
    return std::make_pair(key, score);
}

std::string ZSet::decodeKey(const std::string &codedKey)
{
    return codedKey.substr(keyTemplate_.size(), std::string::npos);
}

} // namespace catchdb

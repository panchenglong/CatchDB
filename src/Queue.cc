#include "Queue.h"
#include "Logger.h"
#include "Util.h"
#include "leveldb/db.h"
#include "leveldb/options.h"
#include "leveldb/write_batch.h"
#include <cstring>
#include <limits>
#include <stdexcept>
#include <cstdio>

namespace catchdb
{

namespace
{
const char QUEUE_TYPE_INDENTIFIRE = 'Q';
const uint64_t META_RECORD_SEQ = 0;
const uint64_t ITEM_MIN_SEQ = 1;
const uint64_t ITEM_MAX_SEQ = std::numeric_limits<uint64_t>::max() / 2;
const uint64_t ITEM_SEQ_INIT = ITEM_MAX_SEQ / 2;
} // namespace


Queue::Queue(const CatchDBPtr db, const std::string &queueName)
    : db_(db), name_(queueName), size_(0)
{
    procMap = { { "qsize", &Queue::size },
                { "qfront", &Queue::front },
                { "qback", &Queue::back },
                { "qpush", &Queue::pushBack },
                { "qpush_front", &Queue::pushFront },
                { "qpush_back", &Queue::pushBack },
                { "multi-qpush", &Queue::pushBackM },
                { "multi-qpush_front", &Queue::pushFrontM },
                { "multi-qpush_back", &Queue::pushBackM },
                { "qpop", &Queue::popFront },
                { "qpop_front", &Queue::popFront },
                { "qpop_back", &Queue::popBack },
                { "qclear", &Queue::clear },
                { "qlist", &Queue::list },
                { "qslice", &Queue::slice },
                { "qget", &Queue::get } };

    keyTemplate_.append(1, QUEUE_TYPE_INDENTIFIRE);
    uint16_t keySize = static_cast<uint16_t>(queueName.size());
    keyTemplate_.append((char *)&keySize, sizeof (uint16_t));
    keyTemplate_.append(queueName.data(), keySize);

    Response resp;
    (void) size(nullptr, &resp); // do not care whether succeed
}

Status Queue::process(const RequestPtr req, ResponsePtr resp)
{
    std::string &cmd = req->blocks[0];
    if (procMap.find(cmd) == procMap.end())
        return Status::NotImplemented;
    auto func = procMap[cmd];
    return (this->*func)(req, resp);
}

Status Queue::size(const RequestPtr &req, ResponsePtr resp)
{
    (void) req;
    if (size_ == 0) {
        std::string val;
        auto s = get_(META_RECORD_SEQ, &val);
        if (s == Status::OK) {
            auto v = decodeMetaValue(val);
            size_ = std::get<0>(v);
            frontSeq_ = std::get<1>(v);
            backSeq_ = std::get<2>(v);
        } else if (s == Status::NotFound){
            size_ = 0;
            frontSeq_ = ITEM_SEQ_INIT - 1;
            backSeq_ = ITEM_SEQ_INIT;
        } else {
            return Status::Error;
        }
    }
    resp->push_back(std::to_string(size_));
    return Status::OK;
}

bool Queue::empty()
{
    return size_ == 0;
}

Status Queue::front(const RequestPtr &req, ResponsePtr resp)
{
    (void) req;
    std::string val;
    auto s = get_(frontSeq_ + 1, &val);
    resp->push_back(val);
    return s;
}

Status Queue::back(const RequestPtr &req, ResponsePtr resp)
{
    (void) req;
    std::string val;
    auto s =  get_(backSeq_ - 1, &val);
    resp->push_back(val);
    return s;
}

Status Queue::get(const RequestPtr &req, ResponsePtr resp)
{
    int64_t index;
    try {
        index = std::stoll(req->blocks[2]);
    } catch(...) {
        return Status::InvalidParameter;
    }
    uint64_t idx = (index >= 0) ? index : (size_ + index);
    if (idx < 0 || idx >= size_)
        return Status::OutOfRange;

    std::string val;
    auto s = get_(frontSeq_ + 1 + idx, &val);
    resp->push_back(val);
    return s;
}

Status Queue::slice(const RequestPtr &req, ResponsePtr resp)
{
    // TODO IMPLEMENTED USING ITERATOR

    int64_t start;
    int64_t num;

    try {
        start = std::stoll(req->blocks[2]);
        num = std::stoll(req->blocks[3]);
    } catch(...) {
        return Status::InvalidParameter;
    }

    if (num < 0)
        return Status::InvalidParameter;

    uint64_t idx = (start >= 0) ? start : start + size_;
    if (idx < 0 || idx >= size_)
        return Status::InvalidParameter;

    for (auto i = 0; i < num; ++i) {
        std::string val;
        auto s = get_(frontSeq_ + 1 + i, &val);
        if (s != Status::OK)
            return s;
        resp->push_back(val);
    }

    return Status::OK;
}

Status Queue::pushFront(const RequestPtr &req, ResponsePtr resp)
{
    (void) resp;
    std::string &value = req->blocks[2];
    return push(Direction::Front, value);
}

Status Queue::pushFrontM(const RequestPtr &req, ResponsePtr resp)
{
    (void) resp;
    std::vector<std::string> values(resp->begin() + 2, resp->end());
    return pushM(Direction::Front, values);
}

Status Queue::pushBack(const RequestPtr &req, ResponsePtr resp)
{
    (void) resp;
    std::string &value = req->blocks[2];
    return push(Direction::Back, value);
}

Status Queue::pushBackM(const RequestPtr &req, ResponsePtr resp)
{
    (void) resp;
    std::vector<std::string> values(resp->begin() + 2, resp->end());
    return pushM(Direction::Back, values);
}

Status Queue::popFront(const RequestPtr &req, ResponsePtr resp)
{
    (void) req;
    (void) resp;
    return pop(Direction::Front);
}

Status Queue::popBack(const RequestPtr &req, ResponsePtr resp)
{
    (void) req;
    (void) resp;
    return pop(Direction::Back);
}

Status Queue::clear(const RequestPtr &req, ResponsePtr resp)
{
    leveldb::WriteBatch batch;
    for (uint64_t i = frontSeq_ + 1; i < backSeq_; ++i) {
        auto key = encodeKey(i);
        batch.Delete(key);
    }
    auto key = encodeKey(META_RECORD_SEQ);
    batch.Delete(key);
    auto s = db_->putM(&batch);
    if (s == Status::OK) {
        size_ = 0;
        frontSeq_ = ITEM_SEQ_INIT - 1;
        backSeq_ = ITEM_SEQ_INIT;
    }
    return s;
}

Status Queue::list(const RequestPtr &req, ResponsePtr resp)
{
    for (uint64_t i = frontSeq_ + 1; i < backSeq_; ++i) {
        std::string val;
        auto s = get_(i, &val);
        if (s != Status::OK)
            return s;
        resp->push_back(val);
    }

    return Status::OK;
}


/*************** private member functions *******************/
Status Queue::get_(uint64_t seq, std::string *ret)
{
    auto key = encodeKey(seq);
    return db_->get(key, ret);
}

Status Queue::push(Direction direction, const std::string &value)
{
    uint64_t seq, fseq, bseq;
    if (direction == Direction::Front) {
        seq = frontSeq_;
        fseq = frontSeq_ - 1;
        bseq = backSeq_;
    } else {
        fseq = frontSeq_;
        seq = backSeq_;
        bseq = backSeq_ + 1;
    }
    leveldb::WriteBatch batch;
    auto key1 = encodeKey(seq);
    batch.Put(key1, value);
    auto key2 = encodeKey(META_RECORD_SEQ);
    auto value2 = encodeMetaValue(size_ + 1, fseq, bseq);
    batch.Put(key2, value2);
    Status s = db_->putM(&batch);
    if (s == Status::OK) {
        ++size_;
        frontSeq_ = fseq;
        backSeq_ = bseq;
        return Status::OK;
    }
    return Status::Error;
}

Status Queue::pushM(Direction direction, const std::vector<std::string> &values)
{
    uint64_t seq;
    int step;
    if (direction == Direction::Front) {
        seq = frontSeq_ + 1;
        step = -1;
    } else {
        seq = backSeq_ - 1;
        step = 1;
    }

    leveldb::WriteBatch batch;
    for (auto &value : values) {
        seq += step;
        auto key = encodeKey(seq);
        batch.Put(key, value);
    }
    auto key = encodeKey(META_RECORD_SEQ);
    std::string value;
    if (direction == Direction::Front) {
        value = encodeMetaValue(size_ + values.size(), seq - 1, backSeq_);
    } else {
        value = encodeMetaValue(size_ + values.size(), frontSeq_, seq + 1);
    }
    batch.Put(key, value);
    Status s = db_->putM(&batch);
    if (s == Status::OK) {
        size_ += values.size();
        if (direction == Direction::Front) {
            frontSeq_ = seq;
        } else {
            backSeq_ = seq;
        }
        return Status::OK;
    }
    return Status::Error;
}

Status Queue::pop(Direction direction)
{
    if (size_ == 0)
        return Status::Empty;

    uint64_t fseq = frontSeq_;
    uint64_t bseq = backSeq_;
    leveldb::WriteBatch batch;
    if (direction == Direction::Front) {
        auto key = encodeKey(frontSeq_ + 1);
        batch.Delete(key);
        ++fseq;
    } else {
        auto key = encodeKey(backSeq_ - 1);
        batch.Delete(key);
        --bseq;
    }
    auto key2 = encodeKey(META_RECORD_SEQ);
    if (size_ == 1) {
        batch.Delete(key2);
    } else {
        auto value = encodeMetaValue(size_ - 1, fseq, bseq);
        batch.Put(key2, value);
    }
    Status s = db_->putM(&batch);
    if (s == Status::OK) {
        --size_;
        frontSeq_ = fseq;
        backSeq_ = bseq;
        return Status::OK;
    }
    return Status::Error;
}

std::string Queue::encodeKey(uint64_t seq)
{
    std::string key(keyTemplate_);
    key.append((char *)&seq, sizeof (uint64_t));
    return key;
}

std::string Queue::encodeMetaValue(uint64_t queueSize, uint64_t frontSeq, uint64_t backSeq)
{
    std::string val;
    val.append((char *)&queueSize, sizeof (uint64_t));
    uint64_t h = ToBigEndian(frontSeq);
    val.append((char *)&h, sizeof (uint64_t));
    uint64_t t = ToBigEndian(backSeq);
    val.append((char *)&t, sizeof (uint64_t));
    return val;
}

std::tuple<uint64_t, uint64_t, uint64_t> Queue::decodeMetaValue(const std::string &val)
{
    const char *v = val.data();
    uint64_t queueSize = *((uint64_t *)v);
    uint64_t frontSeq = *((uint64_t *)(v + sizeof (uint64_t)));
    frontSeq = ToLittleEndian(frontSeq);
    uint64_t backSeq = *((uint64_t *)(v + sizeof (uint64_t) + sizeof (uint64_t)));
    backSeq = ToLittleEndian(backSeq);

    return std::make_tuple(queueSize, frontSeq, backSeq);
}

} // namespace catchdb

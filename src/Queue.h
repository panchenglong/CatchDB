/*
 * Queue is stored in a serial of key-value records. Every record
 * consists of one write. 
 * 
 * With better design of key, records of the same type, same structure
 * will be stored contiguously, or at least nearby. Here I rely on the 
 * custom comparator function to achieve this goal. See AggregateComparator.hh
 *
 * The idea is adapted from SSDB, with modification.
 * record format:
 * 'Q' | key_size | key | value
 * item record: key := SizeQueueName + QueueName + Sequence [ITEM_MIN_SEQ, ITEM_MAX_SEQ]; value := ItemValue
 * meta record: key := SizeQueueName + QueueName + Sequence 0; value := SIZE +QUEUE_FRONT_SEQ + QUEUE_BACK_SEQ
 * SizeQueueName : 2 bytes, i.e. key size is limited to 65536
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

class Queue
{
public:
    Queue(const CatchDBPtr db, const std::string &queueName);
    ~Queue() {}

    Status process(const RequestPtr req, ResponsePtr resp);

    Status size(const RequestPtr &req, ResponsePtr resp);

    bool empty();

    Status front(const RequestPtr &req, ResponsePtr resp);
    Status back(const RequestPtr &req, ResponsePtr resp);
    Status get(const RequestPtr &req, ResponsePtr resp);

    // qslice name begin limit -> [begin, begin + limit)
    Status slice(const RequestPtr &req, ResponsePtr resp);

    Status pushFront(const RequestPtr &req, ResponsePtr resp);
    Status pushFrontM(const RequestPtr &req, ResponsePtr resp);
    Status pushBack(const RequestPtr &req, ResponsePtr resp);
    Status pushBackM(const RequestPtr &req, ResponsePtr resp);

    Status popFront(const RequestPtr &req, ResponsePtr resp);
    Status popBack(const RequestPtr &req, ResponsePtr resp);

    Status clear(const RequestPtr &req, ResponsePtr resp);

    // get all items
    Status list(const RequestPtr &req, ResponsePtr resp);

    // non-copyable
    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

private:
    typedef Status (Queue::*proc_t) (const RequestPtr&, ResponsePtr);
    // std::map<std::string, 
    //          std::function<Status (const RequestPtr&, ResponsePtr)>> procMap;

    std::map<std::string, proc_t> procMap;

    Status get_(uint64_t seq, std::string *ret);

    enum class Direction { Front, Back };
    Status push(Direction direction, const std::string &ret);
    Status pushM(Direction direction, const std::vector<std::string> &ret);
    Status pop(Direction direction);

    std::string encodeKey(uint64_t seq);
    std::string encodeMetaValue(uint64_t queueSize, uint64_t frontSeq, uint64_t backSeq);
    std::tuple<uint64_t, uint64_t, uint64_t> decodeMetaValue(const std::string &val);

    CatchDBPtr db_;
    std::string name_;
    std::string keyTemplate_;
    uint64_t size_;
    uint64_t frontSeq_; // points to where TO BE insertd NEXT
    uint64_t backSeq_;
};

typedef std::unique_ptr<Queue> QueuePtr;

} // namespace catchdb

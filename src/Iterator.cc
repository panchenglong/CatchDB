#include "Iterator.h"
#include "Util.h"
#include <limits>

namespace catchdb
{

Iterator::Iterator(leveldb::DB *db, 
                   const std::string &prefix,
                   const std::string &exclude)
    : db_(db), prefix_(prefix), exclude_(exclude)
{
    leveldb::ReadOptions options;
    options.fill_cache = false;
    it_ = db_->NewIterator(options);
}

Iterator::~Iterator()
{
    delete it_;
}

std::vector<KVPair> Iterator::range(const std::string &start,
                          const std::string &end,
                          uint64_t limit)
{
    std::vector<KVPair> res;

    if (limit == 0) {
        limit = std::numeric_limits<uint64_t>::max();
    }
    int count = 0;
    it_->Seek(start);
    if (end.empty()) {
        while (it_->Valid()){
            if (BeginWith(it_->key().ToString(), prefix_) &&
                it_->key() != exclude_) {
                res.push_back(KVPair(it_->key().ToString(),
                                     it_->value().ToString()));
                ++count;
                if (count == limit)
                    break;
            }
            it_->Next();
        }
    } else {
        while (it_->Valid() && CatchDBCompare(it_->key().ToString(), end) <= 0){
            if (BeginWith(it_->key().ToString(), prefix_) &&
                it_->key() != exclude_) {
                res.push_back(KVPair(it_->key().ToString(),
                                     it_->value().ToString()));
                ++count;
                if (count == limit)
                    break;
            }
            it_->Next();
        }

    }

    return res;
}

std::vector<std::string> Iterator::keys(const std::string &start,
                              const std::string &end,
                              uint64_t limit)
{
    std::vector<std::string> res;

    if (limit == 0) {
        limit = std::numeric_limits<uint64_t>::max();
    }
    int count = 0;
    it_->Seek(start);
    if (end.empty()) {
        while (it_->Valid()){
            if (BeginWith(it_->key().ToString(), prefix_) &&
                it_->key() != exclude_) {
                res.push_back(it_->key().ToString());
                ++count;
                if (count == limit)
                    break;
            }
            it_->Next();
        }
    } else {
        while (it_->Valid() && CatchDBCompare(it_->key().ToString(), end) <= 0){
            if (BeginWith(it_->key().ToString(), prefix_) &&
                it_->key() != exclude_) {
                res.push_back(it_->key().ToString());
                ++count;
                if (count == limit)
                    break;
            }
            it_->Next();
        }

    }

    return res;
}

std::vector<std::string> Iterator::values(const std::string &start,
                                const std::string &end,
                                uint64_t limit)
{
    std::vector<std::string> res;

    if (limit == 0) {
        limit = std::numeric_limits<uint64_t>::max();
    }
    int count = 0;
    it_->Seek(start);
    if (end.empty()) {
        while (it_->Valid()){
            if (BeginWith(it_->key().ToString(), prefix_) &&
                it_->key() != exclude_) {
                res.push_back(it_->value().ToString());
                ++count;
                if (count == limit)
                    break;
            }
            it_->Next();
        }
    } else {
        while (it_->Valid() && CatchDBCompare(it_->key().ToString(), end) <= 0){
            if (BeginWith(it_->key().ToString(), prefix_) &&
                it_->key() != exclude_) {
                res.push_back(it_->value().ToString());
                ++count;
                if (count == limit)
                    break;
            }
            it_->Next();
        }

    }

    return res;
}

Status Iterator::status()
{
    if (it_->status().ok()) {
        return Status::OK;
    } else {
        return Status::Error;
    }
}

} // namespace catchdb

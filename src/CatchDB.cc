#include "CatchDB.h"
#include "Logger.h"
#include "AggregateComparator.hh"
#include "leveldb/filter_policy.h"
#include "leveldb/cache.h"

namespace catchdb
{

CatchDB::CatchDB(leveldb::DB *db, const std::string &name) 
    : ldb_(db), name_(name) {}

CatchDB::~CatchDB()
{
    if (ldb_) {
        delete ldb_;
    }
}

Status CatchDB::get(const std::string &key, std::string *ret)
{
    leveldb::Status s = ldb_->Get(leveldb::ReadOptions(), key, ret);
    if (s.IsNotFound()) {
        return Status::NotFound;
    } else if (!s.ok()) {
        LogError(s.ToString().c_str());
        return Status::Error;
    } else {
        return Status::OK;
    }
}

Status CatchDB::put(const std::string &key, const std::string &value)
{
    leveldb::Status s = ldb_->Put(leveldb::WriteOptions(), key, value);
    if (s.ok()) {
        return Status::OK;
    } else {
        return Status::Error;
    }
}

Status CatchDB::del(const std::string &key)
{
    leveldb::Status s = ldb_->Delete(leveldb::WriteOptions(), key);
    if (s.ok()) {
        return Status::OK;
    } else {
        return Status::Error;
    }
}

Status CatchDB::putM(leveldb::WriteBatch *batch)
{
   leveldb::Status s = ldb_->Write(leveldb::WriteOptions(), batch);
    if (s.ok()) {
        return Status::OK;
    } else {
        LogError(s.ToString().c_str());
        return Status::Error;
    }
}

Iterator* CatchDB::newIterator(const std::string &prefix, 
                               const std::string &exclude)
{
    return new Iterator(ldb_, prefix, exclude);
}

CatchDBPtr CatchDB::Open(const ConfigPtr &config)
{
    leveldb::Options options;
    options.create_if_missing = true;
    options.filter_policy = leveldb::NewBloomFilterPolicy(10);
    options.block_cache = leveldb::NewLRUCache(config->cacheSize * 1048576);
    options.block_size = config->blockSize * 1024;
    options.write_buffer_size = config->writeBufferSize * 1024 * 1024;
    if (config->compression) {
        options.compression = leveldb::kSnappyCompression;
    } else {
        options.compression = leveldb::kNoCompression;
    }
    options.comparator = new AggregateComparator;

    std::string dbName = config->dbPath + config->dbName;
    leveldb::DB *db;
    auto status = leveldb::DB::Open(options, dbName, &db);
    if (!status.ok())
        return nullptr;

    return CatchDBPtr(new CatchDB(db, dbName));
}

} // namespace catchdb

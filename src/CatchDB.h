#pragma once

#include <memory>
#include <string>
#include "leveldb/db.h"
#include "Status.h"
#include "Config.h"
#include "Iterator.h"

namespace catchdb
{

class CatchDB;
typedef std::shared_ptr<CatchDB> CatchDBPtr;

class CatchDB
{
public:
    CatchDB(leveldb::DB *db, const std::string &name);
    ~CatchDB();

    static CatchDBPtr Open(const ConfigPtr &config);

    CatchDB(const CatchDB&) = delete;
    CatchDB& operator=(const CatchDB&) = delete;

    Status get(const std::string &key, std::string *ret);
    Status put(const std::string &key, const std::string &value);
    Status del(const std::string &key);
    Status putM(leveldb::WriteBatch *batch);

    Iterator* newIterator(const std::string &prefix, 
                          const std::string &exclude="");
private:

    leveldb::DB *ldb_;
    std::string name_;
};

} // namespace catchdb

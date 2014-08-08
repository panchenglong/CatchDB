#pragma once

#include "CatchDB.h"
#include "Status.h"
#include "Protocol.h"

namespace catchdb
{

namespace KV
{

Status process(const CatchDBPtr db, const RequestPtr req, ResponsePtr resp);

Status get(const CatchDBPtr db, const RequestPtr req, ResponsePtr resp);
Status getall(const CatchDBPtr db, const RequestPtr req, ResponsePtr resp);
Status set(const CatchDBPtr db, const RequestPtr req, ResponsePtr resp);
Status setM(const CatchDBPtr db, const RequestPtr req, ResponsePtr resp);
Status del(const CatchDBPtr db, const RequestPtr req, ResponsePtr resp);
Status exists(const CatchDBPtr db, const RequestPtr req, ResponsePtr resp);
Status keys(const CatchDBPtr db, const RequestPtr req, ResponsePtr resp);

} // namespace KV

} // namespace catchdb

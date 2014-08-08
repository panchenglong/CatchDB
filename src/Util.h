#pragma once

#include <string>

namespace catchdb
{

void StringToLower(char *str, size_t num);

uint64_t ToBigEndian(uint64_t n);

uint64_t ToLittleEndian(uint64_t n);

template<typename Num>
std::string NumberToString(Num n)
{
    std::string buf;
    buf.append((char *)&n, sizeof (n));
    return buf;
}

template<typename Num>
Num StringToNumber(const std::string &s)
{
    return *((Num *) s.data());
}

const char *ErrorDescription(int errnum);

bool BeginWith(const std::string &a, const std::string &b);

// the same as Comapare function in AggregateComparator
int CatchDBCompare(const std::string& a, const std::string& b);

} // namespace catchdb

/*
 * Custom comparator that aggregate records of same type and same instance.
 * Because LevelDB store records in order, records with adjacent keys will 
 * be stored adjacently.
 *
 * Common format of keys in catchdb data structures:
 * | type | size content | size content | ...
 * type: 1 byte
 * size: 2 bytes
 * content: size bytes
 */

#pragma once

#include <cstdint>
#include <cstring>
#include "leveldb/comparator.h"

namespace catchdb
{

class AggregateComparator : public leveldb::Comparator
{
    //   if a < b: negative result
    //   if a > b: positive result
    //   else: zero result
    int Compare(const leveldb::Slice& a, const leveldb::Slice& b) const
    {
        if (a[0] != b[0])
            return a[0] - b[0];

        switch (a[0]) {
            case 'K':
                return a.compare(b);
            case 'H': {
                uint16_t sizea = *((uint16_t*)(a.data() + 1));
                uint16_t sizeb = *((uint16_t*)(b.data() + 1));
                if (sizea != sizeb)
                    return sizea - sizeb;
                if (a.size() != b.size())
                    return a.size() - b.size();
                return memcmp(a.data() + 3 + sizea,
                              b.data() + 3 + sizea,
                              a.size() - 3 - sizea);
            }
            case 'Q': {
                uint16_t sizea = *((uint16_t*)(a.data() + 1));
                uint16_t sizeb = *((uint16_t*)(b.data() + 1));
                if (sizea != sizeb)
                    return sizea - sizeb;
                return memcmp(a.data() + 3, b.data() + 3, 8);
            }
            case 'Z': {
                // check name first
                if (a.size() < 4 || b.size() < 4)
                    return a.compare(b);

                uint16_t sizea = *((uint16_t*)(a.data() + 2));
                uint16_t sizeb = *((uint16_t*)(b.data() + 2));
                if (sizea != sizeb)
                    return sizea - sizeb;
                
                int ret = memcmp(a.data() + 4, b.data() + 4, sizea);
                if (ret != 0) {
                    return ret;
                }

                // Key or Score
                if (a[1] != b[1])
                    return a[1] - b[1];

                const char *aa = a.data() + 4 + sizea;
                const char *bb = b.data() + 4 + sizea;
                int sizeNow = 4 + sizea;
                if (a[1] == 'S') {
                    if ((a.size() - sizeNow >= sizeof (int64_t)) &&
                        (b.size() - sizeNow >= sizeof (int64_t))) {
                        int64_t scorea = *((int64_t*)aa);
                        int64_t scoreb = *((int64_t*)bb);

                        if (scorea != scoreb)
                            return scorea - scoreb;
                        sizeNow += sizeof (int64_t);
                    }
                    if (a.size() == b.size()) {
                        return memcmp(aa + sizeof (int64_t), 
                                      bb + sizeof (int64_t), 
                                      a.size() - sizeNow);
                    }
                    return a.size() - b.size();

                } else {
                    if (a.size() == b.size()) {
                        return memcmp(aa, bb, a.size() - 4 - sizea);
                    }
                    return a.size() - b.size();
                }
            }
            default:
                return a.compare(b);
        }
    }

    const char* Name() const
    {
        return "catchdb.AggregateComparator";
    }

    void FindShortestSeparator(std::string* start, const leveldb::Slice& limit) const
    {
    }

    void FindShortSuccessor(std::string* key) const
    {
    }
};

} // namespace catchdb

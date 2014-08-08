#include "Util.h"
#include <endian.h>
#include <string.h>

namespace catchdb
{

void StringToLower(char *str, size_t num)
{
    for (int i = 0; i < num && *str != '\0'; ++i) {
        *str = tolower(*str);
    }
}

uint64_t ToBigEndian(uint64_t n)
{
    return htobe64(n);
}

uint64_t ToLittleEndian(uint64_t n)
{
    return htole64(n);
}


const char *ErrorDescription(int errnum)
{
    char buf[1024];
    memset(buf, 0, 1024);

    return strerror_r(errnum, buf, 1024);
}

bool BeginWith(const std::string &a, const std::string &b)
{
    if (a.size() < b.size()) {
        return false;
    }

    if (memcmp(a.data(), b.data(), b.size()) == 0) {
        return true;
    } else {
        return false;
    }
}

int CatchDBCompare(const std::string& a, const std::string& b)
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

            if (a[1] == 'S') {
                int64_t scorea = *((int64_t*)aa);
                int64_t scoreb = *((int64_t*)bb);
                if (scorea != scoreb)
                    return scorea - scoreb;
                if (a.size() == b.size()) {
                    return memcmp(aa + sizeof (int64_t), 
                                  bb + sizeof (int64_t), 
                                  a.size() - 4 - sizea - sizeof (int64_t));
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


} // namespace catchdb

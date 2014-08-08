#pragma once

namespace catchdb
{

class Buffer
{
public:
    Buffer(int size);
    ~Buffer();

    bool full();
    int avail();
    void reset();
    char* data();
    char* tail();
    int size();
    void incr(int len);
    void decr(int len);

private:
    int size_;
    int avail_;
    int start_;
    int pos_;
    char *buf_;
    bool accrossBuf_;
};


} // namespace catchdb

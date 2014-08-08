#include "Buffer.h"
#include <cstring>
#include <cassert>

namespace catchdb
{

Buffer::Buffer(int size)
    : size_(size), avail_(size), start_(0), pos_(0), accrossBuf_(false)
{
    buf_ = new char[size * 2];
}

Buffer::~Buffer()
{
    delete[] buf_;
}

int Buffer::avail()
{
    return avail_;
}

bool Buffer::full()
{
    return avail_ == 0;
}

void Buffer::reset()
{
    start_ = 0;
    pos_ = 0;
    avail_ = size_;
    accrossBuf_ = false;
}

char* Buffer::data()
{
    return buf_ + start_;
}

int Buffer::size()
{
    return size_ - avail_;
}

char* Buffer::tail()
{
    return accrossBuf_ ? buf_ + size_ + pos_ : buf_ + pos_;
}

void Buffer::incr(int len)
{
    assert(len <= avail_);
    pos_ = (pos_ + len) % size_;
    avail_ -= len;
    if (accrossBuf_ == false && pos_ < start_ && pos_ != 0) {
        accrossBuf_ = true; // data span accross two bufs
    }
}

void Buffer::decr(int len)
{
    assert(len <= size_ - avail_);
    avail_ += len;
    start_ = (start_ + len) % size_;
    if (accrossBuf_ == true && start_ <= pos_) {
        // now all data resides in buf2
        memcpy(buf_, buf_ + size_ + start_, pos_ - start_);
        pos_ -= start_;
        start_ = 0;
        accrossBuf_ = false;
    }
}

/*********** private method ************/

} // namespace catchdb

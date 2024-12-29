#include "stream.h"

#include <cstdlib>
#include <cstring>

#include "base_macro.h"

const int GROW_BLOCK = 64 * 1024;
const int stream_max_size = 16 * 1024 * 1024;

Stream::Stream()
{
    capacity_ = GROW_BLOCK;
    buf_ = (char*)malloc(capacity_);
    if (buf_ == nullptr) {
        ASSERT(buf_ != nullptr);
        abort();
    }
}

Stream::~Stream()
{
    if (buf_ != nullptr) {
        free(buf_);
        buf_ = nullptr;
    }
    rpos_ = 0;
    size_ = 0;
    capacity_ = 0;
}

int Stream::Append(const char* data, int data_len)
{
    if (EnsureWritable(data_len) != 0) return -1;
    memcpy(buf_ + size_, data, data_len);
    size_ = size_ + data_len;
    return 0;
}

int Stream::EnsureWritable(int data_len)
{
    if (data_len <= 0) {
        return 0;
    }

    Shrink();
    const int expect_size = size_ + data_len;
    if (expect_size > stream_max_size) {
        return -1;
    }
    if (capacity_ < expect_size) {
        capacity_ = ROUNDUP(expect_size, GROW_BLOCK);
        char* new_buf = (char*)malloc(capacity_);
        if (new_buf == nullptr) {
            ASSERT(new_buf != nullptr);
            abort();
        }
        if (size_ > 0) {
            memcpy(new_buf, buf_, size_);
        }
        free(buf_);
        buf_ = new_buf;
    }
    return 0;
}

void Stream::Skip(int offset)
{
    if (offset < 0) return;
    if (rpos_ + offset < size_) {
        rpos_ += offset;
    } else {
        rpos_ = 0;
        size_ = 0;
    }
}

void Stream::Reset()
{
    rpos_ = 0;
    size_ = 0;
}

void Stream::Shrink()
{
    if (rpos_ > 0) {
        int readable_len = size_ - rpos_;
        memmove(buf_, buf_ + rpos_, readable_len);
        rpos_ = 0;
        size_ = readable_len;
    }
}

void Stream::AddSize(int n)
{
    if (size_ + n <= capacity_) {
        size_ += n;
    }
}

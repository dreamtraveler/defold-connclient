#pragma once

class Stream
{
public:
    Stream();
    ~Stream();

public:
    int Append(const char* data, int data_len);
    int EnsureWritable(int data_len);
    void Skip(int offset);
    void Reset();
    void Shrink();
    char* End() { return buf_ + size_; }
    int Len() { return size_ - rpos_; }
    char* Buf() { return buf_ + rpos_; }
    void AddSize(int n);

public:
    int size() { return size_; }
    int capacity() { return capacity_; }

private:
    char* buf_ = {nullptr};
    int rpos_ = {0};
    int size_ = {0};
    int capacity_ = {0};
};

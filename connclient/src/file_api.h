#pragma once
#include <string>

#include "base_macro.h"
#ifdef OS_WIN32
#include <direct.h>
#include <fcntl.h>  // for _open()/_close()/_read()/_write()...
#include <io.h>     // for _open()

#include <cstring>  // for memcpy()
#else
#include <dirent.h>     // for opendir()
#include <fcntl.h>      // for fcntl()
#include <sys/ioctl.h>  // for ioctl()
#include <sys/stat.h>   // for open()
#include <sys/types.h>  // for open()
#include <unistd.h>     // for fcntl()

#include <cerrno>  // for errno
#endif

using namespace std;
#ifdef OS_WIN32
#define FILE_PATH_SLASH "\\"
#else
#define FILE_PATH_SLASH "/"
#define O_BINARY 0
#endif

namespace FileAPI
{
int open_ex(const char* filename, int flags);
int open_ex(const char* filename, int flags, int mode);
void close_ex(int fd);
int mkdir_ex(const char* dirname, int mode);
long read_ex(int fd, void* buf, uint32_t len);
long write_ex(int fd, const void* buf, uint32_t len);
int fcntl_ex(int fd, int cmd);
int fcntl_ex(int fd, int cmd, int64_t arg);
bool getfilenonblocking_ex(int fd);
void setfilenonblocking_ex(int fd, bool on);
void ioctl_ex(int fd, int request, void* argp);
void setfilenonblocking_ex2(int fd, bool on);
uint32_t availablefile_ex(int fd);
int dup_ex(int fd);
int64_t lseek_ex(int fd, int64_t offset, int whence);
int64_t tell_ex(int fd);
void list_files(const string& folder_path, const char* file_suffix, bool rm_folder_path,
                vector<string>& file_names_vec);

template <class T>
T base_name(T const& path, T const& delims = "/\\")
{
    return path.substr(path.find_last_of(delims) + 1);
}
template <class T>
T remove_extension(T const& filename)
{
    typename T::size_type const p(filename.find_last_of('.'));
    return p > 0 && p != T::npos ? filename.substr(0, p) : filename;
}
};  // namespace FileAPI

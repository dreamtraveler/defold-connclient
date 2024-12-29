#include "file_api.h"

#include <iostream>


int FileAPI::open_ex(const char* filename, int flags)
{
#ifndef OS_WIN32
    int fd = open(filename, flags);
#else
    int fd = _open(filename, flags);
#endif

    return fd;
}

int FileAPI::open_ex(const char* filename, int flags, int mode)
{
#ifndef OS_WIN32
    int fd = open(filename, flags, mode);
#else
    int fd = _open(filename, flags, mode);
#endif

    return fd;
}

int FileAPI::mkdir_ex(const char* dirname, int mode)
{
#ifndef OS_WIN32
    int result = mkdir(dirname, mode);
#else
    int result = _mkdir(dirname);
#endif

    return result;
}


long FileAPI::read_ex(int fd, void* buf, uint32_t len)
{
#ifndef OS_WIN32
    long result = read(fd, buf, len);
#else
    long result = _read(fd, buf, len);
#endif
    return result;
}

long FileAPI::write_ex(int fd, const void* buf, uint32_t len)
{
#ifndef OS_WIN32
    long result = write(fd, buf, len);
#else
    long result = _write(fd, buf, len);
#endif
    return result;
}

void FileAPI::close_ex(int fd)
{
    close(fd);
}

int FileAPI::fcntl_ex(int fd, int cmd)
{
#ifndef OS_WIN32
    int result = fcntl(fd, cmd);
    return result;
#else
    return 0;
#endif
}

int FileAPI::fcntl_ex(int fd, int cmd, int64_t arg)
{
#ifndef OS_WIN32
    int result = fcntl(fd, cmd, arg);
    return result;
#else
    return 0;
#endif
}

bool FileAPI::getfilenonblocking_ex(int fd)
{
#ifndef OS_WIN32
    int flags = fcntl_ex(fd, F_GETFL, 0);
    return flags | O_NONBLOCK;
#else
    return FALSE;
#endif
}

void FileAPI::setfilenonblocking_ex(int fd, bool on)
{
#ifndef OS_WIN32
    int flags = fcntl_ex(fd, F_GETFL, 0);

    if (on)
        // make nonblocking fd
        flags |= O_NONBLOCK;
    else
        // make blocking fd
        flags &= ~O_NONBLOCK;

    fcntl_ex(fd, F_SETFL, flags);
#else
#endif
}

void FileAPI::ioctl_ex(int fd, int request, void* argp)
{
#ifndef OS_WIN32
    ioctl(fd, request, argp);
#else
#endif
}

void FileAPI::setfilenonblocking_ex2(int fd, bool on)
{
#ifndef OS_WIN32
    uint64_t arg = (on == true ? 1 : 0);
    ioctl_ex(fd, FIONBIO, &arg);
#else
#endif
}

uint32_t FileAPI::availablefile_ex(int fd)
{
#ifndef OS_WIN32
    uint32_t arg = 0;
    ioctl_ex(fd, FIONREAD, &arg);
    return arg;
#else
    return 0;
#endif
}

int FileAPI::dup_ex(int fd)
{
#ifndef OS_WIN32
    int newfd = dup(fd);
#else
    int newfd = _dup(fd);
#endif
    return newfd;
}

int64_t FileAPI::lseek_ex(int fd, int64_t offset, int whence)
{
#ifndef OS_WIN32
    int64_t result = lseek(fd, offset, whence);
#else
    int64_t result = _lseek(fd, offset, whence);
#endif

    return result;
}

int64_t FileAPI::tell_ex(int fd)
{
#ifndef OS_WIN32
    int64_t result = 0;
#else
    int64_t result = _tell(fd);
    if (result < 0) {
    }
#endif

    return result;
}

void FileAPI::list_files(const string& folder_path, const char* file_suffix, bool rm_folder_path,
                         vector<string>& file_names_vec)
{
#ifdef OS_WIN32
    _finddata_t file;
    intptr_t flag;
    string filename;
    if (file_suffix == nullptr) {
        cout << "file_suffix == nullptr" << endl;
        return;
    }
    filename = folder_path + FILE_PATH_SLASH + "*" + file_suffix;
    if ((flag = _findfirst(filename.c_str(), &file)) == -1) {
        cout << "There is no such type file [." << file_suffix << "]" << endl;
    } else {
        if (!rm_folder_path)
            file_names_vec.push_back(string(folder_path + FILE_PATH_SLASH + string(file.name)));
        else
            file_names_vec.push_back(string(file.name));
        while (_findnext(flag, &file) == 0) {
            if (!rm_folder_path)
                file_names_vec.push_back(string(folder_path + FILE_PATH_SLASH + string(file.name)));
            else
                file_names_vec.push_back(string(file.name));
        }
    }
    _findclose(flag);
#else
    DIR* dir = opendir(folder_path.c_str());
    dirent* p = nullptr;
    while ((p = readdir(dir)) != nullptr) {
        if (p->d_name[0] != '.') {
            string name = p->d_name;
            if (file_suffix == nullptr) {
                cout << "file_suffix == nullptr" << endl;
                return;
            }
            string str_file_suffix = file_suffix;
            if (name.size() <= str_file_suffix.size()) continue;
            std::size_t suffix_found =
                name.find(str_file_suffix, name.size() - str_file_suffix.size());
            if (suffix_found == std::string::npos) continue;
            if (!rm_folder_path)
                file_names_vec.emplace_back(
                    string(folder_path + FILE_PATH_SLASH + string(p->d_name)));
            else
                file_names_vec.emplace_back(p->d_name);
        }
    }
    closedir(dir);
#endif
}

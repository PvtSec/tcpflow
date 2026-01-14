#ifndef BE13_SAFE_OPEN_H
#define BE13_SAFE_OPEN_H

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <ostream>
#include <streambuf>
#include <sys/stat.h>
#include <unistd.h>

namespace be13 {

inline int open_no_symlink(const char *path, int flags, mode_t mode)
{
    int open_flags = flags;
#ifdef O_NOFOLLOW
    open_flags |= O_NOFOLLOW;
#endif
#ifdef O_CLOEXEC
    open_flags |= O_CLOEXEC;
#endif
    return ::open(path, open_flags, mode);
}

inline int open_no_symlink(const char *path, int flags)
{
    return open_no_symlink(path, flags, 0);
}

inline bool write_all(int fd, const char *buf, size_t len)
{
    size_t off = 0;
    while (off < len) {
        ssize_t wrote = ::write(fd, buf + off, len - off);
        if (wrote < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        off += static_cast<size_t>(wrote);
    }
    return true;
}

class fdoutbuf final : public std::streambuf {
public:
    explicit fdoutbuf(int fd) : fd_(fd) {}

protected:
    int_type overflow(int_type ch) override
    {
        if (ch == traits_type::eof()) {
            return traits_type::not_eof(ch);
        }

        char c = static_cast<char>(ch);
        if (!write_all(fd_, &c, 1)) {
            return traits_type::eof();
        }
        return ch;
    }

    std::streamsize xsputn(const char *s, std::streamsize count) override
    {
        if (count <= 0) {
            return 0;
        }
        const size_t len = static_cast<size_t>(count);
        if (!write_all(fd_, s, len)) {
            return 0;
        }
        return count;
    }

    int sync() override { return 0; }

private:
    int fd_;
};

class fdostream final : public std::ostream {
public:
    explicit fdostream(int fd) : std::ostream(nullptr), buf_(fd) { rdbuf(&buf_); }

private:
    fdoutbuf buf_;
};

} // namespace be13

#endif


#ifndef QDEBUGHELPER_H
#define QDEBUGHELPER_H

#include <QDebug>
#include <thread>
#include <sstream>
#include <string>

#ifdef __cpp_lib_filesystem
#include <filesystem>
    namespace fs=std::filesystem;
#else
#include <experimental/filesystem>
    namespace fs = std::experimental::filesystem;
#endif


static inline QDebug operator<<(QDebug qd, const std::thread::id& threadId)
{
    std::stringstream ss;
    ss << threadId;
    qd << ss.str().c_str();
    return qd;
}

static inline QDebug operator<<(QDebug qd, const std::string& s)
{
    qd << s.c_str();
    return qd;
}
static inline QDebug operator<<(QDebug qd, const fs::path& p)
{
    qd << p.string();
    return qd;
}

#endif // QDEBUGHELPER_H

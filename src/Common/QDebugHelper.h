#ifndef QDEBUGHELPER_H
#define QDEBUGHELPER_H

#include <QDebug>
#include <QMetaEnum>
#include <QEvent>

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


QDebug operator<<(QDebug qd, const std::thread::id& threadId);

QDebug operator<<(QDebug qd, const std::string& s);

QDebug operator<<(QDebug qd, const fs::path& p);

QDebug operator<<(QDebug qd, const QEvent * ev);

QString toQString(const QEvent * ev);

#endif // QDEBUGHELPER_H

#ifndef DIRECTORYRUNNER_H
#define DIRECTORYRUNNER_H

#include <vector>
#include <string>

#include "Logger.h"

#ifdef __cpp_lib_filesystem
#include <filesystem>
    namespace fs=std::filesystem;
#else
#include <experimental/filesystem>
    namespace fs = std::experimental::filesystem;
#endif

using std::string;
using std::vector;

class DirectoryRunner
{
protected:
    DirectoryRunner();
public:
    static DirectoryRunner* make(Logger& logger);

    virtual ~DirectoryRunner();

    virtual void run(const fs::path& path) = 0;
    virtual void stop() = 0;

    virtual const vector<string>& getSourceFiles() const = 0;
};

#endif // DIRECTORYRUNNER_H

#include "DirectoryRunner.h"
#include <thread>

#include <QDebug>

#include <deque>
#include <mutex>
#include <optional>
#include <atomic>
#include <cassert>

using std::thread;
using std::deque;
using std::optional;
using std::mutex;
using std::lock_guard;
using std::atomic;
using fs::directory_iterator;

template<class T> T popDeque(deque<T>& deque)
{
    T result = deque.front();
    deque.pop_front();
    return result;
}

DirectoryRunner::DirectoryRunner()
{

}
DirectoryRunner::~DirectoryRunner()
{

}

static inline QDebug& operator<<(QDebug& qd, const string& s)
{
    qd << s.c_str();
    return qd;
}
static inline QDebug& operator<<(QDebug& qd, const fs::path& p)
{
    qd << p.string();
    return qd;
}

class DirectoryRunnerWorkerInterface : public virtual Logger
{
public:
    virtual ~DirectoryRunnerWorkerInterface(){}
    virtual void addFile(const fs::path& path) = 0;
    virtual void addDirectory(const fs::path& path) = 0;
    virtual bool isDone() = 0;
    virtual optional<fs::path> getWork() = 0;
    virtual void doneWork() = 0;
};

class DirectoryRunnerWorker
{
public:
    DirectoryRunnerWorker(DirectoryRunnerWorkerInterface& runner)
        : m_runner(runner)
    {
    }

    void start()
    {
        while (!m_runner.isDone())
        {
            auto pathO = m_runner.getWork();
            if (pathO.has_value())
            {
                processPath(*pathO);
                m_runner.doneWork();
            }
        };
    }

    void processPath(const fs::path& path)
    {
        m_runner.logMessage("   Entering path \"" + path.string() + "\"");
        for (const auto de : directory_iterator(path))
        {
            if (fs::is_directory(de))
                m_runner.addDirectory(de.path());
            else if (fs::is_regular_file(de))
                m_runner.addFile(de.path());
            else
                m_runner.logMessage("   Unknown file type \"" + de.path().string() + "\"");
        }

    }
private:
    DirectoryRunnerWorkerInterface& m_runner;
};

class DirectoryRunnerImpl :
        public DirectoryRunner,
        public virtual DirectoryRunnerWorkerInterface
{
public:
    DirectoryRunnerImpl(Logger& logger) :
        m_logger(logger)
    {
    }
    void run(const fs::path& path) override
    {
        m_foundSourceFiles.clear();
        qDebug() << "DR.run(" << path.string() << "): Number of CPUs:" << thread::hardware_concurrency();
        if (!fs::exists(path))
        {
            qDebug() << " Path \"" << path.string() << "\" not found";
            return;
        }
        qDebug() << "  Current directory" << fs::current_path();
        qDebug() << "  Running directory" << path;

/*        for (auto de : directory_iterator(path))
        {
            const string name = de.path().string();
            m_foundSourceFiles.push_back(name);
            qDebug() << "    " << name;
        }*/

        assert(m_numPathsToProcess==0);
        m_numPathsToProcess = 0;
        assert(m_pathsToRun.empty());
        m_pathsToRun.clear();
        addDirectory(path);
        DirectoryRunnerWorker worker(*this);
        worker.start();

        qDebug() << "  Number of subdirectories:" << m_foundSourceFiles.size();
    }
    void stop() override
    {}

    const vector<string>& getSourceFiles() const override
    {
        return m_foundSourceFiles;
    }

public:
    void addFile(const fs::path& path) override
    {
        lock_guard lock(m_foundSourceFilesMutex);
        m_foundSourceFiles.push_back(path.string());
    }

    void addDirectory(const fs::path& path) override
    {
        lock_guard lock(m_pathsToRunMutex);
        m_pathsToRun.push_back(path);
        m_numPathsToProcess++;
    }

    optional<fs::path> getWork()
    {
        lock_guard lock(m_pathsToRunMutex);
        const bool workLeft = !m_pathsToRun.empty();
        return (workLeft)
            ? optional<fs::path>(popDeque(m_pathsToRun))
            : optional<fs::path>();
    }

    void doneWork() override
    {
        assert (m_numPathsToProcess>0);
        m_numPathsToProcess--;
    }

    bool isDone() override {
        return m_numPathsToProcess==0;
    }

    void logMessage(const string& message) override
    {
        m_logger.logMessage(message);
    }

private:
    Logger& m_logger;

    vector<string> m_foundSourceFiles;
    mutex          m_foundSourceFilesMutex;

    deque<fs::path> m_pathsToRun;
    mutex           m_pathsToRunMutex;

    atomic<int> m_numPathsToProcess = 0;
};

DirectoryRunner* DirectoryRunner::make(Logger& logger)
{
    return new DirectoryRunnerImpl(logger);
}

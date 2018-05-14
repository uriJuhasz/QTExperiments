#include "DirectoryRunner.h"
#include <thread>

#include <QDebug>
#include <QTime>

#include <deque>
#include <mutex>
#include <optional>
#include <atomic>
#include <cassert>
#include <condition_variable>

using std::thread;
using std::deque;
using std::optional;
using std::mutex;
using std::lock_guard;
using std::unique_lock;
using std::atomic;
using std::condition_variable;
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
//        m_runner.logMessage("   Entering path \"" + path.string() + "\"");
        for (const auto de : directory_iterator(path))
        {
            if (fs::is_directory(de))
                m_runner.addDirectory(de.path());
            else if (fs::is_regular_file(de))
                m_runner.addFile(de.path());
            else
                qDebug() << "   Unknown file type \"" << de.path().string() << "\"";
                            //m_runner.logMessage("   Unknown file type \"" + de.path().string() + "\"");
        }

    }
private:
    DirectoryRunnerWorkerInterface& m_runner;
};

class DirectoryRunnerImpl :
        public DirectoryRunner,
        public virtual DirectoryRunnerWorkerInterface
{
signals:

public:
    DirectoryRunnerImpl(DirectoryRunnerListener& listener) :
        listener(listener)
    {
    }
    void start(const fs::path& path) override
    {
        m_foundSourceFiles.clear();
        const int numCPUs = thread::hardware_concurrency();
        qDebug() << "DR.run(" << path.string() << "): Number of CPUs:" << numCPUs;
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

        assert(m_numDirectoriesPathsToProcess==0);
        m_numDirectoriesPathsToProcess = 0;
        assert(m_pathsToRun.empty());
        m_pathsToRun.clear();
        addDirectory(path);
        assert(threads.empty());
        threads.clear();
        assert(workers.empty());
        workers.clear();

        const int numThreads = numCPUs;
        threads.reserve(numThreads);
        workers.reserve(numThreads);

        qDebug() << "  Starting threads";
        QTime timer;
        timer.start();
        for (int i=0; i<numThreads; ++i)
        {
            workers.emplace_back(*this);
            DirectoryRunnerWorker& worker = workers.back();
            threads.emplace_back(thread([&worker](){worker.start();}));
        }

        qDebug() << "  Waiting for threads";
        for (int i=0; i<numThreads; ++i)
            threads[i].join();

        threads.clear();
        workers.clear();

        const qint64 elapsed = timer.elapsed();
        qDebug() << "  Done joining threads - " << elapsed << "ms";

        qDebug() << "  Number of subdirectories:" << m_foundSourceFiles.size();
    }
    void stopInternal()
    {
        stopping = true;
        lock_guard lock(m_pathsToRunMutex);
        {
            m_pathsToRun.clear();
            m_numDirectoriesPathsToProcess = 0;
            for (unsigned int i=0; i<threads.size(); ++i)
                threads[i].join();
            threads.clear();
            workers.clear();
        }
        stopping = false;
    }
    void stop() override
    {
        qDebug() << "  DR: stop called - stopping threads";
        stopInternal();
    }

    void done() override
    {
        qDebug() << "  DR: done called - stopping threads";
        stopInternal();
    }

    const vector<string>& getSourceFiles() override
    {
        lock_guard lock(m_foundSourceFilesMutex);
        return m_foundSourceFiles;
    }

public:
    void addFile(const fs::path& path) override
    {
        if (stopping) return;
        {
            lock_guard lock(m_foundSourceFilesMutex);
            m_foundSourceFiles.push_back(path.string());
            numAddedFilesModulo++;
        }
        if (numAddedFilesModulo>updateGranularity)
        {
            numAddedFilesModulo = 0;
            listener.updateList();
        }
    }

    void addDirectory(const fs::path& path) override
    {
        if (stopping) return;
        unique_lock<mutex> guard(m_pathsToRunMutex);
        m_pathsToRun.push_back(path);
        m_numDirectoriesPathsToProcess++;
        workAvailable.notify_all();
    }

    optional<fs::path> getWork()
    {
        if (stopping)
            return optional<fs::path>();
        unique_lock<mutex> guard(m_pathsToRunMutex);
        while (!stopping && m_numDirectoriesPathsToProcess>0)
        {
            const bool workLeft = !m_pathsToRun.empty();
            if (workLeft)
                return optional<fs::path>(popDeque(m_pathsToRun));
            workAvailable.wait(guard);
        }
        return optional<fs::path>();
    }

    void doneWork() override
    {
        if (stopping)
            return;
        assert (m_numDirectoriesPathsToProcess>0);
        m_numDirectoriesPathsToProcess--;
        if (m_numDirectoriesPathsToProcess==0)
        {
            qDebug() << "  DR: work done";
            workAvailable.notify_all();
//            listener.directoryRunnerDone();
        }

    }

    bool isDone() override {
        return stopping || m_numDirectoriesPathsToProcess==0;
    }

    void logMessage(const string& message) override
    {
        listener.logMessage(message);
    }

private:
    DirectoryRunnerListener& listener;

    vector<string> m_foundSourceFiles;
    mutex          m_foundSourceFilesMutex;

    deque<fs::path> m_pathsToRun;
    mutex           m_pathsToRunMutex;

    atomic<int> m_numDirectoriesPathsToProcess = 0;

    static constexpr int updateGranularity = 10000;
    atomic<int> numAddedFilesModulo = 0;

    vector<thread> threads;
    vector<DirectoryRunnerWorker> workers;

    condition_variable workAvailable;

    bool stopping = false;
};

DirectoryRunner* DirectoryRunner::make(DirectoryRunnerListener& listener)
{
    return new DirectoryRunnerImpl(listener);
}

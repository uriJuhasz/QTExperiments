#include "DirectoryRunner.h"

#include "Common/QDebugHelper.h"

#include <QTime>

#include <thread>
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
        for (const auto& de : directory_iterator(path))
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
        m_listener(listener)
    {
    }

    ~DirectoryRunnerImpl() override
    {
        if (m_collectingThread.joinable())
            m_collectingThread.join();
    }
    void startInternal()
    {
        const unsigned int numCPUs = thread::hardware_concurrency();
        const unsigned int numThreads = numCPUs;
        m_threads.reserve(numThreads);
        m_workers.reserve(numThreads);

        qDebug() << "  [" << std::this_thread::get_id() << "]DR.run: Starting threads";
        QTime timer;
        timer.start();
        for (unsigned int i=0; i<numThreads; ++i)
        {
            m_workers.emplace_back(*this);
            DirectoryRunnerWorker& worker = m_workers.back();
            m_threads.emplace_back(thread([&worker](){worker.start();}));
        }

        qDebug() << "  [" << std::this_thread::get_id() << "]DR.run: Waiting for threads";
        for (unsigned int i=0; i<numThreads; ++i)
            m_threads[i].join();

        m_threads.clear();
        m_workers.clear();

        const qint64 elapsed = timer.elapsed();
        qDebug() << "  [" << std::this_thread::get_id() << "]DR.run: Done joining threads - " << elapsed << "ms";

        qDebug() << "   Number of subdirectories:" << m_foundSourceFiles.size();
    }
    void start(const fs::path& path) override
    {
        if (m_collectingThread.joinable())
        {
            if (m_numDirectoriesPathsToProcess==0)
            {
                m_collectingThread.join();
            }
            else
            {
                qDebug() << "DirectoryRunner called while running - call ignored";
                return;
            }
        }
        const unsigned int numCPUs = thread::hardware_concurrency();
        qDebug() << " [" << std::this_thread::get_id() << "]DR.run(" << path.string() << "): Number of CPUs:" << numCPUs;
        if (!fs::exists(path))
        {
            qDebug() << "  Path \"" << path.string() << "\" not found";
            return;
        }
        qDebug() << "   Current directory" << fs::current_path();
        qDebug() << "   Running directory" << path;
        assert(m_numDirectoriesPathsToProcess==0);
        m_numDirectoriesPathsToProcess = 0;
        assert(m_pathsToRun.empty());
        m_pathsToRun.clear();
        addDirectory(path);
        assert(m_threads.empty());
        m_threads.clear();
        assert(m_workers.empty());
        m_workers.clear();

        m_foundSourceFiles.clear();

        m_collectingThread = thread([this](){startInternal();});
    }
    void stopInternal()
    {
        m_stopping = true;
        lock_guard<mutex> lock(m_pathsToRunMutex);
        {
            m_pathsToRun.clear();
            m_numDirectoriesPathsToProcess = 0;
            for (unsigned int i=0; i<m_threads.size(); ++i)
                m_threads[i].join();
            m_threads.clear();
            m_workers.clear();
        }
        m_stopping = false;
    }
    void stop() override
    {
        qDebug() << "  [" << std::this_thread::get_id() << "]DR: stop called - stopping threads";
        stopInternal();
    }

    void done() override
    {
        qDebug() << "  [" << std::this_thread::get_id() << "]DR: done called - stopping threads";
        stopInternal();
    }

    const vector<string> getSourceFiles() override
    {
        vector<string> result;
        {
            lock_guard lock(m_foundSourceFilesMutex);
            result = m_foundSourceFiles;
        }
        return result;
    }

    void logMessageInternal(const Logger::LogLevel, const std::string&) override
    {

    }
public:
    void addFile(const fs::path& path) override
    {
        if (m_stopping) return;
        {
            lock_guard lock(m_foundSourceFilesMutex);
            m_foundSourceFiles.push_back(path.string());
            m_numAddedFilesModulo++;
        }
        if (m_numAddedFilesModulo>s_updateGranularity)
        {
            m_numAddedFilesModulo = 0;
            qDebug() << "  [" << std::this_thread::get_id() << "]DR.addFile().mod";
            m_listener.updateList();
        }
    }

    void addDirectory(const fs::path& path) override
    {
        if (m_stopping) return;
//        logMessage("DR: adding directory(" + path.string() + ")");

        unique_lock<mutex> guard(m_pathsToRunMutex);
        m_pathsToRun.push_back(path);
        m_numDirectoriesPathsToProcess++;
        m_workAvailable.notify_all();
    }

    optional<fs::path> getWork() override
    {
        if (m_stopping)
            return optional<fs::path>();
        unique_lock<mutex> guard(m_pathsToRunMutex);
        while (!m_stopping && m_numDirectoriesPathsToProcess>0)
        {
            const bool workLeft = !m_pathsToRun.empty();
            if (workLeft)
                return optional<fs::path>(popDeque(m_pathsToRun));
            m_workAvailable.wait(guard);
        }
        return optional<fs::path>();
    }

    void doneWork() override
    {
        if (m_stopping)
            return;
        assert (m_numDirectoriesPathsToProcess>0);
        m_numDirectoriesPathsToProcess--;
        if (m_numDirectoriesPathsToProcess==0)
        {
            qDebug() << "  [" << std::this_thread::get_id() << "]DR: work done";
            m_workAvailable.notify_all();
            m_listener.directoryRunnerDone();
        }

    }

    bool isDone() override {
        return m_stopping || m_numDirectoriesPathsToProcess==0;
    }

    void logMessage(const string& message)
    {
       qDebug() << message.c_str();// m_listener.logMessage(LogLevel::Info,message);
    }

private:
    DirectoryRunnerImpl() = delete;
    DirectoryRunnerImpl(const DirectoryRunnerImpl&) = delete;
    DirectoryRunnerImpl& operator=(const DirectoryRunnerImpl&) = delete;

    DirectoryRunnerListener& m_listener;

    vector<string> m_foundSourceFiles;
    mutex          m_foundSourceFilesMutex;

    deque<fs::path> m_pathsToRun;
    mutex           m_pathsToRunMutex;

    atomic<int> m_numDirectoriesPathsToProcess{0};

    static constexpr int s_updateGranularity = 10000;
    atomic<int> m_numAddedFilesModulo{0};

    vector<thread> m_threads;
    vector<DirectoryRunnerWorker> m_workers;

    condition_variable m_workAvailable;

    bool m_stopping = false;

    std::thread m_collectingThread;
};

DirectoryRunner* DirectoryRunner::make(DirectoryRunnerListener& listener)
{
    return new DirectoryRunnerImpl(listener);
}

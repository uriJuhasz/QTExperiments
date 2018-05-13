#include "DirectoryRunner.h"
#include <thread>

#include <QDebug>

using std::thread;
using fs::directory_iterator;

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

class DirectoryRunnerImpl : public DirectoryRunner
{
public:
    void run(const fs::path& path) override
    {
        m_foundSourceFiles.clear();
        qDebug() << "DR.run(" << path.string() << "): Number of CPUs:" << thread::hardware_concurrency();
        if (!fs::exists(path))
        {
            qDebug() << " Path \"" << path.string() << "\" not found";
            return;
        }
        qDebug() << "  Current directory" << fs::current_path().string();
        for (auto de : directory_iterator(path))
        {
            const string name = de.path().string();
            m_foundSourceFiles.push_back(name);
            qDebug() << "    " << name;
        }
        qDebug() << "  Number of subdirectories:" << m_foundSourceFiles.size();
    }
    void stop() override
    {}

    const vector<string>& getSourceFiles() const override
    {
        return m_foundSourceFiles;
    }
private:
    vector<string> m_foundSourceFiles;
};

DirectoryRunner* DirectoryRunner::make()
{
    return new DirectoryRunnerImpl();
}

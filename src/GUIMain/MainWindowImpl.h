#ifndef MAINWINDOWIMPL_H
#define MAINWINDOWIMPL_H

#include "GUIMain/MainWindow.h"
#include "Logger/Logger.h"
#include "DirectoryRunner/DirectoryRunner.h"

#include <QStringListModel>

#include <memory>
#include <mutex>
#include <string>


using std::mutex;
using std::unique_ptr;
using std::string;

class MainWindowImpl :
        public MainWindow,
        public virtual Logger,
        public virtual DirectoryRunnerListener
{
    Q_OBJECT

private:
    friend class MainWindow;
    MainWindowImpl(QWidget* parent);

    const string& getDirectoryName() const;
    void setDirectory(const string& directory, const bool updateWidget=true);
    void startDirectoryRunner();
    void stopDirectoryRunner();

    void logMessageInternal(const LogLevel level, const string& message) override;
    void updateList() override;

    void directoryRunnerDone() override;

    bool event(QEvent *event) override;

public slots:
    void updateListSignalHandler();

    void on_selectDirectoryButton_clicked() override;

    void on_directoryNameWidget_editingFinished() override;

    void on_directoryNameWidget_textChanged(const QString &arg1) override;

    void on_startAnalysisButton_clicked() override;

signals:
    void updateListSignal();

private:
    string m_dirName;
    QStringListModel* const m_logModel;

    mutex  m_logMutex;

    unique_ptr<DirectoryRunner> m_directoryRunner;
};

#endif // MAINWINDOWIMPL_H

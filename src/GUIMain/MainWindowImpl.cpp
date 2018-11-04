#include "MainWindowImpl.h"
#include "ui_MainWindow.h"

#include <QFileDialog>
#include <QTime>
#include <QDebug>

#include <optional>

using std::make_unique;
using std::lock_guard;
using std::optional;

MainWindowImpl::MainWindowImpl(QWidget* parent) :
    MainWindow(parent),
    m_logModel(new QStringListModel())
{
    ui->logWindow->setModel(m_logModel);
    qRegisterMetaType<QVector<int> >("QVector<int>");
//    QObject::connect(this,updateListSignal,this,updateListSignalHandler);
    ui->directoryNameWidget->setText(QString::fromStdString("C:/Code/llvm-7/src"));
}

const string& MainWindowImpl::getDirectoryName() const{ return m_dirName; }
void MainWindowImpl::setDirectory(const string& directory, const bool updateWidget)
{
    m_dirName = directory;
    if (updateWidget)
        ui->directoryNameWidget->setText(QString::fromStdString(m_dirName));
}

void MainWindowImpl::startDirectoryRunner()
{
    auto p = fs::path(m_dirName);
    if (!fs::exists(p))
        return;

    if (!m_directoryRunner)
        m_directoryRunner = unique_ptr<DirectoryRunner>(DirectoryRunner::make(*this));

    m_directoryRunner->start(p);

    updateListSignalHandler();
}
void MainWindowImpl::stopDirectoryRunner()
{
    m_directoryRunner->stop();
}

void MainWindowImpl::logMessageInternal(const LogLevel level, const string& message)
{
    lock_guard lock(m_logMutex);
    const int row = m_logModel->rowCount();
    if (m_logModel->insertRow(row))
    {
        m_logModel->setData(m_logModel->index(row, 0), QString::fromStdString(message));
        ui->logWindow->scrollToBottom();
    }
}

void MainWindowImpl::updateList()
{
    updateListSignal();
}

class QEventDRDone : public QEvent
{
public:
    QEventDRDone() : QEvent(staticType()) {}
   static QEvent::Type staticType() {
      static int type = QEvent::registerEventType();
      return static_cast<QEvent::Type>(type);
   }
};

bool MainWindowImpl::event(QEvent *e)
{
    if (e && e->type()==QEventDRDone::staticType())
    {
//        m_directoryRunner->done();
//        updateListSignalHandler();
        return true;
    }else
        return QMainWindow::event(e);
}
void MainWindowImpl::directoryRunnerDone()
{
    QCoreApplication::postEvent(this,new QEventDRDone());
}

void MainWindowImpl::updateListSignalHandler()
{
    qDebug() << " +MW.updateList";
    QTime timer;
    timer.start();

    const vector<string> paths = m_directoryRunner->getSourceFiles();

    QStringList pathNames;
    for (auto p : paths)
        pathNames << QString::fromStdString(p);

    qDebug() << "  MW.updateList.1 - " << timer.elapsed() << "ms";
    QStringListModel* pathListModel = new QStringListModel(pathNames, ui->pathListView);

    qDebug() << "  MW.updateList.2 - " << timer.elapsed() << "ms";
    ui->pathListView->setModel(pathListModel);

    qDebug() << "  MW.updateList.3 - " << timer.elapsed() << "ms";
//    ui->pathListView->scrollToBottom();

    const qint64 elapsed = timer.elapsed();
    qDebug() << " -MW.updateList - " << elapsed << "ms";
}

void MainWindowImpl::on_selectDirectoryButton_clicked()
{
    const auto name = QFileDialog::getExistingDirectory(this,QString(),QString::fromStdString(getDirectoryName())).toStdString();
    setDirectory(name);
}

void MainWindowImpl::on_directoryNameWidget_editingFinished()
{
    setDirectory(ui->directoryNameWidget->text().toStdString(),false);
}

void MainWindowImpl::on_directoryNameWidget_textChanged(const QString &arg1)
{
    setDirectory(arg1.toStdString());
}

void MainWindowImpl::on_startAnalysisButton_clicked()
{
    startDirectoryRunner();
}

unique_ptr<MainWindow> MainWindow::make(QWidget* parent)
{
    return unique_ptr<MainWindow>(new MainWindowImpl(parent));
}


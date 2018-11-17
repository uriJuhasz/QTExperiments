#include "MainWindowImpl.h"
#include "ui_MainWindow.h"

#include "Common/QDebugHelper.h"

#include <QFileDialog>
#include <QTime>
#include <QElapsedTimer>
#include <QEvent>
#include <QMetaEnum>
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
    qDebug() << "[" << std::this_thread::get_id() << "]+MW.startDirectoryRunner()";

    const auto p = fs::path(m_dirName);
    if (fs::exists(p))
    {
        if (!m_directoryRunner)
            m_directoryRunner = unique_ptr<DirectoryRunner>(DirectoryRunner::make(*this));

        this->ui->startAnalysisButton->setEnabled(false);
        m_directoryRunner->start(p);
        updateListSignalHandler();
    }
    qDebug() << "[" << std::this_thread::get_id() << "]-MW.startDirectoryRunner()";
}
void MainWindowImpl::stopDirectoryRunner()
{
    m_directoryRunner->stop();
}

void MainWindowImpl::logMessageInternal(const LogLevel /*level*/, const string& message)
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
    QElapsedTimer timer; timer.start();
    qDebug() << "+[" << std::this_thread::get_id() << "]MW.updateList()";
    updateListSignal();
    qDebug() << "-[" << std::this_thread::get_id() << "]MW.updateList() - " << timer.elapsed() << "ms";
}

QDebug operator<<(QDebug str, const QEvent * ev) {
    static int eventEnumIndex = QEvent::staticMetaObject.indexOfEnumerator("Type");
    str << "QEvent";
    if (ev)
    {
        const auto type = ev->type();
        const QString name = QEvent::staticMetaObject.enumerator(eventEnumIndex).valueToKey(type);
        if (!name.isEmpty()) str << name; else str << type;
    }
    else
    {
      str << (void*)ev;
    }
    return str.maybeSpace();
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

static unsigned int qEventND = 0;

static std::string evndstr()
{
    return std::string(qEventND,' ');
}
bool MainWindowImpl::event(QEvent *e)
{
    QEvent ev2 = e ? *e : QEvent(QEvent::None);
    QElapsedTimer timer; timer.start();
    qDebug().nospace().noquote() << evndstr() << "[" << std::this_thread::get_id() << "]+MW.event(" << e << ")";
    qEventND++;

    bool retVal = true;
    if (e && e->type()==QEventDRDone::staticType())
    {
        qDebug() << evndstr() << "[" << std::this_thread::get_id() << "]+MW.updateListEvent()";

        this->ui->startAnalysisButton->setEnabled(true);
//        m_directoryRunner->done();
        updateListSignalHandler();
        retVal = true;
        qDebug() << evndstr() << "[" << std::this_thread::get_id() << "]-MW.updateListEvent()";
    }else
        retVal = QMainWindow::event(e);

    qEventND--;
    qDebug().nospace().noquote() << evndstr() << "[" << std::this_thread::get_id() << "]-MW.event(" << e << ") - " << timer.elapsed() << "ms";
    return retVal;
}
void MainWindowImpl::directoryRunnerDone()
{
    QCoreApplication::postEvent(this,new QEventDRDone());
}

void MainWindowImpl::updateListSignalHandler()
{
    qDebug() << "    [" << std::this_thread::get_id() << "]+MW.updateListSignal";
    QElapsedTimer timer; timer.start();

    {
        const vector<string> paths = m_directoryRunner->getSourceFiles();
        qDebug() << "      [" << std::this_thread::get_id() << "]MW.updateListSignal(num=" << paths.size() << ").0 - " << timer.elapsed() << "ms";
        {
            QStringList pathNames;
            for (auto p : paths)
                pathNames << QString::fromStdString(p);

            qDebug() << "      [" << std::this_thread::get_id() << "]MW.updateListSignal.1 - " << timer.elapsed() << "ms";
            QStringListModel* pathListModel = new QStringListModel(pathNames, ui->pathListView);

            qDebug() << "      [" << std::this_thread::get_id() << "]MW.updateListSignal.2 - " << timer.elapsed() << "ms";
            ui->pathListView->setModel(pathListModel);

            qDebug() << "      [" << std::this_thread::get_id() << "]MW.updateListSignal.3 - " << timer.elapsed() << "ms";
        //    ui->pathListView->scrollToBottom();
        }
        qDebug() << "      [" << std::this_thread::get_id() << "]MW.updateListSignal.4 - " << timer.elapsed() << "ms";
    }

    const qint64 elapsed = timer.elapsed();
    qDebug() << "     [" << std::this_thread::get_id() << "]-MW.updateListSignal - " << elapsed << "ms";
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
    qDebug() << " +MW.on_startAnalysisButton_clicked";
    startDirectoryRunner();
    qDebug() << " -MW.on_startAnalysisButton_clicked";
}

unique_ptr<MainWindow> MainWindow::make(QWidget* parent)
{
    return unique_ptr<MainWindow>(new MainWindowImpl(parent));
}


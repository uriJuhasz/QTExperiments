#include "MainWindow.h"
#include "ui_mainwindow.h"

#include "Logger.h"

#include <QFileDialog>
#include <QStringListModel>

#include "DirectoryRunner.h"

#include <memory>
#include <mutex>

using std::unique_ptr;
using std::make_unique;
using std::mutex;
using std::lock_guard;

class MainWindowPrivate :
        public virtual Logger
{
public:
    MainWindowPrivate(MainWindow& mainWindow) :
        m_mainWindow(mainWindow),
        m_logModel(new QStringListModel())
    {
    }

    const string& getDirectoryName() const{ return m_dirName; }
    void setDirectory(const string& directory, const bool updateWidget=true)
    {
        m_dirName = directory;
        if (updateWidget)
            m_mainWindow.ui->directoryNameWidget->setText(QString::fromStdString(m_dirName));
    }

    void startDirectoryRunner()
    {
        auto p = fs::path(m_dirName);
        if (!fs::exists(p))
            return;

        if (!m_directoryRunner)
            m_directoryRunner = unique_ptr<DirectoryRunner>(DirectoryRunner::make(*this));

        m_directoryRunner->run(p);

        const vector<string>& paths = m_directoryRunner->getSourceFiles();

        QStringList pathNames;
        for (auto p : paths)
            pathNames << QString::fromStdString(p);

        QStringListModel* pathListModel = new QStringListModel(pathNames, m_mainWindow.ui->pathListView);

        m_mainWindow.ui->pathListView->setModel(pathListModel);
    }
    void stopDirectoryRunner();

    void logMessage(const string& message) override
    {
        lock_guard lock(m_logMutex);
        const int row = m_logModel->rowCount();
        if (m_logModel->insertRow(row))
            m_logModel->setData(m_logModel->index(row, 0), QString::fromStdString(message));
    }

private:
    friend class MainWindow;

    MainWindow& m_mainWindow;

    string m_dirName;
    QStringListModel* const m_logModel;

    mutex  m_logMutex;

    unique_ptr<DirectoryRunner> m_directoryRunner;
};


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_private(new MainWindowPrivate(*this))
{
    ui->setupUi(this);
    ui->pathListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->logWindow->setModel(m_private->m_logModel);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_private;
}

void MainWindow::on_selectDirectoryButton_clicked()
{
    string name = QFileDialog::getExistingDirectory(this,QString(),QString::fromStdString(m_private->getDirectoryName())).toStdString();
    m_private->setDirectory(name);
}

void MainWindow::on_directoryNameWidget_editingFinished()
{
    m_private->setDirectory(ui->directoryNameWidget->text().toStdString(),false);
}

void MainWindow::on_directoryNameWidget_textChanged(const QString &arg1)
{
    m_private->setDirectory(arg1.toStdString());
}

void MainWindow::on_startAnalysisButton_clicked()
{
    m_private->startDirectoryRunner();
}


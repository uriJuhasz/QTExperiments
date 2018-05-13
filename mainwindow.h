#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <string>
#include <optional>

using std::string;
using std::optional;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_selectDirectoryButton_clicked();

    void on_directoryNameWidget_editingFinished();

    void on_directoryNameWidget_textChanged(const QString &arg1);

    void on_startAnalysisButton_clicked();

private:
    Ui::MainWindow *ui;

    friend class MainWindowPrivate;
    class MainWindowPrivate* m_private;
};

#endif // MAINWINDOW_H

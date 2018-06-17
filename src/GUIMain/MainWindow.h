#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>

using std::unique_ptr;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

protected:
    explicit MainWindow(QWidget *parent);

public:
    virtual ~MainWindow();
    static unique_ptr<MainWindow> make(QWidget* parent=nullptr);

public slots:
    virtual void on_selectDirectoryButton_clicked() = 0;

    virtual void on_directoryNameWidget_editingFinished() = 0;

    virtual void on_directoryNameWidget_textChanged(const QString &arg1) = 0;

    virtual void on_startAnalysisButton_clicked() = 0;

protected:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H

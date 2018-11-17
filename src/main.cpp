#include "GUIMain/MainWindow.h"
#include "Logger/logger.h"
#include "Common/QDebugHelper.h"

#include <QApplication>
#include <QTextStream>
#include <QDebug>
#include <QElapsedTimer>
#include <QEvent>

#include <memory>

using std::unique_ptr;

//int main2();


class TestApp : public QApplication
{
public:
    using QApplication::QApplication;

    static int nd;
    static QString eventString(const bool start,const QObject *receiver, const QEvent *event)
    {
        QString retVal;
        QTextStream ss(&retVal);
        ss << (start ? "+" : "-");
        for (int i=0; i<nd; ++i)
            ss << " ";
        ss << "Event(" << receiver << " : " << event << ")";
        return retVal;
    }
    static QString ndstr()
    {
        QString retVal;
        QTextStream ss(&retVal);
        for (int i=0; i<nd; ++i)
            ss << " ";
        return retVal;
    }
    bool notify(QObject *receiver, QEvent *event) override
    {
        qDebug().noquote().nospace() << ndstr() << "+Event(" << receiver << " : " << event << ")";
        nd++;
        QElapsedTimer timer; timer.start();
        const auto retVal = QApplication::notify(receiver,event);
        nd--;
        qDebug().noquote().nospace() << ndstr() << "-Event(" << event << ") - " << timer.elapsed() << "ms";
        return retVal;
    }
};
int TestApp::nd = 0;

//static void qtEventFilter(c)

int main(int argc, char *argv[])
{
    unique_ptr<Logger> logger( Logger::make() );

    //main2();
    TestApp app(argc, argv);
//    QObject::installEventFilter();
    auto w = MainWindow::make();
    w->show();

    return app.exec();
}


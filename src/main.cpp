#include "GUIMain/MainWindow.h"
#include "Logger/logger.h"
#include <QApplication>

#include <memory>
using std::unique_ptr;

//int main2();

int main(int argc, char *argv[])
{
    unique_ptr<Logger> logger( Logger::make() );

    //main2();
    QApplication a(argc, argv);
    auto w = MainWindow::make();
    w->show();

    return a.exec();
}

#include <iostream>
using std::cout;
using std::endl;

void foo(const std::string &) {}
int main1()
{
    //foo(false);
    return 0;
}
/*
class A { public: int a; inline virtual ~A(){} };
class B : public A { public: int b; inline virtual ~B(){} };
int main2()
{
    B * bp = new B[10];
    A * ap = bp;
    B ba[10] = bp;
    A aa[10] = bp;

    cout << "sizeof(A)=" << sizeof(A) << endl;
    cout << "sizeof(B)=" << sizeof(B) << endl;
    cout << "ap[5]  =" << &ap[5]   << endl;
    cout << "ap[5].a=" << &ap[5].a << endl;
    cout << "bp[5]  =" << &bp[5]   << endl;
    cout << "bp[5].a=" << &bp[5].a << endl;
    cout << "ap[6]  =" << &ap[6]   << endl;
    cout << "bp[6]  =" << &bp[6]   << endl;
    ap[5].a = 1;
    return 0;
}
*/


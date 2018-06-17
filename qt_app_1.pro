#-------------------------------------------------
#
# Project created by QtCreator 2018-05-05T14:35:03
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qt_app_1
TEMPLATE = app

#CONFIG += c++17
#QMAKE_CXXFLAGS += /std:c++17


gcc:QMAKE_CXXFLAGS += -std=c++17
gcc:QMAKE_CXXFLAGS += -Wall -Wextra -pedantic
msvc:QMAKE_CXXFLAGS += /W3 -std:c++17
#gcc:QMAKE_LFLAGS += -lstdc++fs

CONFIG += c++17

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += $$PWD/src/

SOURCES += \
        src/main.cpp \
    src/DirectoryRunner/DirectoryRunner.cpp \
    src/GUIMain/MainWindow.cpp \
    src/GUIMain/MainWindowImpl.cpp \
    src/Logger/Logger.cpp

HEADERS += \
    src/DirectoryRunner/DirectoryRunner.h \
    src/GUIMain/MainWindow.h \
    src/GUIMain/MainWindowImpl.h \
    src/Logger/Logger.h \


FORMS += \
    src/GUIMain/MainWindow.ui

gcc: LIBS += -lstdc++fs

#win32:     !win32-g++: PRE_TARGETDEPS += $$PWD/../../../msys64/mingw64/lib/gcc/x86_64-w64-mingw32/7.3.0/stdc++fs.lib
#else:  unix|win32-g++: PRE_TARGETDEPS += $$PWD/../../../msys64/mingw64/lib/gcc/x86_64-w64-mingw32/7.3.0/libstdc++fs.a

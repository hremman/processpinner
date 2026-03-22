QT       += core gui concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
CONFIG += resources_big
#CONFIG += console

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/aboutdialog.cpp \
    src/loghandler.cpp \
    src/configurationelementwidget.cpp \
    src/editconfigdialog.cpp \
    src/executabledto.cpp \
    src/globals.cpp \
    src/helpwindow.cpp \
    src/main.cpp \
    src/mainwindow.cpp \
    src/processtab.cpp \
    src/projectparser.cpp \
    src/tools/executors/binexecutor.cpp \
    src/tools/executors/pythonexecutor.cpp \
    src/tools/executors/javaexecutor.cpp \
    src/tools/parsers/gradle.cpp \
    src/tools.cpp

HEADERS += \
    src/aboutdialog.h \
    src/loghandler.h \
    src/configurationelementwidget.h \
    src/editconfigdialog.h \
    src/executabledto.h \
    src/globals.hpp \
    src/helpwindow.h \
    src/mainwindow.h \
    src/tools/executors/binexecutor.h \
    src/tools/executors/pythonexecutor.h \
    src/tools/executors/javaexecutor.h \
    src/tools/parsers/parsers.hpp \
    src/processtab.h \
    src/projectparser.hpp \
    src/tools.h

FORMS += \
    src/forms/aboutdialog.ui \
    src/forms/configurationelementwidget.ui \
    src/forms/editconfigdialog.ui \
    src/forms/helpwindow.ui \
    src/forms/mainwindow.ui \
    src/forms/processtab.ui

TRANSLATIONS += \
    resouces/ProcessPinner_ru_RU.ts
CONFIG += lrelease
CONFIG += embed_translations

RESOURCES += \
    resouces/resources.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

#linux
#cqtdeployer -bin myApp -qmake /path/to/qt6/bin/qmake -targetDir ./dist
#https://github.com/QuasarApp/CQtDeployer

#windeployqt ProcessLauncher.exe
#cqtdeployer -bin myApp.exe -qmake /path/to/windows/qt6/bin/qmake.exe
win32:RC_ICONS = resouces/logo-512.ico

macx {
    #macdeployqt ProcessLauncher.app для сборки бандла
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 14.0
    ICON = resouces/logo-512.icns
}

VERSION = "0.1.1"
DEFINES += APP_VERSION=\"\\\"$${VERSION}\\\"\"

DISTFILES += \
    all.md \
    deployWin.bat \
    themes.md \
    .gitignore
    

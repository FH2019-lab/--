QT       += core gui charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QT += multimedia multimediawidgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    addteamdialog.cpp \
    barchartdialog.cpp \
    calibrationdialog.cpp \
    displayLabel.cpp \
    main.cpp \
    mainwindow.cpp \
    mergedialog.cpp

HEADERS += \
    addteamdialog.h \
    barchartdialog.h \
    calibrationdialog.h \
    displayLabel.h \
    mainwindow.h \
    mergedialog.h

FORMS += \
    addteamdialog.ui \
    barchartdialog.ui \
    calibrationdialog.ui \
    mainwindow.ui \
    mergedialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    pic.qrc

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../../App/opencv/build/x64/vc15/lib/ -lopencv_world454
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../../App/opencv/build/x64/vc15/lib/ -lopencv_world454d
else:unix: LIBS += -L$$PWD/../../../../../App/opencv/build/x64/vc15/lib/ -lopencv_world454

INCLUDEPATH += $$PWD/../../../../../App/opencv/build/include
DEPENDPATH += $$PWD/../../../../../App/opencv/build/include

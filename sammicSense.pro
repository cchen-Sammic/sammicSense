#-------------------------------------------------
#
# Project created by QtCreator 2014-10-01T09:12:24
#
#-------------------------------------------------

QT       += core gui \
           widgets printsupport \

#greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport
contains(QT_VERSION, ^5\\..*\\..*): QT += widgets

TARGET = sammicSense
TEMPLATE = app

include(src/qextserialport.pri)

SOURCES += main.cpp\
        dialog.cpp \
    qcustomplot.cpp \
    hled.cpp \

HEADERS  += dialog.h \
    qcustomplot.h \
    hled.h \


FORMS    += dialog.ui

OTHER_FILES +=

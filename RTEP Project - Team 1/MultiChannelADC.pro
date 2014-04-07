# Qt project file - qmake uses his to generate a Makefile

QT       += core gui

CONFIG          += qt warn_on debug

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = MultiChannelADC

LIBS += -lfftw3 -lqwt -lm -lbcm2835 -lrt -std=c99

HEADERS += window.h adcreader.h gz_clk.h gpio-sysfs.h

SOURCES += main.cpp window.cpp adcreader.cpp gpio-sysfs.cpp gz_clk.cpp

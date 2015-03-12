#-------------------------------------------------
#
# Project created by QtCreator 2013-06-05T11:29:02
#
#-------------------------------------------------

include(../defaults.pri)

QT       += core gui
QT       += opengl
TARGET = vidman
TEMPLATE = lib
CONFIG += staticlib
CONFIG += console
CONFIG -= app_bundle


SOURCES += mainwindow.cpp \
    videodialog.cpp \
    videofilewriter.cpp \
    videocompressorthread.cpp \
    stoppablethread.cpp \
    settings.cpp \
    filewriter.cpp \
    cycdatabuffer.cpp \
    camerathread.cpp \
    event.cpp \
    eventcontainer.cpp \
    eventreader.cpp \
    imagecontainer.cpp \
    linenumberarea.cpp \
    textedit.cpp \
    highlighter.cpp \
    glvideowidget.cpp \
    timerwithpause.cpp \
    motiondetector.cpp \
    videoevent.cpp \
    outputdevice.cpp \
    glthread.cpp \
    camera.cpp \
    logfile.cpp

HEADERS  += mainwindow.h \
    videodialog.h \
    videofilewriter.h \
    videocompressorthread.h \
    stoppablethread.h \
    settings.h \
    filewriter.h \
    cycdatabuffer.h \
    config.h \
    common.h \
    camerathread.h \
    event.h \
    eventcontainer.h \
    eventreader.h \
    imagecontainer.h \
    linenumberarea.h \
    textedit.h \
    highlighter.h \
    glvideowidget.h \
    timerwithpause.h \
    motiondetector.h \
    videoevent.h \
    outputdevice.h \
    glthread.h \
    camera.h \
    logfile.h

FORMS    += mainwindow.ui \
    videodialog.ui

INCLUDEPATH += /usr/include/opencv
LIBS += -L/usr/share/lib -ldc1394
LIBS += -lopencv_core
LIBS += -lopencv_imgproc
LIBS += -lopencv_highgui
LIBS += -lopencv_ml
LIBS += -lopencv_video
LIBS += -lopencv_features2d
LIBS += -lopencv_calib3d
LIBS += -lopencv_objdetect
LIBS += -lopencv_contrib
LIBS += -lopencv_legacy
LIBS += -lopencv_flann
LIBS += -ljpeg

QMAKE_CXXFLAGS += -std=c++11

# remove possible other optimization flags
QMAKE_CXXFLAGS_RELEASE -= -O
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O2

# add the desired -O3 if not present
QMAKE_CXXFLAGS_RELEASE *= -O3

Release:DESTDIR = release
Release:OBJECTS_DIR = release/.obj
Release:MOC_DIR = release/.moc
Release:RCC_DIR = release/.rcc
Release:UI_DIR = release/.ui

Debug:DESTDIR = debug
Debug:OBJECTS_DIR = debug/.obj
Debug:MOC_DIR = debug/.moc
Debug:RCC_DIR = debug/.rcc
Debug:UI_DIR = debug/.ui

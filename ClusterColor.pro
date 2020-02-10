QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

VERSION = 1.0.0.0

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ColorSpace.cpp \
    Comparison.cpp \
    Conversion.cpp \
    aboutdialog.cpp \
    automaticgroupgeneration.cpp \
    clickableimage.cpp \
    clickableswatch.cpp \
    exponentialslider.cpp \
    imageviewer.cpp \
    linearslider.cpp \
    main.cpp \
    clustercolormain.cpp \
    palette.cpp \
    palettecolor.cpp \
    palettegroup.cpp \
    palettegroupview.cpp \
    preferences.cpp

HEADERS += \
    ColorSpace.h \
    Comparison.h \
    Conversion.h \
    Utils.h \
    aboutdialog.h \
    automaticgroupgeneration.h \
    clickableimage.h \
    clickableswatch.h \
    clustercolormain.h \
    exponentialslider.h \
    imageviewer.h \
    linearslider.h \
    palette.h \
    palettecolor.h \
    palettegroup.h \
    palettegroupview.h \
    preferences.h

FORMS += \
    aboutdialog.ui \
    automaticgroupgeneration.ui \
    clustercolormain.ui \
    exponentialslider.ui \
    imageviewer.ui \
    linearslider.ui \
    palettegroupview.ui \
    preferences.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES +=

RESOURCES += \
    Resources.qrc

QT += core gui widgets network multimedia multimediawidgets
LIBS += -lrga
CONFIG += c++11 link_pkgconfig
# GStreamer 依赖
PKGCONFIG += gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0 opencv4

# OpenCV 头文件路径
INCLUDEPATH += /usr/include/opencv4

# OpenCV 链接库 (新增了 imgcodecs)
PKGCONFIG += opencv4 gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0

SOURCES += \
    AVRecorder.cpp \
    RgaConverter.cpp \
    SrtClient.cpp \
    SrtServer.cpp \
    WeatherClockWidget.cpp \
    main.cpp \
    mainwidget.cpp \
    temperaturechartwidget.cpp \
    videowidget.cpp \
    FaceProcessor.cpp \
    audioanalyzer.cpp \
    audiowidget.cpp \
    led_control.cpp

HEADERS += \
    AVRecorder.h \
    RgaConverter.h \
    SrtClient.h \
    SrtServer.h \
    WeatherClockWidget.h \
    mainwidget.h \
    temperaturechartwidget.h \
    videowidget.h \
    FaceProcessor.h \
    audioanalyzer.h \
    audiowidget.h \
    led_control.h

FORMS += \
    mainwidget.ui

RESOURCES += \
    images.qrc

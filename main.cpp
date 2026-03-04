#include "mainwidget.h"
#include <QApplication>
#include <opencv2/core.hpp> // 注册类型需要

int main(int argc, char *argv[])
{
    // 【关键】禁用 X11 共享内存，防止 RK3576 绘图崩溃
    qputenv("QT_X11_NO_MITSHM", "1");
    // 【关键】清除 GStreamer 视频接收器，防止它抢占窗口
    qunsetenv("QT_GSTREAMER_WIDGET_VIDEOSINK");

    QApplication a(argc, argv);

    // 注册 OpenCV 类型以便跨线程信号槽传递
    qRegisterMetaType<cv::Mat>("cv::Mat");

    MainWidget w;
    w.show();

    return a.exec();
}

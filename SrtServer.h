// SrtServer.h
#ifndef SRTSERVER_H
#define SRTSERVER_H

#include <QObject>
#include <gst/gst.h>
#include <opencv2/opencv.hpp>

class SrtServer : public QObject {
    Q_OBJECT
public:
    explicit SrtServer(QObject *parent = nullptr);
    ~SrtServer();

public slots:
    void startServer();
    void stopServer();
    void pushFrame(const cv::Mat &frame);

private:
    GstElement *pipeline = nullptr;
    GstElement *appsrc = nullptr;
    bool isRunning = false;
};

#endif

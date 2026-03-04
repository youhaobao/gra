#ifndef SRTCLIENT_H
#define SRTCLIENT_H

#include <QObject>
#include <gst/gst.h>
#include <opencv2/opencv.hpp>

class SrtClient : public QObject {
    Q_OBJECT
public:
    explicit SrtClient(QObject *parent = nullptr);
    ~SrtClient();

signals:
    void frameReceived(const cv::Mat &frame);

public slots:
    void startClient();
    void stopClient();

private:
    // 声明必须匹配：static 成员，参数是 GstElement*
    static GstFlowReturn onNewSample(GstElement *sink, gpointer data);
    GstElement *pipeline = nullptr;
    bool isRunning = false;
};

#endif

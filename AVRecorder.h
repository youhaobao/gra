#ifndef AVRECORDER_H
#define AVRECORDER_H

#include <QObject>
#include <gst/gst.h>
#include <opencv2/opencv.hpp>
#include <QByteArray>
#include <QString>

class AVRecorder : public QObject {
    Q_OBJECT
public:
    explicit AVRecorder(QObject *parent = nullptr);
    ~AVRecorder();

public slots:
    void startRecording(const QString &filePath);
    void stopRecording();
    void pushVideoFrame(const cv::Mat &nv12Frame);
    void pushAudioBuffer(const QByteArray &audioData);

private:
    GstElement *pipeline = nullptr;
    GstElement *vsrc = nullptr;
    GstElement *asrc = nullptr;
    bool isRecording = false;
};

#endif // AVRECORDER_H

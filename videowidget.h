#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QImage>
#include <QMutex>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QRect>
#include <QThread>
#include <atomic>
#include <QByteArray> // 👈 接收音频流必须包含这个头文件

// 引入自定义类
#include "SrtServer.h"
#include "SrtClient.h"
#include "RgaConverter.h"
#include "FaceProcessor.h"
#include "AVRecorder.h" // 👈 必须包含新创建的录像机类头文件

// 采集工作线程类
class CaptureWorker : public QObject {
    Q_OBJECT
public:
    explicit CaptureWorker(QObject *parent = nullptr);
public slots:
    void startWork();
    void stopWork();
signals:
    void frameCaptured(cv::Mat frame);
private:
    std::atomic<bool> m_running;
};

// 主界面类
class VideoWidget : public QWidget {
    Q_OBJECT
public:
    explicit VideoWidget(QWidget *parent = nullptr);
    ~VideoWidget();

    // 📢 所有对外的接口都要放在 public slots 下
public slots:
    void takeSnapshot();                    // 拍照
    void toggleRecording();                 // 录像开关
    void receiveAudio(QByteArray data);     // 接收音频流

    void handleRawFrame(cv::Mat frame);     // 处理采集到的原始 YUYV
    void handleStreamFrame(cv::Mat frame);  // 处理 SRT 拉流回来的 NV12
    void onFaceDetected(const QRect &rect); // 处理人脸框
    void togglePushStream();                // 推拉流切换

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QLabel *videoLabel;
    QImage m_currentImage;
    QMutex m_imgMutex;
    QRect m_currentFaceRect;

    // 各个子线程和处理器
    QThread *captureThread;
    CaptureWorker *captureWorker;

    QThread *pThread;
    FaceProcessor *processor;

    QThread *serverThread;
    SrtServer *server;

    QThread *clientThread;
    SrtClient *client;

    // 📢 新增：音视频录制器与它的专属线程
    QThread *recorderThread;
    AVRecorder *recorder;

    RgaConverter rga;
    QPushButton *btnPush;
    bool isPushing;

    // 原子状态控制位 (多线程安全)
    std::atomic<bool> m_snapshotRequested{false}; // 拍照状态
    std::atomic<bool> m_isRecording{false};       // 录像状态
};

#endif // VIDEOWIDGET_H

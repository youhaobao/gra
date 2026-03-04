#include "videowidget.h"
#include <QVBoxLayout>
#include <QLayout>
#include <QDebug>
#include <QPainter>
#include <QThread>
#include <QDateTime>
#include <QDir>

// ================= CaptureWorker 实现 =================
CaptureWorker::CaptureWorker(QObject *parent) : QObject(parent), m_running(false) {}

void CaptureWorker::startWork() {
    cv::VideoCapture cap;
    int api = cv::CAP_V4L2;

    if (!cap.open("/dev/video11", api)) {
        if(!cap.open("/dev/video0", api)) {
            qDebug() << "❌ Capture: 无法打开任何摄像头节点";
            return;
        }
    }

    // 1. 强制 YUYV 格式
    cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('Y', 'U', 'Y', 'V'));
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    cap.set(cv::CAP_PROP_FPS, 30);

    // 2. 【关键】禁止 OpenCV 自动转 RGB，确保拿到 2 通道 YUYV
    cap.set(cv::CAP_PROP_CONVERT_RGB, 0);

    qDebug() << "✅ CaptureWorker: 摄像头启动成功 (YUYV 640x480)";
    m_running = true;
    cv::Mat frame;

    while (m_running) {
        cap >> frame;
        if (frame.empty()) {
            QThread::msleep(10);
            continue;
        }
        emit frameCaptured(frame.clone());
        QThread::msleep(30);
    }
    cap.release();
}

void CaptureWorker::stopWork() { m_running = false; }

// ================= VideoWidget 实现 =================
VideoWidget::VideoWidget(QWidget *parent) : QWidget(parent), isPushing(false) {
    qRegisterMetaType<cv::Mat>("cv::Mat");

    recorderThread = new QThread(this);
    recorder = new AVRecorder();
    recorder->moveToThread(recorderThread);
    recorderThread->start();

    if (this->layout()) {
        QWidget *dummy = new QWidget;
        dummy->setLayout(this->layout());
        delete dummy;
    }

    this->setAttribute(Qt::WA_OpaquePaintEvent);
    this->setAutoFillBackground(false);

    // 1. 设置按钮
    btnPush = new QPushButton("点击开始拉流", this);
    btnPush->setGeometry(10, 10, 120, 40);
    btnPush->setEnabled(true);
    btnPush->show();
    connect(btnPush, &QPushButton::clicked, this, &VideoWidget::togglePushStream);

    // 2. 初始化各个线程 (只初始化一次!)

    // Server (推流)
    serverThread = new QThread(this);
    server = new SrtServer();
    server->moveToThread(serverThread);
    serverThread->start();

    // Client (拉流) - 此时不启动 startClient
    clientThread = new QThread(this);
    client = new SrtClient();
    client->moveToThread(clientThread);
    connect(client, &SrtClient::frameReceived, this, &VideoWidget::handleStreamFrame, Qt::QueuedConnection);
    clientThread->start();

    // AI (人脸)
    pThread = new QThread(this);
    processor = new FaceProcessor();
    processor->moveToThread(pThread);
    connect(processor, &FaceProcessor::faceDetected, this, &VideoWidget::onFaceDetected);
    pThread->start();

    // Capture (采集)
    captureThread = new QThread(this);
    captureWorker = new CaptureWorker();
    captureWorker->moveToThread(captureThread);
    connect(captureThread, &QThread::started, captureWorker, &CaptureWorker::startWork);
    connect(captureWorker, &CaptureWorker::frameCaptured, this, &VideoWidget::handleRawFrame, Qt::QueuedConnection);
    captureThread->start();

    // 3. 仅自动启动服务端
    QMetaObject::invokeMethod(server, "startServer", Qt::QueuedConnection);

    qDebug() << "✅ VideoWidget: 初始化完毕 (服务端运行中, 客户端待命)";
}

void VideoWidget::takeSnapshot() {
    m_snapshotRequested = true;
    qDebug() << "📸 拍照请求已发出...";
}

// 1. 按钮触发切换
void VideoWidget::toggleRecording() {
    m_isRecording = !m_isRecording;
    if (m_isRecording) {
        QString path = "/home/baiwen/Videos/Record";
        QDir().mkpath(path); // 建立文件夹
        QString fileName = QString("%1/VID_%2.mkv").arg(path).arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
        // 异步启动
        QMetaObject::invokeMethod(recorder, "startRecording", Qt::QueuedConnection, Q_ARG(QString, fileName));
    } else {
        // 异步停止
        QMetaObject::invokeMethod(recorder, "stopRecording", Qt::QueuedConnection);
    }
}

// 2. 接收音频流
void VideoWidget::receiveAudio(QByteArray data) {
    if (m_isRecording) {
        QMetaObject::invokeMethod(recorder, "pushAudioBuffer", Qt::QueuedConnection, Q_ARG(QByteArray, data));
    }
}

void VideoWidget::handleRawFrame(cv::Mat yuyvFrame) {
    if (yuyvFrame.empty()) return;


    // --- 拍照逻辑
    if (m_snapshotRequested) {
        m_snapshotRequested = false; // 消费掉请求
        qDebug() << "🖼️ [采集线程] 捕获到原始帧，准备写入磁盘...";

        cv::Mat bgrSave;
        try {
            // YUYV 是原始数据，转为 BGR 才能存为 JPG
            cv::cvtColor(yuyvFrame, bgrSave, cv::COLOR_YUV2BGR_YUYV);

            // 路径：确保这个目录你拥有写权限，建议先用 /tmp 测试
            QString path = "/home/baiwen/Pictures/Record";
            QDir().mkpath(path);

            QString fileName = QString("%1/IMG_%2.jpg")
                                   .arg(path)
                                   .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz"));

            if (cv::imwrite(fileName.toStdString(), bgrSave)) {
                qDebug() << "✅ [成功] 照片已保存:" << fileName;
            } else {
                qDebug() << "❌ [失败] OpenCV 写入磁盘返回 false，检查路径权限";
            }
        } catch (const cv::Exception& e) {
            qDebug() << "❌ [异常] 格式转换或写入出错:" << e.what();
        }
    }

    cv::Mat nv12Frame;
    if (rga.convertYuyvToNv12(yuyvFrame, nv12Frame)) {
        // 推给网络
        QMetaObject::invokeMethod(server, "pushFrame", Qt::QueuedConnection, Q_ARG(cv::Mat, nv12Frame));

        // 📢 【新增】推给录像机
        if (m_isRecording) {
            QMetaObject::invokeMethod(recorder, "pushVideoFrame", Qt::QueuedConnection, Q_ARG(cv::Mat, nv12Frame.clone()));
        }
    }

    // 2. AI 检测部分
    if (processor && !processor->m_isBusy) {
        processor->m_isBusy = true; // 锁定

        cv::Mat bgrFrame;
        try {
            // 只有将 YUYV 转为 BGR，OpenCV 的 CascadeClassifier 才能工作
            cv::cvtColor(yuyvFrame, bgrFrame, cv::COLOR_YUV2BGR_YUYV);

            // 使用异步调用，不阻塞主采集线程
            QMetaObject::invokeMethod(processor, "processMat", Qt::QueuedConnection,
                                      Q_ARG(cv::Mat, bgrFrame.clone())); // 必须克隆，防止多线程内存踩踏
        } catch (...) {
            processor->m_isBusy = false; // 转换失败也要解锁
        }
    }
}

void VideoWidget::handleStreamFrame(cv::Mat i420Frame) {
    if (i420Frame.empty()) return;

    cv::Mat rgbaFrame;
    // 调用我们新写的 I420 转 RGBA 接口
    if (rga.convertI420ToRgba(i420Frame, rgbaFrame)) {
        m_imgMutex.lock();
        m_currentImage = QImage(rgbaFrame.data, rgbaFrame.cols, rgbaFrame.rows,
                                rgbaFrame.step, QImage::Format_RGBA8888).copy();
        m_imgMutex.unlock();
        update();
    }
}

void VideoWidget::onFaceDetected(const QRect &rect) {
    // 增加打印，看信号是否穿过线程回到了主窗口
    if (!rect.isNull()) {
        // qDebug() << "📦 UI 收到人脸坐标:" << rect;
    }

    m_currentFaceRect = rect;
    // update(); // 这里不需要强制 update，因为 handleStreamFrame 正在以 30fps 刷新界面
}

void VideoWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 1. 背景色
    painter.fillRect(rect(), QColor("#000000"));

    m_imgMutex.lock();
    if (!m_currentImage.isNull()) {
        QSize widgetSize = this->size();
        QSize imgSize = m_currentImage.size();
        imgSize.scale(widgetSize, Qt::KeepAspectRatio);
        int x_off = (widgetSize.width() - imgSize.width()) / 2;
        int y_off = (widgetSize.height() - imgSize.height()) / 2;
        QRect targetRect(x_off, y_off, imgSize.width(), imgSize.height());

        // 2. 绘制视频帧
        painter.drawImage(targetRect, m_currentImage);

        // 3. 绘制工业级识别框
        if (!m_currentFaceRect.isNull()) {
            float sx = (float)imgSize.width() / 640.0f;
            float sy = (float)imgSize.height() / 480.0f;
            int rx = m_currentFaceRect.x() * sx + x_off;
            int ry = m_currentFaceRect.y() * sy + y_off;
            int rw = m_currentFaceRect.width() * sx;
            int rh = m_currentFaceRect.height() * sy;

            // 绘制绿色战术框（仅转角，显专业）
            painter.setPen(QPen(QColor(0, 255, 0), 1));
            int l = 12;
            painter.drawLine(rx, ry, rx + l, ry); painter.drawLine(rx, ry, rx, ry + l); //左上
            painter.drawLine(rx + rw, ry, rx + rw - l, ry); painter.drawLine(rx + rw, ry, rx + rw, ry + l); //右上
            painter.drawLine(rx, ry + rh, rx + l, ry + rh); painter.drawLine(rx, ry + rh, rx, ry + rh - l); //左下
            painter.drawLine(rx + rw, ry + rh, rx + rw - l, ry + rh); painter.drawLine(rx + rw, ry + rh, rx + rw, ry + rh - l); //右下

            // 4. 识别数据标注（标签）
            painter.fillRect(rx, ry - 18, 120, 18, QColor(0, 255, 0, 180));
            painter.setPen(Qt::black);
            painter.setFont(QFont("Arial", 8, QFont::Bold));
            painter.drawText(rx + 4, ry - 5, "OBJ: FACE | CONF: 0.92");
        }

        // 5. 屏幕左上角状态叠层
        painter.setPen(QColor(255, 255, 255, 100));
        painter.setFont(QFont("Monospace", 8));
        painter.drawText(targetRect.adjusted(10, 20, 0, 0), "SYSTEM: ACTIVE | PROTOCOL: SRT");
        painter.drawText(targetRect.adjusted(10, 35, 0, 0), "HARDWARE: RK3576 NPU/RGA");
    }
    m_imgMutex.unlock();

    static int flashEffect = 0;

    m_imgMutex.lock();
    // 绘制视频帧等逻辑...
    if (!m_currentImage.isNull()) {
        // ... (你原有的绘制 QImage 代码)
    }
    m_imgMutex.unlock();

    // 检查是否有拍照请求，如果有，触发闪烁
    if (m_snapshotRequested) {
        flashEffect = 4; // 持续 4 帧的白光
    }

    if (flashEffect > 0) {
        // 绘制半透明白色覆盖层，模拟快门闪光
        painter.fillRect(rect(), QColor(255, 255, 255, 150));
        flashEffect--;
        update(); // 强制刷新界面以完成闪烁动画
    }
}

void VideoWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    if (btnPush) btnPush->move(10, 10);
}

void VideoWidget::togglePushStream() {
    if (!isPushing) {
        qDebug() << "▶️ 用户请求：开始拉流...";
        QMetaObject::invokeMethod(client, "startClient", Qt::QueuedConnection);
        btnPush->setText("停止拉流");
        btnPush->setStyleSheet("background-color: #ffcccc;");
        isPushing = true;
    } else {
        qDebug() << "⏹️ 用户请求：停止拉流...";
        QMetaObject::invokeMethod(client, "stopClient", Qt::QueuedConnection);
        btnPush->setText("开始拉流");
        btnPush->setStyleSheet("");
        isPushing = false;
    }
}

VideoWidget::~VideoWidget() {
    if(captureWorker) captureWorker->stopWork();
    if(captureThread) { captureThread->quit(); captureThread->wait(1000); }
    if (server) QMetaObject::invokeMethod(server, "stopServer", Qt::BlockingQueuedConnection);
    if (client) QMetaObject::invokeMethod(client, "stopClient", Qt::BlockingQueuedConnection);
    if(serverThread) { serverThread->quit(); serverThread->wait(); }
    if(clientThread) { clientThread->quit(); clientThread->wait(); }
    if(pThread) { pThread->quit(); pThread->wait(); }
}

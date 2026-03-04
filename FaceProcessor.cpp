#include "FaceProcessor.h"
#include <QDebug>

FaceProcessor::FaceProcessor(QObject *parent) : QObject(parent), m_isBusy(false) {
    // 确保路径正确
    if (faceCascade.load("/home/baiwen/Downloads/lbpcascade_frontalface.xml")) {
        qDebug() << "✅ AI线程: 成功加载人脸模型";
    } else {
        qDebug() << "❌ AI线程: 模型加载失败，请检查路径！";
    }
}

void FaceProcessor::processMat(cv::Mat frame) {
    // 只要函数退出，就强制把 busy 设为 false
    struct BusyGuard {
        std::atomic<bool> &flag;
        ~BusyGuard() { flag = false; }
    } guard{m_isBusy};

    if (frame.empty()) return;

    try {
        cv::Mat gray, smallImg;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        cv::equalizeHist(gray, gray);

        // 缩放尺寸对 AI 性能影响极大
        cv::resize(gray, smallImg, cv::Size(320, 240));

        std::vector<cv::Rect> faces;
        // 参数调优: 降低 minNeighbors 到 2 提高检测率，minSize 设为 30x30
        faceCascade.detectMultiScale(smallImg, faces, 1.1, 2, 0, cv::Size(30, 30));

        if (!faces.empty()) {
            float scaleX = (float)frame.cols / 320.0f;
            float scaleY = (float)frame.rows / 240.0f;
            cv::Rect f = faces[0];

            QRect rect(f.x * scaleX, f.y * scaleY, f.width * scaleX, f.height * scaleY);

            // 📢 这里的打印非常关键，必须看到它！
            //qDebug() << "🎯 AI: 捕捉到人脸 ->" << rect;

            emit faceDetected(rect);
        } else {
            // 没检测到也发个信号，表示当前帧是空的
            emit faceDetected(QRect());
        }
    } catch (const std::exception& e) {
        qDebug() << "❌ AI检测崩溃:" << e.what();
    }
}

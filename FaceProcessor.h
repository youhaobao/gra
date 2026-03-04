#ifndef FACEPROCESSOR_H
#define FACEPROCESSOR_H

#include <QObject>
#include <QRect>
#include <atomic>
#include <opencv2/opencv.hpp>

Q_DECLARE_METATYPE(cv::Mat)

class FaceProcessor : public QObject {
    Q_OBJECT
public:
    explicit FaceProcessor(QObject *parent = nullptr);
    std::atomic<bool> m_isBusy;

public slots:
    void processMat(cv::Mat frame);

signals:
    void faceDetected(const QRect &rect);
    void frameReadyToPush(const cv::Mat &frame);

private:
    cv::CascadeClassifier faceCascade;
};

#endif

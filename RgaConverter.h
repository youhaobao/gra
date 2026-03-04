#ifndef RGACONVERTER_H
#define RGACONVERTER_H

#include <opencv2/opencv.hpp>
#include <rga/RgaApi.h>
#include <rga/im2d.h>

class RgaConverter {
public:
    RgaConverter();
    ~RgaConverter();

    // 采集端：YUYV -> NV12
    bool convertYuyvToNv12(const cv::Mat &srcYuyv, cv::Mat &dstNv12);

    // 拉流端：I420 -> RGBA (解决花屏/绿屏的最终方案)
    bool convertI420ToRgba(const cv::Mat &srcI420, cv::Mat &dstRgba);

private:
    void* m_bufYuv;  // 对齐的 YUV 缓冲区
    void* m_bufRgba; // 对齐的 RGBA 缓冲区
};

#endif

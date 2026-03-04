#include "RgaConverter.h"
#include <QDebug>
#include <stdlib.h>
#include <cstring>

RgaConverter::RgaConverter() : m_bufYuv(nullptr), m_bufRgba(nullptr) {
    // 申请 4K 对齐内存。RK3576 RGA 必须在对齐地址上操作才能避免全绿
    if (posix_memalign(&m_bufYuv, 4096, 2 * 1024 * 1024) != 0) qDebug() << "YUV Alloc Fail";
    if (posix_memalign(&m_bufRgba, 4096, 4 * 1024 * 1024) != 0) qDebug() << "RGBA Alloc Fail";
}

RgaConverter::~RgaConverter() {
    if (m_bufYuv) free(m_bufYuv);
    if (m_bufRgba) free(m_bufRgba);
}

// 客户端拉流：I420 -> RGBA (100% 解决紫绿条纹和全绿)
bool RgaConverter::convertI420ToRgba(const cv::Mat &srcI420, cv::Mat &dstRgba) {
    if (srcI420.empty() || !m_bufRgba) return false;
    int w = srcI420.cols;
    int h = srcI420.rows * 2 / 3;

    // 1. 将洗干净的 I420 考入对齐区
    memcpy(m_bufYuv, srcI420.data, w * h * 3 / 2);

    rga_buffer_t src, dst;
    memset(&src, 0, sizeof(src));
    memset(&dst, 0, sizeof(dst));

    // 2. 设置源为 I420 (RK_FORMAT_YCbCr_420_P 是三平面 I420)
    src = wrapbuffer_virtualaddr(m_bufYuv, w, h, RK_FORMAT_YCbCr_420_P);
    src.wstride = w;
    src.hstride = h;

    // 3. 设置目标为 RGBA
    dst = wrapbuffer_virtualaddr(m_bufRgba, w, h, RK_FORMAT_RGBA_8888);
    dst.wstride = w;
    dst.hstride = h;

    // 4. 执行转换 (使用 imcvtcolor，简单稳定)
    IM_STATUS status = imcvtcolor(src, dst, src.format, dst.format);
    if (status != IM_STATUS_SUCCESS) return false;

    // 返回 4 通道 Mat
    dstRgba = cv::Mat(h, w, CV_8UC4, m_bufRgba).clone();
    return true;
}

// 服务端：YUYV -> NV12
bool RgaConverter::convertYuyvToNv12(const cv::Mat &srcYuyv, cv::Mat &dstNv12) {
    if (srcYuyv.empty() || !m_bufYuv) return false;
    int w = srcYuyv.cols;
    int h = srcYuyv.rows;
    memcpy(m_bufYuv, srcYuyv.data, w * h * 2);
    rga_buffer_t src = wrapbuffer_virtualaddr(m_bufYuv, w, h, RK_FORMAT_YUYV_422);
    rga_buffer_t dst = wrapbuffer_virtualaddr(m_bufRgba, w, h, RK_FORMAT_YCbCr_420_SP);
    if (imcvtcolor(src, dst, src.format, dst.format) != IM_STATUS_SUCCESS) return false;
    dstNv12 = cv::Mat(h * 3 / 2, w, CV_8UC1, m_bufRgba).clone();
    return true;
}

// SrtServer.cpp
#include "SrtServer.h"
#include <QDebug>
#include <gst/app/gstappsrc.h>

SrtServer::SrtServer(QObject *parent) : QObject(parent) {
    gst_init(nullptr, nullptr);
}

SrtServer::~SrtServer() {
    stopServer();
}

void SrtServer::startServer() {
    if (isRunning) return;

    GError *error = nullptr;

    // 首先尝试硬件编码器 (mpph264enc)
    QString hwPipeStr =
        "appsrc name=mysink is-live=true format=3 ! "
        "video/x-raw,format=NV12,width=640,height=480,framerate=30/1 ! "
        "mpph264enc gop=30 bps=2000000 ! "
        "h264parse config-interval=1 ! "
        "mpegtsmux ! "
        "srtsink uri=\"srt://:8890?mode=listener\" sync=false";

    pipeline = gst_parse_launch(hwPipeStr.toUtf8().data(), &error);
    
    if (error || !pipeline) {
        qDebug() << "⚠️ 硬件编码器 mpph264enc 启动失败，切换到软件编码器 x264enc";
        if (error) {
            g_error_free(error);
            error = nullptr;
        }
        if (pipeline) {
            gst_object_unref(pipeline);
            pipeline = nullptr;
        }

        // 使用软件编码器 (x264enc)
        QString swPipeStr =
            "appsrc name=mysink is-live=true format=3 ! "
            "video/x-raw,format=NV12,width=640,height=480,framerate=30/1 ! "
            "x264enc key-int-max=30 bitrate=2000 speed-preset=ultrafast tune=zerolatency ! "
            "h264parse config-interval=1 ! "
            "mpegtsmux ! "
            "srtsink uri=\"srt://:8890?mode=listener\" sync=false";

        pipeline = gst_parse_launch(swPipeStr.toUtf8().data(), &error);
        
        if (error || !pipeline) {
            qDebug() << "❌ 软件编码器也启动失败:" << (error ? error->message : "unknown error");
            if (error) g_error_free(error);
            if (pipeline) gst_object_unref(pipeline);
            pipeline = nullptr;
            return;
        }
        
        qDebug() << "✅ 使用软件编码器 x264enc";
    } else {
        qDebug() << "✅ 使用硬件编码器 mpph264enc";
    }

    appsrc = gst_bin_get_by_name(GST_BIN(pipeline), "mysink");
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    isRunning = true;
    qDebug() << "✅ SRT Server started (Config Interval Enabled)";
}

void SrtServer::stopServer() {
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        pipeline = nullptr;
    }
    isRunning = false;
}

void SrtServer::pushFrame(const cv::Mat &frame) {
    if (!appsrc || !isRunning || frame.empty()) return;

    GstBuffer *buffer = gst_buffer_new_allocate(nullptr, frame.total(), nullptr);
    gst_buffer_fill(buffer, 0, frame.data, frame.total());

    static GstClockTime timestamp = 0;
    GST_BUFFER_PTS(buffer) = timestamp;
    GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, 30);
    timestamp += GST_BUFFER_DURATION(buffer);

    gst_app_src_push_buffer(GST_APP_SRC(appsrc), buffer);
}

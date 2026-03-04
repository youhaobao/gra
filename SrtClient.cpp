#include "SrtClient.h"
#include <QDebug>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/video/video.h> // 必须包含

SrtClient::SrtClient(QObject *parent) : QObject(parent) {
    gst_init(nullptr, nullptr);
}

SrtClient::~SrtClient() {
    stopClient();
}

GstFlowReturn SrtClient::onNewSample(GstElement *sink, gpointer data) {
    SrtClient *self = static_cast<SrtClient*>(data);
    GstAppSink *appsink = GST_APP_SINK(sink);
    GstSample *sample = gst_app_sink_pull_sample(appsink);
    if (!sample) return GST_FLOW_ERROR;

    GstCaps *caps = gst_sample_get_caps(sample);
    GstVideoInfo info;
    gst_video_info_from_caps(&info, caps);

    GstBuffer *buffer = gst_sample_get_buffer(sample);
    GstMapInfo map;
    if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        int w = info.width;
        int h = info.height;

        // I420 三平面拷贝：彻底解决任何 Stride 问题
        cv::Mat i420_clean(h * 3 / 2, w, CV_8UC1);
        uint8_t *src = (uint8_t *)map.data;
        uint8_t *dst = i420_clean.data;

        // 1. 拷贝 Y (亮度)
        for (int i = 0; i < h; i++) {
            memcpy(dst + i * w, src + info.offset[0] + i * info.stride[0], w);
        }
        // 2. 拷贝 U (色度)
        uint8_t *dst_u = dst + w * h;
        for (int i = 0; i < h / 2; i++) {
            memcpy(dst_u + i * w / 2, src + info.offset[1] + i * info.stride[1], w / 2);
        }
        // 3. 拷贝 V (色度)
        uint8_t *dst_v = dst + w * h + (w * h / 4);
        for (int i = 0; i < h / 2; i++) {
            memcpy(dst_v + i * w / 2, src + info.offset[2] + i * info.stride[2], w / 2);
        }

        // 验证：存最后一张图。如果这张图正了，说明 I420 洗涤成功！
        /*static int cnt = 0;
        if (cnt++ == 60) {
            cv::Mat bgr;
            cv::cvtColor(i420_clean, bgr, cv::COLOR_YUV2BGR_I420);
            cv::imwrite("client_i420_final.jpg", bgr);
        }
        */

        emit self->frameReceived(i420_clean.clone());
        gst_buffer_unmap(buffer, &map);
    }
    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

void SrtClient::startClient() {
    if (isRunning) return;
    // 【关键】强制输出 format=I420。videoconvert 会在这里解开 AFBC 压缩块
    QString pipeStr =
        "srtsrc uri=\"srt://127.0.0.1:8890?mode=caller\" latency=100 ! "
        "tsdemux ! h264parse ! mppvideodec ! "
        "videoconvert ! video/x-raw,format=I420,width=640,height=480 ! "
        "appsink name=mysink emit-signals=true sync=false max-buffers=1 drop=true";

    GError *error = nullptr;
    pipeline = gst_parse_launch(pipeStr.toUtf8().data(), &error);
    if (error) {
        qDebug() << "❌ Client Error:" << error->message;
        g_error_free(error);
        return;
    }

    GstElement *appsink = gst_bin_get_by_name(GST_BIN(pipeline), "mysink");
    if (appsink) {
        g_signal_connect(appsink, "new-sample", G_CALLBACK(SrtClient::onNewSample), this);
        gst_object_unref(appsink);
    }

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    isRunning = true;
    qDebug() << "✅ SRT Client Pipeline Started";
}

void SrtClient::stopClient() {
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        pipeline = nullptr;
    }
    isRunning = false;
}

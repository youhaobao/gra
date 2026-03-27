#include "AVRecorder.h"
#include <QDebug>
#include <gst/app/gstappsrc.h>

AVRecorder::AVRecorder(QObject *parent) : QObject(parent) {
    gst_init(nullptr, nullptr);
}

AVRecorder::~AVRecorder() {
    stopRecording();
}

void AVRecorder::startRecording(const QString &filePath) {
    if (isRecording) return;

    GError *error = nullptr;

    // 首先尝试硬件编码器 (mpph264enc)
    QString hwPipeStr = QString(
                          "appsrc name=vsrc is-live=true do-timestamp=true format=time ! "
                          "video/x-raw,format=NV12,width=640,height=480,framerate=30/1 ! "
                          "queue ! mpph264enc ! h264parse ! queue ! matroskamux name=mux ! "
                          "filesink location=\"%1\" "
                          "appsrc name=asrc is-live=true do-timestamp=true format=time ! "
                          "audio/x-raw,format=S16LE,rate=44100,channels=2,layout=interleaved ! "
                          "queue ! audioconvert ! audioresample ! opusenc ! queue ! mux."
                          ).arg(filePath);

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
        QString swPipeStr = QString(
                              "appsrc name=vsrc is-live=true do-timestamp=true format=time ! "
                              "video/x-raw,format=NV12,width=640,height=480,framerate=30/1 ! "
                              "queue ! x264enc ! h264parse ! queue ! matroskamux name=mux ! "
                              "filesink location=\"%1\" "
                              "appsrc name=asrc is-live=true do-timestamp=true format=time ! "
                              "audio/x-raw,format=S16LE,rate=44100,channels=2,layout=interleaved ! "
                              "queue ! audioconvert ! audioresample ! opusenc ! queue ! mux."
                              ).arg(filePath);

        pipeline = gst_parse_launch(swPipeStr.toUtf8().data(), &error);

        if (error || !pipeline) {
            qCritical() << "❌ AVRecorder 管道创建失败:" << (error ? error->message : "unknown error");
            if (error) g_error_free(error);
            if (pipeline) gst_object_unref(pipeline);
            pipeline = nullptr;
            return;
        }
        
        qDebug() << "✅ 使用软件编码器 x264enc";
    } else {
        qDebug() << "✅ 使用硬件编码器 mpph264enc";
    }

    vsrc = gst_bin_get_by_name(GST_BIN(pipeline), "vsrc");
    asrc = gst_bin_get_by_name(GST_BIN(pipeline), "asrc");

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    isRecording = true;
    qDebug() << "🔴 音视频录制已启动:" << filePath;
}

void AVRecorder::stopRecording() {
    if (!isRecording || !pipeline) return;

    // 关键：发送 EOS 让容器正常封包，防止文件损坏
    if (vsrc) gst_app_src_end_of_stream(GST_APP_SRC(vsrc));
    if (asrc) gst_app_src_end_of_stream(GST_APP_SRC(asrc));

    GstBus *bus = gst_element_get_bus(pipeline);
    gst_bus_timed_pop_filtered(bus, 3 * GST_SECOND, (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    gst_object_unref(bus);

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    pipeline = nullptr;
    vsrc = asrc = nullptr;
    isRecording = false;
    qDebug() << "⬛ 音视频录制已安全封包。";
}

void AVRecorder::pushVideoFrame(const cv::Mat &frame) {
    if (!vsrc || !isRecording || frame.empty()) return;
    gsize size = frame.total();
    GstBuffer *buffer = gst_buffer_new_allocate(nullptr, size, nullptr);
    gst_buffer_fill(buffer, 0, frame.data, size);

    GstFlowReturn ret;
    g_signal_emit_by_name(vsrc, "push-buffer", buffer, &ret);
    gst_buffer_unref(buffer);
}

void AVRecorder::pushAudioBuffer(const QByteArray &data) {
    if (!asrc || !isRecording || data.isEmpty()) return;
    GstBuffer *buffer = gst_buffer_new_allocate(nullptr, data.size(), nullptr);
    gst_buffer_fill(buffer, 0, data.constData(), data.size());

    GstFlowReturn ret;
    g_signal_emit_by_name(asrc, "push-buffer", buffer, &ret);
    gst_buffer_unref(buffer);
}

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

    // 构建 GStreamer 硬件编码双轨管道 (Video: H264, Audio: Opus)
    QString pipeStr = QString(
                          "appsrc name=vsrc is-live=true do-timestamp=true format=time ! "
                          "video/x-raw,format=NV12,width=640,height=480,framerate=30/1 ! "
                          "queue ! mpph264enc ! h264parse ! queue ! matroskamux name=mux ! "
                          "filesink location=\"%1\" "
                          "appsrc name=asrc is-live=true do-timestamp=true format=time ! "
                          "audio/x-raw,format=S16LE,rate=44100,channels=2,layout=interleaved ! "
                          "queue ! audioconvert ! audioresample ! opusenc ! queue ! mux."
                          ).arg(filePath);

    GError *error = nullptr;
    pipeline = gst_parse_launch(pipeStr.toUtf8().data(), &error);
    if (error) {
        qCritical() << "❌ AVRecorder 管道创建失败:" << error->message;
        g_error_free(error);
        return;
    }

    vsrc = gst_bin_get_by_name(GST_BIN(pipeline), "vsrc");
    asrc = gst_bin_get_by_name(GST_BIN(pipeline), "asrc");

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    isRecording = true;
    qDebug() << "🔴 硬件级音视频录制已启动:" << filePath;
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

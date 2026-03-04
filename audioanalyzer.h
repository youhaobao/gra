// AudioAnalyzer.h - 保持原有结构，只修改参数
#ifndef AUDIOANALYZER_H
#define AUDIOANALYZER_H

#include <QObject>
#include <QAudioInput>
#include <QAudioFormat>
#include <QAudioDeviceInfo>
#include <QByteArray>
#include <QVector>
#include <QTimer>

class AudioAnalyzer : public QObject
{
    Q_OBJECT

public:
    explicit AudioAnalyzer(QObject *parent = nullptr);
    ~AudioAnalyzer();

    bool start();// 开始音频分析
    void stop();// 停止音频分析

signals:
    void spectrumReady(const QVector<qreal> &spectrum);  // 音量变化信号
    void rawAudioReady(const QByteArray &data); // 发送原始音频流

private slots:
    void onAudioDataReady();

private:
    void processAudioData(const QByteArray &data);
    QVector<qreal> calculateSpectrum(const QByteArray &data);

    QAudioDeviceInfo m_audioDeviceInfo;
    QAudioInput *m_audioInput;
    QAudioFormat m_format;
    QIODevice *m_audioDevice;
    static const int BAND_COUNT = 32; // 从 8 增加到 32
};

#endif // AUDIOANALYZER_H

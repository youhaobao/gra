#include "audioanalyzer.h"
#include <QDebug>
#include <cmath>
#include <QElapsedTimer>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

AudioAnalyzer::AudioAnalyzer(QObject *parent)
    : QObject(parent)
    , m_audioInput(nullptr)
    , m_audioDevice(nullptr)
{
    qDebug() << "=== Creating AudioAnalyzer (ES8388 Optimized) ===";

    // 查找 ES8388 设备
    QAudioDeviceInfo selectedDevice = QAudioDeviceInfo::defaultInputDevice();
    QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);

    for (const QAudioDeviceInfo &device : devices) {
        QString deviceName = device.deviceName().toLower();
        if (deviceName.contains("es8388") || deviceName.contains("rockchip")) {
            selectedDevice = device;
            break;
        }
    }

    m_audioDeviceInfo = selectedDevice;

    // 1. 彻底关闭 ALC (全满的元凶之一)，手动控制增益
    system("amixer -c 0 sset 'ALC Capture Function' 'Off'");

    // 2. 将采集源指向麦克风 (Line 1 通常是板载 Mic)
    system("amixer -c 0 sset 'Left PGA Mux' 'Differential'");
    system("amixer -c 0 sset 'Right PGA Mux' 'Differential'");
    system("amixer -c 0 sset 'Differential Mux' 'Line 1'");

    // 3. 设置主麦克风物理增益 (0-31)，先设为中等，防止爆表
    system("amixer -c 0 sset 'Main Mic' 20");

    // 4. 设置数字采集卷 (0-192)，先设为 120
    system("amixer -c 0 sset 'Capture Digital' 120");

    // 5. 确保录音没有被静音
    system("amixer -c 0 sset 'Capture Mute' off");

    // 设置音频格式：44.1kHz, 双声道 (arecord测试出的硬件强制参数)
    m_format.setSampleRate(44100);
    m_format.setChannelCount(2);
    m_format.setSampleSize(16);
    m_format.setCodec("audio/pcm");
    m_format.setByteOrder(QAudioFormat::LittleEndian);
    m_format.setSampleType(QAudioFormat::SignedInt);

    if (!m_audioDeviceInfo.isFormatSupported(m_format)) {
        m_format = m_audioDeviceInfo.nearestFormat(m_format);
    }
}

AudioAnalyzer::~AudioAnalyzer() {
    stop();
}

bool AudioAnalyzer::start() {
    stop();
    m_audioInput = new QAudioInput(m_audioDeviceInfo, m_format, this);
    m_audioInput->setBufferSize(2048);
    m_audioDevice = m_audioInput->start();

    if (!m_audioDevice) {
        qWarning() << "❌ Failed to start audio input";
        return false;
    }

    connect(m_audioDevice, &QIODevice::readyRead, this, &AudioAnalyzer::onAudioDataReady);
    qDebug() << "✅ Audio analyzer started at 44100Hz";
    return true;
}

void AudioAnalyzer::stop() {
    if (m_audioInput) {
        m_audioInput->stop();
        m_audioInput->deleteLater();
        m_audioInput = nullptr;
    }
    m_audioDevice = nullptr;
}

void AudioAnalyzer::onAudioDataReady() {
    if (!m_audioDevice) return;
    QByteArray data = m_audioDevice->readAll();
    if (data.isEmpty()) return;

    // 全量、无损地把音频发给录像引擎
    emit rawAudioReady(data);

    // 节流控制：限制每秒更新 25 次，防止界面卡顿
    static QElapsedTimer throttler;
    if (!throttler.isValid()) throttler.start();
    if (throttler.elapsed() < 40) return;
    throttler.restart();

    QVector<qreal> spectrum = calculateSpectrum(data);
    emit spectrumReady(spectrum);
}

QVector<qreal> AudioAnalyzer::calculateSpectrum(const QByteArray &data)
{
    const int numSamples = data.size() / 4; // Stereo 16bit
    if (numSamples < BAND_COUNT) return QVector<qreal>(BAND_COUNT, 0.0);

    const qint16 *input = reinterpret_cast<const qint16*>(data.constData());

    // 1. 去直流偏移 (保持不变)
    qreal sum = 0;
    for (int i = 0; i < numSamples; i++) sum += input[i * 2];
    qreal mean = sum / numSamples;

    QVector<qreal> spectrum(BAND_COUNT, 0.0);
    int samplesPerBand = numSamples / BAND_COUNT;

    static QVector<qreal> lastSpectrum(BAND_COUNT, 0.0);
    if (lastSpectrum.size() != BAND_COUNT) lastSpectrum.fill(0.0, BAND_COUNT);

    for (int band = 0; band < BAND_COUNT; ++band) {
        int startSample = band * samplesPerBand;
        int endSample = (band == BAND_COUNT - 1) ? numSamples : (band + 1) * samplesPerBand;

        qreal rms = 0;
        for (int i = startSample; i < endSample; ++i) {
            qreal val = (static_cast<qreal>(input[i * 2]) - mean) / 32768.0;
            rms += val * val;
        }
        rms = std::sqrt(rms / (endSample - startSample));

        // --- 【整合网上资源：工业级灵敏度补偿算法】 ---

        // A. 基础暴力放大：由于小声说话 RMS 可能只有 0.0005，我们要乘 200 倍
        qreal value = rms * 200.0;

        // B. 极低噪声门限：只过滤掉绝对的电子静默（0.01）
        if (value < 0.01) {
            value = 0;
        } else {
            // C. 核心修正：平方根映射 (Square Root Mapping)
            // 相比 log，sqrt 能让 0.01 瞬间变成 0.1，让小声音极其显眼
            // 而 1.0 依然是 1.0，不会溢出
            value = std::sqrt(value);
        }

        // D. 灵敏平滑滤波
        // 降低历史权重到 0.5，让律动更“跟手”，不再觉得迟钝
        qreal currentVal = qBound(0.0, value, 1.0);
        lastSpectrum[band] = lastSpectrum[band] * 0.5 + currentVal * 0.5;

        spectrum[band] = lastSpectrum[band];
    }
    return spectrum;
}

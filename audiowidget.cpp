// AudioWidget.cpp
#include "audiowidget.h"
#include <QPainter>
#include <QDateTime>
#include <QRandomGenerator>
#include <QDir>
#include <QFile>
#include <QDebug>

AudioWidget::AudioWidget(QWidget *parent)
    : QWidget(parent)
    , m_spectrum(32, 0.0)
    , m_timer(new QTimer(this))
    , m_isSimulating(false)
    , m_sensitivity(1.5)
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(10);

    m_waveformWidget = new WaveformWidget(this);
    m_mainLayout->addWidget(m_waveformWidget);

    setupProgressBars();

    QString backgroundPath = "/home/baiwen/Downloads/gra/images/audio.png";
    if (QFile::exists(backgroundPath)) {
        m_background.load(backgroundPath);
        qDebug() << "✅ 音频背景图加载成功:" << backgroundPath;
    } else {
        qDebug() << "⚠️ 音频背景图未找到:" << backgroundPath;
    }

    connect(m_timer, &QTimer::timeout, this, &AudioWidget::onTimeout);
    m_timer->start(50);
    initializeRandomSpectrum();
}

AudioWidget::~AudioWidget()
{
}

void AudioWidget::setSpectrum(const QVector<qreal> &values) {
    if (values.size() != m_spectrum.size()) return;

    for (int i = 0; i < m_spectrum.size(); ++i) {
        qreal target = values[i];

        if (target > m_spectrum[i]) {
            m_spectrum[i] = target;
        } else {
            m_spectrum[i] -= 0.05;
            if (m_spectrum[i] < 0) m_spectrum[i] = 0;
        }
    }
    
    if (m_waveformWidget) {
        m_waveformWidget->setSpectrum(m_spectrum);
    }
}

void AudioWidget::setUseRealAudio(bool useReal)
{
    if (useReal) {
        m_timer->stop();
        m_isSimulating = false;
        emit readyForRealAudio();
    } else {
        m_isSimulating = true;
        m_timer->start(50);
        initializeRandomSpectrum();
    }
}

void AudioWidget::setupProgressBars()
{
    m_volumeLabel = new QLabel("音量", this);
    m_volumeLabel->setStyleSheet("font-size: 12px; color: #999999;");
    m_mainLayout->addWidget(m_volumeLabel);
    
    m_volumeBar = new QProgressBar(this);
    m_volumeBar->setRange(0, 100);
    m_volumeBar->setValue(50);
    m_volumeBar->setStyleSheet("QProgressBar { border: none; background-color: #E5E7EB; border-radius: 4px; height: 8px; } QProgressBar::chunk { background-color: #1677FF; border-radius: 4px; }");
    m_mainLayout->addWidget(m_volumeBar);

    m_frequencyLabel = new QLabel("频率", this);
    m_frequencyLabel->setStyleSheet("font-size: 12px; color: #999999;");
    m_mainLayout->addWidget(m_frequencyLabel);
    
    m_frequencyBar = new QProgressBar(this);
    m_frequencyBar->setRange(0, 100);
    m_frequencyBar->setValue(30);
    m_frequencyBar->setStyleSheet("QProgressBar { border: none; background-color: #E5E7EB; border-radius: 4px; height: 8px; } QProgressBar::chunk { background-color: #1677FF; border-radius: 4px; }");
    m_mainLayout->addWidget(m_frequencyBar);

    m_noiseLabel = new QLabel("噪音", this);
    m_noiseLabel->setStyleSheet("font-size: 12px; color: #999999;");
    m_mainLayout->addWidget(m_noiseLabel);
    
    m_noiseBar = new QProgressBar(this);
    m_noiseBar->setRange(0, 100);
    m_noiseBar->setValue(20);
    m_noiseBar->setStyleSheet("QProgressBar { border: none; background-color: #E5E7EB; border-radius: 4px; height: 8px; } QProgressBar::chunk { background-color: #1677FF; border-radius: 4px; }");
    m_mainLayout->addWidget(m_noiseBar);
}

void AudioWidget::initializeRandomSpectrum()
{
    for (int i = 0; i < m_spectrum.size(); ++i) {
        m_spectrum[i] = QRandomGenerator::global()->generateDouble() * 0.6;
    }
    
    if (m_waveformWidget) {
        m_waveformWidget->setSpectrum(m_spectrum);
    }
}

void AudioWidget::onTimeout()
{
    if (!m_isSimulating) return;

    for (int i = 0; i < m_spectrum.size(); ++i) {
        qreal delta = (QRandomGenerator::global()->generateDouble() - 0.5) * 0.2;
        m_spectrum[i] += delta;

        if (QRandomGenerator::global()->generateDouble() < 0.05) {
            m_spectrum[i] += QRandomGenerator::global()->generateDouble() * 0.4;
        }

        m_spectrum[i] = qBound(0.0, m_spectrum[i], 1.0);
    }

    if (m_volumeBar) {
        int currentVolume = m_volumeBar->value();
        int deltaVolume = (QRandomGenerator::global()->generateDouble() - 0.5) * 10;
        int newVolume = qBound(0, currentVolume + deltaVolume, 100);
        m_volumeBar->setValue(newVolume);
    }

    if (m_frequencyBar) {
        int currentFreq = m_frequencyBar->value();
        int deltaFreq = (QRandomGenerator::global()->generateDouble() - 0.5) * 8;
        int newFreq = qBound(0, currentFreq + deltaFreq, 100);
        m_frequencyBar->setValue(newFreq);
    }

    if (m_noiseBar) {
        int currentNoise = m_noiseBar->value();
        int deltaNoise = (QRandomGenerator::global()->generateDouble() - 0.5) * 5;
        int newNoise = qBound(0, currentNoise + deltaNoise, 100);
        m_noiseBar->setValue(newNoise);
    }

    if (m_waveformWidget) {
        m_waveformWidget->setSpectrum(m_spectrum);
    }
}

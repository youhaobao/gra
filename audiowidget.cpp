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
    // 设置布局
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(10);

    // 创建波形图widget
    m_waveformWidget = new QWidget(this);
    m_waveformWidget->setFixedHeight(100);
    m_mainLayout->addWidget(m_waveformWidget);

    // 设置进度条
    setupProgressBars();

    // === 加载背景图片 ===
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
        qreal target = values[i]; // 不再乘以 0.8

        if (target > m_spectrum[i]) {
            // 瞬间弹起，无延迟
            m_spectrum[i] = target;
        } else {
            // 优雅下落
            m_spectrum[i] -= 0.05;
            if (m_spectrum[i] < 0) m_spectrum[i] = 0;
        }
    }
    update();
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
    // 音量进度条
    m_volumeLabel = new QLabel("音量", this);
    m_volumeLabel->setStyleSheet("font-size: 12px; color: #999999;");
    m_mainLayout->addWidget(m_volumeLabel);
    
    m_volumeBar = new QProgressBar(this);
    m_volumeBar->setRange(0, 100);
    m_volumeBar->setValue(50);
    m_volumeBar->setStyleSheet("QProgressBar { border: none; background-color: #E5E7EB; border-radius: 4px; height: 8px; } QProgressBar::chunk { background-color: #1677FF; border-radius: 4px; }");
    m_mainLayout->addWidget(m_volumeBar);

    // 频率进度条
    m_frequencyLabel = new QLabel("频率", this);
    m_frequencyLabel->setStyleSheet("font-size: 12px; color: #999999;");
    m_mainLayout->addWidget(m_frequencyLabel);
    
    m_frequencyBar = new QProgressBar(this);
    m_frequencyBar->setRange(0, 100);
    m_frequencyBar->setValue(30);
    m_frequencyBar->setStyleSheet("QProgressBar { border: none; background-color: #E5E7EB; border-radius: 4px; height: 8px; } QProgressBar::chunk { background-color: #1677FF; border-radius: 4px; }");
    m_mainLayout->addWidget(m_frequencyBar);

    // 噪音进度条
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
    update();
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

    // 更新进度条
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

    update();
}

void AudioWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    if (!m_waveformWidget) return;

    QPainter painter(m_waveformWidget);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // === 绘制背景 ===
    painter.fillRect(m_waveformWidget->rect(), QColor("#F8FAFC"));

    // === 绘制透明律动条 ===
    int barCount = m_spectrum.size();
    double spacing = 4.0;
    double barWidth = (double)(m_waveformWidget->width() - (barCount + 1) * spacing) / barCount;

    for (int i = 0; i < barCount; ++i) {
        double barHeight = m_spectrum[i] * (m_waveformWidget->height() * 0.85);

        // === 透明律动条（使用半透明微光蓝）===
        QColor barColor(22, 119, 255, 180); // #1677FF with 70% opacity

        painter.setPen(Qt::NoPen);
        painter.setBrush(barColor);
        painter.drawRect(spacing + i * (barWidth + spacing),
                         m_waveformWidget->height() - barHeight,
                         barWidth,
                         barHeight);

        // === 顶部高亮点（更透明）===
        if (barHeight > 4) {
            QColor highlightColor(255, 255, 255, 100); // 白色，40% 透明度
            painter.setBrush(highlightColor);
            painter.drawRect(spacing + i * (barWidth + spacing),
                             m_waveformWidget->height() - barHeight,
                             barWidth,
                             2);
        }
    }
}

// AudioWidget.h
#ifndef AUDIOWIDGET_H
#define AUDIOWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QVector>
#include <QPixmap>
#include <QVBoxLayout>
#include <QProgressBar>
#include <QLabel>
#include <QPainter>

class WaveformWidget : public QWidget
{
    Q_OBJECT
public:
    explicit WaveformWidget(QWidget *parent = nullptr) : QWidget(parent) {
        setMinimumHeight(100);
    }
    
    void setSpectrum(const QVector<qreal> &spectrum) {
        m_spectrum = spectrum;
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        painter.fillRect(rect(), QColor("#F8FAFC"));
        
        int barCount = m_spectrum.size();
        if (barCount == 0) return;
        
        double spacing = 4.0;
        double barWidth = (double)(width() - (barCount + 1) * spacing) / barCount;
        
        for (int i = 0; i < barCount; ++i) {
            double barHeight = m_spectrum[i] * (height() * 0.85);
            QColor barColor(22, 119, 255, 180);
            
            painter.setPen(Qt::NoPen);
            painter.setBrush(barColor);
            painter.drawRect(spacing + i * (barWidth + spacing),
                             height() - barHeight,
                             barWidth,
                             barHeight);
        }
    }
    
private:
    QVector<qreal> m_spectrum;
};

class AudioWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AudioWidget(QWidget *parent = nullptr);
    ~AudioWidget();
    void setSpectrum(const QVector<qreal> &spectrum);
    void setUseRealAudio(bool useReal);

signals:
    void readyForRealAudio();

private slots:
    void onTimeout();

private:
    void initializeRandomSpectrum();
    void setupProgressBars();

    QVector<qreal> m_spectrum;
    QTimer *m_timer;
    bool m_isSimulating;
    QPixmap m_background;
    qreal m_sensitivity;
    
    QVBoxLayout *m_mainLayout;
    WaveformWidget *m_waveformWidget;
    QProgressBar *m_volumeBar;
    QProgressBar *m_frequencyBar;
    QProgressBar *m_noiseBar;
    QLabel *m_volumeLabel;
    QLabel *m_frequencyLabel;
    QLabel *m_noiseLabel;
};

#endif // AUDIOWIDGET_H

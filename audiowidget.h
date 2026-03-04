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

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void initializeRandomSpectrum();
    void setupProgressBars();

    QVector<qreal> m_spectrum;
    QTimer *m_timer;
    bool m_isSimulating;
    QPixmap m_background; // 背景图片
    qreal m_sensitivity;
    
    // 进度条相关
    QVBoxLayout *m_mainLayout;
    QWidget *m_waveformWidget;
    QProgressBar *m_volumeBar;
    QProgressBar *m_frequencyBar;
    QProgressBar *m_noiseBar;
    QLabel *m_volumeLabel;
    QLabel *m_frequencyLabel;
    QLabel *m_noiseLabel;
};

#endif // AUDIOWIDGET_H

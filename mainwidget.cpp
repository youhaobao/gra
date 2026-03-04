#include "mainwidget.h"
#include "ui_mainwidget.h"
#include "led_control.h"
#include "audiowidget.h"
#include "audioanalyzer.h"
#include "videowidget.h"
#include "WeatherClockWidget.h"
#include <QDebug>
#include <QVBoxLayout>
#include <QTime>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkProxy>
#include <QGraphicsDropShadowEffect>

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWidget)
    , m_audioWidget(new AudioWidget(this))
    , m_audioAnalyzer(new AudioAnalyzer(this))
    , m_weatherClockWidget(nullptr)
{
    ui->setupUi(this);
    qDebug() << "SSL 支持状态:" << QSslSocket::supportsSsl();

    // 1. 音频律动
    QVBoxLayout *audioLayout = new QVBoxLayout(ui->audioCard);
    audioLayout->addWidget(m_audioWidget);
    audioLayout->setContentsMargins(16, 16, 16, 16);

    connect(m_audioAnalyzer, &AudioAnalyzer::spectrumReady,
            m_audioWidget, &AudioWidget::setSpectrum);

    if (m_audioAnalyzer->start()) {
        m_audioWidget->setUseRealAudio(true);
        qDebug() << "Real audio spectrum analyzer started";
    }

    // 2. 创建模块化时钟+天气部件
    m_weatherClockWidget = new WeatherClockWidget(this);
    m_weatherClockWidget->setCoordinates(23.02, 113.75);

    if (ui->weatherCard) {
        QVBoxLayout *weatherLayout = new QVBoxLayout(ui->weatherCard);
        weatherLayout->setContentsMargins(16, 16, 16, 16);
        weatherLayout->addWidget(m_weatherClockWidget);
    }

    VideoWidget *myVideo = nullptr;
    if (ui->videoContainer) {
        myVideo = new VideoWidget(ui->videoContainer);
        QVBoxLayout *videoLayout = new QVBoxLayout(ui->videoContainer);
        videoLayout->setContentsMargins(0, 0, 0, 0);
        videoLayout->addWidget(myVideo);
    }

    // 4. 拍照按钮连接
    if (myVideo) {
        QPushButton *captureBtn = ui->captureButton;
        if (captureBtn) {
            connect(captureBtn, &QPushButton::clicked, myVideo, &VideoWidget::takeSnapshot);
        }

        // 截图按钮
        QPushButton *screenshotBtn = ui->screenshotButton;
        if (screenshotBtn) {
            connect(screenshotBtn, &QPushButton::clicked, myVideo, &VideoWidget::takeSnapshot);
        }

        // 文件按钮
        QPushButton *fileBtn = ui->fileButton;
        if (fileBtn) {
            connect(fileBtn, &QPushButton::clicked, []() {
                qDebug() << "文件按钮点击";
            });
        }
    }

    // 2. 📢 关键音视频桥接：把 AudioAnalyzer 截获的声音导入到 VideoWidget
    if (m_audioAnalyzer && myVideo) {
        connect(m_audioAnalyzer, &AudioAnalyzer::rawAudioReady,
                myVideo, &VideoWidget::receiveAudio, Qt::QueuedConnection);
    }

    // 4. 样式表
    QString styleSheet = R"(
/* 全局背景：浅天蓝色 */
QWidget#MainWidget {
    background-color: #E6F2FF;
    color: #333333;
    font-family: "Microsoft YaHei", "Noto Sans CJK", "Inter", sans-serif;
}

/* 卡片样式 */
QWidget#weatherCard, QWidget#audioCard {
    background-color: #FFFFFF;
    border-radius: 12px;
    padding: 16px;
}

/* 按钮样式 */
QPushButton {
    background-color: #FFFFFF;
    color: #333333;
    border: 1px solid #E5E7EB;
    border-radius: 8px;
    font-size: 14px;
    font-weight: 500;
    padding: 10px;
    text-align: center;
}

QPushButton:hover {
    background-color: #F3F4F6;
    border: 1px solid #1677FF;
    color: #1677FF;
}

QPushButton:pressed {
    background-color: #1677FF;
    color: white;
}

/* 视频控制按钮 */
QPushButton#captureButton {
    background-color: #1677FF;
    color: white;
}

QPushButton#captureButton:hover {
    background-color: #3B82F6;
    border: 1px solid #3B82F6;
}

/* 右侧按钮 */
QPushButton#ptzButton {
    border: 2px solid #1677FF;
    color: #1677FF;
    font-weight: 600;
}

QPushButton#ptzButton:hover {
    background-color: #1677FF;
    color: white;
}

/* 进度条样式 */
QProgressBar {
    border: none;
    background-color: #E5E7EB;
    border-radius: 4px;
    height: 8px;
}

QProgressBar::chunk {
    background-color: #1677FF;
    border-radius: 4px;
}
)";

    this->setStyleSheet(styleSheet);

    // 5. 添加阴影效果
    addShadowEffect(ui->weatherCard);
    addShadowEffect(ui->audioCard);
    addShadowEffect(ui->videoContainer);

    led_init();
}


void MainWidget::addShadowEffect(QWidget *widget)
{
    if (widget) {
        QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
        shadow->setBlurRadius(10);
        shadow->setXOffset(0);
        shadow->setYOffset(2);
        shadow->setColor(QColor(0, 0, 0, 50));
        widget->setGraphicsEffect(shadow);
    }
}

MainWidget::~MainWidget()
{
    delete ui;  // 你已有这行
}

// mainwidget.cpp
void MainWidget::on_pushButton_clicked()
{
    static int status = 1;
    if(status)
        qDebug() << "LED clicked on";
    else
        qDebug() << "LED clicked off";

    led_control(status);
    status = !status;
}

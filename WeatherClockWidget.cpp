#include "WeatherClockWidget.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QNetworkRequest>
#include <QNetworkProxy>
#include <QPainter>
#include <QDebug>

WeatherClockWidget::WeatherClockWidget(QWidget *parent)
    : QWidget(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    m_networkManager->setProxy(QNetworkProxy::NoProxy);
    setupUi();

    m_clockTimer = new QTimer(this);
    connect(m_clockTimer, &QTimer::timeout, this, &WeatherClockWidget::updateTime);
    m_clockTimer->start(1000);
    updateTime();

    m_weatherTimer = new QTimer(this);
    connect(m_weatherTimer, &QTimer::timeout, this, &WeatherClockWidget::fetchWeather);
    m_weatherTimer->start(3600 * 1000);
    QTimer::singleShot(2000, this, &WeatherClockWidget::fetchWeather);
}

void WeatherClockWidget::setCoordinates(double lat, double lon) {
    m_lat = lat; m_lon = lon; fetchWeather();
}

void WeatherClockWidget::setupUi() {
    this->setStyleSheet("background: #FFFFFF; border-radius: 12px;");
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 12, 15, 12);

    m_dateLabel = new QLabel(this);
    m_dateLabel->setStyleSheet("color: #999999; font-size: 14px; font-weight: bold;");
    mainLayout->addWidget(m_dateLabel);

    QHBoxLayout *midLayout = new QHBoxLayout();
    QVBoxLayout *leftBox = new QVBoxLayout();
    m_clockLabel = new QLabel(this);
    m_clockLabel->setStyleSheet("font-size: 60px; color: #333333; font-weight: 200;");
    m_detailLabel = new QLabel(this);
    m_detailLabel->setStyleSheet("color: #999999; font-size: 12px;");
    leftBox->addWidget(m_clockLabel);
    leftBox->addWidget(m_detailLabel);

    QVBoxLayout *rightBox = new QVBoxLayout();
    rightBox->setAlignment(Qt::AlignCenter);
    m_bigWeatherIcon = new QLabel(this);
    // 给图标加个浅蓝色圆圈底色，在白色背景上更清晰
    m_bigWeatherIcon->setStyleSheet("background-color: #E6F2FF; border-radius: 40px; padding: 5px;");
    m_bigWeatherIcon->setFixedSize(80, 80);

    m_currentTempLabel = new QLabel(this);
    m_currentTempLabel->setStyleSheet("font-size: 28px; color: #333333; font-weight: bold;");
    m_descLabel = new QLabel(this);
    m_descLabel->setStyleSheet("color: #1677FF; font-size: 13px; font-weight: bold;");
    rightBox->addWidget(m_bigWeatherIcon);
    rightBox->addWidget(m_currentTempLabel);
    rightBox->addWidget(m_descLabel);

    midLayout->addLayout(leftBox);
    midLayout->addStretch();
    midLayout->addLayout(rightBox);
    mainLayout->addLayout(midLayout);

    mainLayout->addSpacing(10);
    m_forecastLayout = new QHBoxLayout();
    mainLayout->addLayout(m_forecastLayout);
}

void WeatherClockWidget::updateTime() {
    QDateTime now = QDateTime::currentDateTime();
    m_dateLabel->setText(now.toString("MMM d, yyyy - ddd").toUpper());
    m_clockLabel->setText(now.toString("hh:mm"));
}

void WeatherClockWidget::fetchWeather() {
    ::system("ip link set dev end0 mtu 1400");
    QString url = QString("https://api.open-meteo.com/v1/forecast?latitude=%1&longitude=%2"
                          "&current=temperature_2m,relative_humidity_2m,weather_code,surface_pressure"
                          "&daily=weather_code,temperature_2m_max,temperature_2m_min&timezone=auto&forecast_days=5")
                      .arg(m_lat).arg(m_lon);

    QNetworkReply *reply = m_networkManager->get(QNetworkRequest(QUrl(url)));
    connect(reply, &QNetworkReply::finished, this, &WeatherClockWidget::onWeatherReply);
}

void WeatherClockWidget::onWeatherReply() {
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (reply && reply->error() == QNetworkReply::NoError) {
        QJsonObject root = QJsonDocument::fromJson(reply->readAll()).object();

        // 1. 解析当前天气 (Current)
        if (root.contains("current")) {
            QJsonObject cur = root["current"].toObject();
            m_currentTempLabel->setText(QString("%1°C").arg(qRound(cur["temperature_2m"].toDouble())));
            m_detailLabel->setText(QString("Humidity: %1%\nPressure: %2 hPa")
                                       .arg(cur["relative_humidity_2m"].toInt()).arg(qRound(cur["surface_pressure"].toDouble())));
            m_descLabel->setText(getWeatherDesc(cur["weather_code"].toInt()));

            // 更新右侧大图标
            m_bigWeatherIcon->setPixmap(getWeatherIcon(cur["weather_code"].toInt(), true));
        }

        // 2. 解析预报数据 (Daily)
        if (root.contains("daily")) {
            // 先清理旧的布局内容
            while (QLayoutItem* item = m_forecastLayout->takeAt(0)) {
                if (item->widget()) item->widget()->deleteLater();
                delete item;
            }

            QJsonObject daily = root["daily"].toObject();
            // --- 关键修正：必须先提取数组变量 ---
            QJsonArray times = daily["time"].toArray();
            QJsonArray codesArr = daily["weather_code"].toArray();
            QJsonArray maxsArr = daily["temperature_2m_max"].toArray();
            QJsonArray minsArr = daily["temperature_2m_min"].toArray();

            for (int i = 0; i < 5; ++i) {
                QDate d = QDate::fromString(times[i].toString(), "yyyy-MM-dd");
                QString dayStr = (i == 0) ? "TODAY" : d.toString("ddd").toUpper();

                // 使用提取出来的数组变量 Arr
                m_forecastLayout->addWidget(createForecastItem(
                    dayStr,
                    codesArr[i].toInt(),
                    maxsArr[i].toDouble(),
                    minsArr[i].toDouble()
                    ));
            }
        }
    }
    if(reply) reply->deleteLater();
}

QWidget* WeatherClockWidget::createForecastItem(QString day, int code, double max, double min) {
    QWidget *w = new QWidget();
    QVBoxLayout *l = new QVBoxLayout(w);
    l->setContentsMargins(0,0,0,0);
    l->setSpacing(4);

    QLabel *lDay = new QLabel(day);
    lDay->setAlignment(Qt::AlignCenter);
    lDay->setStyleSheet("font-size: 10px; color: #999999;");

    QLabel *lIcon = new QLabel(); // 确保变量名是 lIcon
    lIcon->setPixmap(getWeatherIcon(code, false));
    lIcon->setAlignment(Qt::AlignCenter);
    // 给图标加个浅蓝色底色，在白色背景上更清晰
    lIcon->setStyleSheet("background-color: #E6F2FF; border-radius: 14px; padding: 2px;");

    QLabel *t = new QLabel(QString("%1/%2°").arg(qRound(max)).arg(qRound(min)));
    t->setAlignment(Qt::AlignCenter);
    t->setStyleSheet("font-size: 10px; color: #333333;");

    l->addWidget(lDay);
    l->addWidget(lIcon);
    l->addWidget(t);
    return w;
}

QString WeatherClockWidget::getWeatherDesc(int code) {
    if (code == 0) return "Sunny";
    if (code == 1) return "Mainly Clear";
    if (code == 2) return "Partly Cloudy";
    if (code == 3) return "Overcast";
    if (code >= 51 && code <= 65) return "Rainy";
    if (code >= 71 && code <= 77) return "Snowy";
    return "Cloudy";
}

// 核心：基于你提供的 8 个图标进行映射
QPixmap WeatherClockWidget::getWeatherIcon(int code, bool big) {
    QString iconName;

    // 1. 映射逻辑（添加了括号以消除编译警告）
    if (code == 0 || code == 1) {
        iconName = "sunny.png";
    } else if (code == 2 || code == 3) {
        iconName = "cloudy.png";
    } else if (code == 45 || code == 48) {
        iconName = "fog.png";
    } else if ((code >= 51 && code <= 61) || code == 80) {
        iconName = "light_rain.png";
    } else if (code == 63 || code == 81) {
        iconName = "moderate_rain.png";
    } else if (code == 55 || code == 65 || code == 82 || (code >= 66 && code <= 67)) {
        iconName = "heavy_rain.png";
    } else if ((code >= 71 && code <= 77) || (code >= 85 && code <= 86)) { // 修复了这里的警告
        iconName = "snow.png";
    } else if (code >= 95) {
        iconName = "thunder.png";
    } else {
        iconName = "cloudy.png";
    }

    // 2. 使用你的校准路径
    QString fullPath = ":/new/prefix1/images/weather/" + iconName;
    QPixmap pix(fullPath);

    int s = big ? 70 : 28;

    // 3. 核心修复：QPixmap(s, s) 必须传入两个参数
    if (pix.isNull()) {
        qDebug() << "❌ 资源加载失败:" << fullPath;
        QPixmap emptyPix(s, s);
        emptyPix.fill(Qt::transparent); // 返回一个透明底，防止界面难看
        return emptyPix;
    }

    return pix.scaled(s, s, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

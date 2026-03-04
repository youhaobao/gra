#ifndef WEATHERCLOCKWIDGET_H
#define WEATHERCLOCKWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QVector>

class WeatherClockWidget : public QWidget
{
    Q_OBJECT
public:
    explicit WeatherClockWidget(QWidget *parent = nullptr);
    void setCoordinates(double lat, double lon);

private slots:
    void updateTime();
    void fetchWeather();
    void onWeatherReply(); // 修改签名，移除参数以匹配实现

private:
    void setupUi();
    QString getWeatherDesc(int code);
    QPixmap getWeatherIcon(int code, bool big = false);
    QWidget* createForecastItem(QString day, int code, double max, double min);

    // UI 组件
    QLabel *m_dateLabel;
    QLabel *m_clockLabel;
    QLabel *m_detailLabel;
    QLabel *m_bigWeatherIcon;
    QLabel *m_currentTempLabel;
    QLabel *m_descLabel;
    QHBoxLayout *m_forecastLayout;

    QTimer *m_clockTimer;
    QTimer *m_weatherTimer;
    QNetworkAccessManager *m_networkManager;

    double m_lat = 23.02; // 默认东莞
    double m_lon = 113.75;
};

#endif

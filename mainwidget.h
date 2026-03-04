#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>
#include "WeatherClockWidget.h"

// 前置声明
class AudioWidget;
class AudioAnalyzer;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWidget; }
QT_END_NAMESPACE

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    MainWidget(QWidget *parent = nullptr);
    ~MainWidget();

private slots:
    void on_pushButton_clicked();

private:
    Ui::MainWidget *ui;

    AudioWidget *m_audioWidget;
    AudioAnalyzer *m_audioAnalyzer;
    WeatherClockWidget *m_weatherClockWidget;

    void addShadowEffect(QWidget *widget);
};

#endif // MAINWIDGET_H

// TemperatureChartWidget.h
#ifndef TEMPERATURECHARTWIDGET_H
#define TEMPERATURECHARTWIDGET_H

#include <QWidget>
#include <QVector>

class TemperatureChartWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TemperatureChartWidget(QWidget *parent = nullptr);
    void setTemperatureData(const QVector<double> &data);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<double> m_data;
};

#endif // TEMPERATURECHARTWIDGET_H

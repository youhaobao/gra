// temperaturechartwidget.cpp
#include "temperaturechartwidget.h"
#include <QPainter>
#include <QPen>
#include <algorithm>
#include <QBrush>

TemperatureChartWidget::TemperatureChartWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(120, 40);
    setStyleSheet("background: transparent;");
}

void TemperatureChartWidget::setTemperatureData(const QVector<double> &data)
{
    m_data = data;
    update();
}

void TemperatureChartWidget::paintEvent(QPaintEvent *)
{
    if (m_data.isEmpty()) return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int width = this->width();
    int height = this->height();

    // 找到温度范围
    double minTemp = *std::min_element(m_data.constBegin(), m_data.constEnd());
    double maxTemp = *std::max_element(m_data.constBegin(), m_data.constEnd());
    if (maxTemp == minTemp) maxTemp = minTemp + 1;

    // 绘制网格线（浅灰色）
    QPen gridPen(QColor(100, 100, 100), 1, Qt::DotLine);
    painter.setPen(gridPen);
    for (int i = 1; i < 4; ++i) {
        int y = i * height / 4;
        painter.drawLine(0, y, width, y);
    }

    // 绘制温度曲线（白色）
    QPen curvePen(Qt::white, 2);
    painter.setPen(curvePen);
    painter.setBrush(Qt::NoBrush);

    // 计算点
    QVector<QPointF> points;
    int numPoints = qMin(m_data.size(), 12); // 最多显示12小时
    for (int i = 0; i < numPoints; ++i) {
        double x = i * (width - 1) / (numPoints - 1.0);
        double normalizedY = (m_data[i] - minTemp) / (maxTemp - minTemp);
        double y = height - normalizedY * (height - 10) - 5; // 留出边距
        points.append(QPointF(x, y));
    }

    // 绘制折线
    for (int i = 0; i < points.size() - 1; ++i) {
        painter.drawLine(points[i], points[i + 1]);
    }

    // 绘制数据点
    painter.setBrush(Qt::white);
    for (const QPointF &point : points) {
        painter.drawEllipse(point, 2, 2);
    }
}

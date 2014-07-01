/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt Enterprise Charts Add-on.
**
** $QT_BEGIN_LICENSE$
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com
** $QT_END_LICENSE$
**
****************************************************************************/

#include "percentbarchartitem_p.h"
#include "bar_p.h"
#include "qabstractbarseries_p.h"
#include "qbarset.h"
#include "qbarset_p.h"

QT_CHARTS_BEGIN_NAMESPACE

PercentBarChartItem::PercentBarChartItem(QAbstractBarSeries *series, QGraphicsItem* item) :
    AbstractBarChartItem(series, item)
{
    connect(series, SIGNAL(labelsPositionChanged(QAbstractBarSeries::LabelsPosition)),
            this, SLOT(handleLabelsPositionChanged()));
    connect(series, SIGNAL(labelsFormatChanged(QString)), this, SLOT(positionLabels()));
}

void PercentBarChartItem::initializeLayout()
{
    qreal categoryCount = m_series->d_func()->categoryCount();
    qreal setCount = m_series->count();
    qreal barWidth = m_series->d_func()->barWidth();

    m_layout.clear();
    for(int category = 0; category < categoryCount; category++) {
        for (int set = 0; set < setCount; set++) {
            QRectF rect;
            QPointF topLeft;
            QPointF bottomRight;

            if (domain()->type() == AbstractDomain::XLogYDomain || domain()->type() == AbstractDomain::LogXLogYDomain) {
                topLeft = domain()->calculateGeometryPoint(QPointF(category - barWidth / 2, domain()->minY()), m_validData);
                bottomRight = domain()->calculateGeometryPoint(QPointF(category + barWidth / 2, domain()->minY()), m_validData);
            } else {
                topLeft = domain()->calculateGeometryPoint(QPointF(category - barWidth / 2, 0), m_validData);
                bottomRight = domain()->calculateGeometryPoint(QPointF(category + barWidth / 2, 0), m_validData);
            }

            if (!m_validData)
                 return;

            rect.setTopLeft(topLeft);
            rect.setBottomRight(bottomRight);
            m_layout.append(rect.normalized());
        }
    }
}

QVector<QRectF> PercentBarChartItem::calculateLayout()
{
    QVector<QRectF> layout;

    // Use temporary qreals for accuracy
    qreal categoryCount = m_series->d_func()->categoryCount();
    qreal setCount = m_series->count();
    qreal barWidth = m_series->d_func()->barWidth();

    for(int category = 0; category < categoryCount; category++) {
        qreal sum = 0;
        qreal categorySum = m_series->d_func()->categorySum(category);
        for (int set = 0; set < setCount; set++) {
            qreal value = m_series->barSets().at(set)->at(category);
            QRectF rect;
            qreal topY = 0;
            qreal newSum = value + sum;
            if (newSum > 0)
                topY = 100 * newSum / categorySum;
            qreal bottomY = 0;
            if (sum > 0)
                bottomY = 100 * sum / categorySum;
            QPointF topLeft = domain()->calculateGeometryPoint(QPointF(category - barWidth/2, topY), m_validData);
            QPointF bottomRight;
            if (domain()->type() == AbstractDomain::XLogYDomain || domain()->type() == AbstractDomain::LogXLogYDomain)
                bottomRight = domain()->calculateGeometryPoint(QPointF(category + barWidth/2, set ? bottomY : domain()->minY()), m_validData);
            else
                bottomRight = domain()->calculateGeometryPoint(QPointF(category + barWidth/2, set ? bottomY : 0), m_validData);

            rect.setTopLeft(topLeft);
            rect.setBottomRight(bottomRight);
            layout.append(rect.normalized());
            sum = newSum;
        }
    }
    return layout;
}

void PercentBarChartItem::handleUpdatedBars()
{
    // Handle changes in pen, brush, labels etc.
    int categoryCount = m_series->d_func()->categoryCount();
    int setCount = m_series->count();
    int itemIndex(0);
    static const QString valueTag(QLatin1String("@value"));

    for (int category = 0; category < categoryCount; category++) {
        for (int set = 0; set < setCount; set++) {
            QBarSetPrivate *barSet = m_series->d_func()->barsetAt(set)->d_ptr.data();
            Bar *bar = m_bars.at(itemIndex);
            bar->setPen(barSet->m_pen);
            bar->setBrush(barSet->m_brush);
            bar->update();

            QGraphicsTextItem *label = m_labels.at(itemIndex);
            qreal p = m_series->d_func()->percentageAt(set, category) * 100.0;
            QString vString(presenter()->numberToString(p, 'f', 0));
            QString valueLabel;
            if (m_series->labelsFormat().isEmpty()) {
                vString.append(QStringLiteral("%"));
                valueLabel = vString;
            } else {
                valueLabel = m_series->labelsFormat();
                valueLabel.replace(valueTag, vString);
            }
            label->setHtml(valueLabel);
            label->setFont(barSet->m_labelFont);
            label->setDefaultTextColor(barSet->m_labelBrush.color());
            label->update();
            itemIndex++;
        }
    }
}

void PercentBarChartItem::handleLabelsPositionChanged()
{
    positionLabels();
}

void PercentBarChartItem::positionLabels()
{
    for (int i = 0; i < m_layout.count(); i++) {
        QGraphicsTextItem *label = m_labels.at(i);
        qreal xPos = m_layout.at(i).center().x() - label->boundingRect().center().x();
        qreal yPos = 0;

        int offset = m_bars.at(i)->pen().width() / 2 + 2;
        if (m_series->labelsPosition() == QAbstractBarSeries::LabelsCenter)
            yPos = m_layout.at(i).center().y() - label->boundingRect().center().y();
        else if (m_series->labelsPosition() == QAbstractBarSeries::LabelsInsideEnd)
            yPos = m_layout.at(i).top() - offset;
        else if (m_series->labelsPosition() == QAbstractBarSeries::LabelsInsideBase)
            yPos = m_layout.at(i).bottom() - label->boundingRect().height() + offset;
        else if (m_series->labelsPosition() == QAbstractBarSeries::LabelsOutsideEnd)
            yPos = m_layout.at(i).top() - label->boundingRect().height() + offset;

        label->setPos(xPos, yPos);
        label->setZValue(zValue() + 1);
    }
}

#include "moc_percentbarchartitem_p.cpp"

QT_CHARTS_END_NAMESPACE
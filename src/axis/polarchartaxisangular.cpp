/****************************************************************************
**
** Copyright (C) 2013 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt Commercial Charts Add-on.
**
** $QT_BEGIN_LICENSE$
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com
** $QT_END_LICENSE$
**
****************************************************************************/

#include "polarchartaxisangular_p.h"
#include "chartpresenter_p.h"
#include "abstractchartlayout_p.h"
#include "qabstractaxis.h"
#include "qabstractaxis_p.h"
#include <QFontMetrics>
#include <QDebug>

QTCOMMERCIALCHART_BEGIN_NAMESPACE

PolarChartAxisAngular::PolarChartAxisAngular(QAbstractAxis *axis, QGraphicsItem *item, bool intervalAxis)
    : PolarChartAxis(axis, item, intervalAxis)
{
}

PolarChartAxisAngular::~PolarChartAxisAngular()
{
}

void PolarChartAxisAngular::updateGeometry()
{
    QGraphicsLayoutItem::updateGeometry();

    const QVector<qreal> &layout = this->layout();
    if (layout.isEmpty())
        return;

    createAxisLabels(layout);
    QStringList labelList = labels();
    QPointF center = axisGeometry().center();
    QList<QGraphicsItem *> arrowItemList = arrowItems();
    QList<QGraphicsItem *> gridItemList = gridItems();
    QList<QGraphicsItem *> labelItemList = labelItems();
    QList<QGraphicsItem *> shadeItemList = shadeItems();
    QGraphicsSimpleTextItem *title = titleItem();

    QGraphicsEllipseItem *axisLine = static_cast<QGraphicsEllipseItem *>(arrowItemList.at(0));
    axisLine->setRect(axisGeometry());

    qreal radius = axisGeometry().height() / 2.0;

    QFontMetrics fn(axis()->labelsFont());
    QRectF previousLabelRect;
    QRectF firstLabelRect;

    qreal labelHeight = 0;

    bool firstShade = true;
    bool nextTickVisible = false;
    if (layout.size())
        nextTickVisible = !(layout.at(0) < 0.0 || layout.at(0) > 360.0);

    for (int i = 0; i < layout.size(); ++i) {
        qreal angularCoordinate = layout.at(i);

        QGraphicsLineItem *gridLineItem = static_cast<QGraphicsLineItem *>(gridItemList.at(i));
        QGraphicsLineItem *tickItem = static_cast<QGraphicsLineItem *>(arrowItemList.at(i + 1));
        QGraphicsSimpleTextItem *labelItem = static_cast<QGraphicsSimpleTextItem *>(labelItemList.at(i));
        QGraphicsPathItem *shadeItem = 0;
        if (i == 0)
            shadeItem = static_cast<QGraphicsPathItem *>(shadeItemList.at(0));
        else if (i % 2)
            shadeItem = static_cast<QGraphicsPathItem *>(shadeItemList.at((i / 2) + 1));

        // Ignore ticks outside valid range
        bool currentTickVisible = nextTickVisible;
        if ((i == layout.size() - 1)
            || layout.at(i + 1) < 0.0
            || layout.at(i + 1) > 360.0) {
            nextTickVisible = false;
        } else {
            nextTickVisible = true;
        }

        qreal labelCoordinate = angularCoordinate;
        qreal labelVisible = currentTickVisible;
        if (intervalAxis()) {
            qreal farEdge;
            if (i == (layout.size() - 1))
                farEdge = 360.0;
            else
                farEdge = qMin(360.0, layout.at(i + 1));

            // Adjust the labelCoordinate to show it if next tick is visible
            if (nextTickVisible)
                labelCoordinate = qMax(0.0, labelCoordinate);

            labelCoordinate = (labelCoordinate + farEdge) / 2.0;
            // Don't display label once the category gets too small near the axis
            if (labelCoordinate < 5.0 || labelCoordinate > 355.0)
                labelVisible = false;
            else
                labelVisible = true;
        }

        // Need this also in label calculations, so determine it first
        QLineF tickLine(QLineF::fromPolar(radius - tickWidth(), 90.0 - angularCoordinate).p2(),
                        QLineF::fromPolar(radius + tickWidth(), 90.0 - angularCoordinate).p2());
        tickLine.translate(center);

        // Angular axis label
        if (axis()->labelsVisible() && labelVisible) {
            labelItem->setText(labelList.at(i));
            const QRectF &rect = labelItem->boundingRect();
            QPointF labelCenter = rect.center();
            labelItem->setTransformOriginPoint(labelCenter.x(), labelCenter.y());
            QRectF boundingRect = labelBoundingRect(fn, labelList.at(i));
            boundingRect.moveCenter(labelCenter);
            QPointF positionDiff(rect.topLeft() - boundingRect.topLeft());

            QPointF labelPoint;
            if (intervalAxis()) {
                QLineF labelLine = QLineF::fromPolar(radius + tickWidth(), 90.0 - labelCoordinate);
                labelLine.translate(center);
                labelPoint = labelLine.p2();
            } else {
                labelPoint = tickLine.p2();
            }

            QRectF labelRect = moveLabelToPosition(labelCoordinate, labelPoint, boundingRect);
            labelItem->setPos(labelRect.topLeft() + positionDiff);

            // Store height for title calculations
            qreal labelClearance = axisGeometry().top() - labelRect.top();
            labelHeight = qMax(labelHeight, labelClearance);

            // Label overlap detection
            if (i && (previousLabelRect.intersects(labelRect) || firstLabelRect.intersects(labelRect))) {
                labelVisible = false;
            } else {
                // Store labelRect for future comparison. Some area is deducted to make things look
                // little nicer, as usually intersection happens at label corner with angular labels.
                labelRect.adjust(-2.0, -4.0, -2.0, -4.0);
                if (firstLabelRect.isEmpty())
                    firstLabelRect = labelRect;

                previousLabelRect = labelRect;
                labelVisible = true;
            }
        }

        labelItem->setVisible(labelVisible);
        if (!currentTickVisible) {
            gridLineItem->setVisible(false);
            tickItem->setVisible(false);
            if (shadeItem)
                shadeItem->setVisible(false);
            continue;
        }

        // Angular grid line
        QLineF gridLine = QLineF::fromPolar(radius, 90.0 - angularCoordinate);
        gridLine.translate(center);
        gridLineItem->setLine(gridLine);
        gridLineItem->setVisible(true);

        // Tick
        tickItem->setLine(tickLine);
        tickItem->setVisible(true);

        // Shades
        if (i % 2 || (i == 0 && !nextTickVisible)) {
            QPainterPath path;
            path.moveTo(center);
            if (i == 0) {
                // If first tick is also the last, we need to custom fill the first partial arc
                // or it won't get filled.
                path.arcTo(axisGeometry(), 90.0 - layout.at(0), layout.at(0));
                path.closeSubpath();
            } else {
                qreal nextCoordinate;
                if (!nextTickVisible) // Last visible tick
                    nextCoordinate = 360.0;
                else
                    nextCoordinate = layout.at(i + 1);
                qreal arcSpan = angularCoordinate - nextCoordinate;
                path.arcTo(axisGeometry(), 90.0 - angularCoordinate, arcSpan);
                path.closeSubpath();

                // Add additional arc for first shade item if there is a partial arc to be filled
                if (firstShade) {
                    QGraphicsPathItem *specialShadeItem = static_cast<QGraphicsPathItem *>(shadeItemList.at(0));
                    if (layout.at(i - 1) > 0.0) {
                        QPainterPath specialPath;
                        specialPath.moveTo(center);
                        specialPath.arcTo(axisGeometry(), 90.0 - layout.at(i - 1), layout.at(i - 1));
                        specialPath.closeSubpath();
                        specialShadeItem->setPath(specialPath);
                        specialShadeItem->setVisible(true);
                    } else {
                        specialShadeItem->setVisible(false);
                    }
                }
            }
            shadeItem->setPath(path);
            shadeItem->setVisible(true);
            firstShade = false;
        }
    }

    // Title, centered above the chart
    QString titleText = axis()->titleText();
    if (!titleText.isEmpty() && axis()->isTitleVisible()) {
        int size(0);
        size = axisGeometry().width();

        QFontMetrics titleMetrics(axis()->titleFont());
        if (titleMetrics.boundingRect(titleText).width() > size) {
            QString string = titleText + "...";
            while (titleMetrics.boundingRect(string).width() > size && string.length() > 3)
                string.remove(string.length() - 4, 1);
            title->setText(string);
        } else {
            title->setText(titleText);
        }

        QRectF titleBoundingRect;
        titleBoundingRect = title->boundingRect();
        QPointF titleCenter = center - titleBoundingRect.center();
        title->setPos(titleCenter.x(), axisGeometry().top() - qreal(titlePadding()) * 2.0 - titleBoundingRect.height() - labelHeight);
    }
}

Qt::Orientation PolarChartAxisAngular::orientation() const
{
    return Qt::Horizontal;
}

void PolarChartAxisAngular::createItems(int count)
{
    if (arrowItems().count() == 0) {
        // angular axis line
        // TODO: need class similar to LineArrowItem for click handling?
        QGraphicsEllipseItem *arrow = new QGraphicsEllipseItem(presenter()->rootItem());
        arrow->setPen(axis()->linePen());
        arrowGroup()->addToGroup(arrow);
    }

    for (int i = 0; i < count; ++i) {
        QGraphicsLineItem *arrow = new QGraphicsLineItem(presenter()->rootItem());
        QGraphicsLineItem *grid = new QGraphicsLineItem(presenter()->rootItem());
        QGraphicsSimpleTextItem *label = new QGraphicsSimpleTextItem(presenter()->rootItem());
        QGraphicsSimpleTextItem *title = titleItem();
        arrow->setPen(axis()->linePen());
        grid->setPen(axis()->gridLinePen());
        label->setFont(axis()->labelsFont());
        label->setPen(axis()->labelsPen());
        label->setBrush(axis()->labelsBrush());
        label->setRotation(axis()->labelsAngle());
        title->setFont(axis()->titleFont());
        title->setPen(axis()->titlePen());
        title->setBrush(axis()->titleBrush());
        title->setText(axis()->titleText());
        arrowGroup()->addToGroup(arrow);
        gridGroup()->addToGroup(grid);
        labelGroup()->addToGroup(label);
        if (gridItems().size() == 1 || (((gridItems().size() + 1) % 2) && gridItems().size() > 0)) {
            QGraphicsPathItem *shade = new QGraphicsPathItem(presenter()->rootItem());
            shade->setPen(axis()->shadesPen());
            shade->setBrush(axis()->shadesBrush());
            shadeGroup()->addToGroup(shade);
        }
    }
}

void PolarChartAxisAngular::handleArrowPenChanged(const QPen &pen)
{
    bool first = true;
    foreach (QGraphicsItem *item, arrowItems()) {
        if (first) {
            first = false;
            // First arrow item is the outer circle of axis
            static_cast<QGraphicsEllipseItem *>(item)->setPen(pen);
        } else {
            static_cast<QGraphicsLineItem *>(item)->setPen(pen);
        }
    }
}

void PolarChartAxisAngular::handleGridPenChanged(const QPen &pen)
{
    foreach (QGraphicsItem *item, gridItems())
        static_cast<QGraphicsLineItem *>(item)->setPen(pen);
}

QSizeF PolarChartAxisAngular::sizeHint(Qt::SizeHint which, const QSizeF &constraint) const
{
    Q_UNUSED(which);
    Q_UNUSED(constraint);
    return QSizeF(-1, -1);
}

qreal PolarChartAxisAngular::preferredAxisRadius(const QSizeF &maxSize)
{
    qreal radius = maxSize.height() / 2.0;
    if (maxSize.width() < maxSize.height())
        radius = maxSize.width() / 2.0;

    int labelHeight = 0;

    if (axis()->labelsVisible()) {
        QVector<qreal> layout = calculateLayout();
        if (layout.isEmpty())
            return radius;

        createAxisLabels(layout);
        QStringList labelList = labels();
        QFont font = axis()->labelsFont();

        QRectF maxRect;
        maxRect.setSize(maxSize);
        maxRect.moveCenter(QPointF(0.0, 0.0));

        // This is a horrible way to find out the maximum radius for angular axis and its labels.
        // It just increments the radius down until everyhing fits the constraint size.
        // Proper way would be to actually calculate it but this seems to work reasonably fast as it is.
        QFontMetrics fm(font);
        bool nextTickVisible = false;
        for (int i = 0; i < layout.size(); ) {
            if ((i == layout.size() - 1)
                || layout.at(i + 1) < 0.0
                || layout.at(i + 1) > 360.0) {
                nextTickVisible = false;
            } else {
                nextTickVisible = true;
            }

            qreal labelCoordinate = layout.at(i);
            qreal labelVisible;

            if (intervalAxis()) {
                qreal farEdge;
                if (i == (layout.size() - 1))
                    farEdge = 360.0;
                else
                    farEdge = qMin(360.0, layout.at(i + 1));

                // Adjust the labelCoordinate to show it if next tick is visible
                if (nextTickVisible)
                    labelCoordinate = qMax(0.0, labelCoordinate);

                labelCoordinate = (labelCoordinate + farEdge) / 2.0;
            }

            if (labelCoordinate < 0.0 || labelCoordinate > 360.0)
                labelVisible = false;
            else
                labelVisible = true;

            if (!labelVisible) {
                i++;
                continue;
            }

            QRectF boundingRect = labelBoundingRect(fm, labelList.at(i));
            labelHeight = boundingRect.height();
            QPointF labelPoint = QLineF::fromPolar(radius + tickWidth(), 90.0 - labelCoordinate).p2();

            boundingRect = moveLabelToPosition(labelCoordinate, labelPoint, boundingRect);
            if (boundingRect.isEmpty() || maxRect.intersected(boundingRect) == boundingRect) {
                i++;
            } else {
                radius -= 1.0;
                if (radius < 1.0) // safeguard
                    return 1.0;
            }
        }
    }

    if (!axis()->titleText().isEmpty() && axis()->isTitleVisible()) {
        QFontMetrics titleMetrics(axis()->titleFont());
        int titleHeight = titleMetrics.boundingRect(axis()->titleText()).height();
        radius -= titlePadding() + (titleHeight / 2);
        if (radius < 1.0) // safeguard
            return 1.0;
    }

    return radius;
}

QRectF PolarChartAxisAngular::moveLabelToPosition(qreal angularCoordinate, QPointF labelPoint, QRectF labelRect) const
{
    // TODO use fuzzy compare for "==" cases?
    // TODO Adjust the rect position near 0, 90, 180, and 270 angles for smoother animation?
    if (angularCoordinate == 0.0)
        labelRect.moveCenter(labelPoint + QPointF(0, -labelRect.height() / 2.0));
    else if (angularCoordinate < 90.0)
        labelRect.moveBottomLeft(labelPoint);
    else if (angularCoordinate == 90.0)
        labelRect.moveCenter(labelPoint + QPointF(labelRect.width() / 2.0 + 2.0, 0)); // +2 so that it does not hit the radial axis
    else if (angularCoordinate < 180.0)
        labelRect.moveTopLeft(labelPoint);
    else if (angularCoordinate == 180.0)
        labelRect.moveCenter(labelPoint + QPointF(0, labelRect.height() / 2.0));
    else if (angularCoordinate < 270.0)
        labelRect.moveTopRight(labelPoint);
    else if (angularCoordinate == 270.0)
        labelRect.moveCenter(labelPoint + QPointF(-labelRect.width() / 2.0 - 2.0, 0)); // -2 so that it does not hit the radial axis
    else if (angularCoordinate < 360.0)
        labelRect.moveBottomRight(labelPoint);
    else
        labelRect.moveCenter(labelPoint + QPointF(0, -labelRect.height() / 2.0));
    return labelRect;
}

#include "moc_polarchartaxisangular_p.cpp"

QTCOMMERCIALCHART_END_NAMESPACE
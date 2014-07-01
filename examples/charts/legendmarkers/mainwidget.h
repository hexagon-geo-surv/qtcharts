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

#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include "qchartglobal.h"
#include "qchart.h"
#include "qchartview.h"
#include <QWidget>
#include <QGraphicsWidget>
#include <QGridLayout>
#include <QGraphicsGridLayout>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QLineSeries>

QT_CHARTS_USE_NAMESPACE

class MainWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MainWidget(QWidget *parent = 0);

public slots:
    void addSeries();
    void removeSeries();
    void connectMarkers();
    void disconnectMarkers();

    void handleMarkerClicked();

private:

    QChart *m_chart;
    QList<QLineSeries *> m_series;

    QChartView *m_chartView;
    QGridLayout *m_mainLayout;
    QGridLayout *m_fontLayout;

};

#endif // MAINWIDGET_H
// Copyright (c) 2013-2014 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#pragma once
#include <Pothos/Config.hpp>
#include <QSizeF>
#include <QString>

static const QString GraphObjectHighlightPenColor = "#0040FF";
static const QString GraphObjectDefaultPenColor = "#000000";
static const QString GraphObjectDefaultFillColor = "#E8E8E8";
static const qreal GraphObjectBorderWidth = 0.5;

static const QString GraphObjectConnPointColor = "#FF85C2";
static const QString GraphObjectConnLineColor = "#000000";
static const qreal GraphObjectConnPointRadius = 3;
static const qreal GraphObjectConnLineLength = 10;

static const QString GraphDrawHighlightColor = "#6464C8";
static const qreal GraphDrawHighlightAlpha = 100;
static const qreal GraphDrawScrollFudge = 20;
static const QString GraphDrawBackgroundColor = "#FCFFFF";
static const QSizeF GraphDrawCanvasSize(1920, 1080);
static const qreal GraphDrawZoomStep = 0.1;
static const qreal GraphDrawZoomMax = 1.5;
static const qreal GraphDrawZoomMin = 0.5;

static const qreal GraphBlockPortTextHPad = 1.5;
static const qreal GraphBlockPortTextVPad = 1.5;
static const qreal GraphBlockPropTextHPad = 5;
static const qreal GraphBlockPropTextVPad = 3;
static const qreal GraphBlockPortVGap = 10;
static const qreal GraphBlockPortVOutterPad = 10;
static const qreal GraphBlockTitleVPad = 5;
static const qreal GraphBlockTitleHPad = 5;
static const qreal GraphBlockPortArc = 3;
static const qreal GraphBlockMainArc = 5;
static const QString GraphBlockTitleFontSize = "9pt";
static const QString GraphBlockPropFontSize = "8pt";
static const QString GraphBlockPortFontSize = "8pt";

static const qreal GraphBreakerTitleVPad = 1.5;
static const qreal GraphBreakerTitleHPad = 1.5;
static const qreal GraphBreakerEdgeJut = 2.5;
static const QString GraphBreakerTitleFontSize = "8pt";

static const qreal GraphConnectionMinPling = 10;
static const qreal GraphConnectionGirth = 1.5;
static const qreal GraphConnectionArrowLen = 10;
static const qreal GraphConnectionArrowAngle = 30;
static const qreal GraphConnectionMaxCurve = 10;
static const qreal GraphConnectionSelectPad = 3;
static const QString GraphConnectionDefaultColor = "#000000";
static const QString GraphConnectionHighlightColor = "#0040FF";

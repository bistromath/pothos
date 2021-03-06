// Copyright (c) 2013-2014 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#pragma once
#include "GraphObjects/GraphBlock.hpp"
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Array.h>
#include <QRectF>
#include <QPointF>
#include <QStaticText>
#include <vector>
#include <map>

struct GraphBlock::Impl
{
    Impl(void):
        changed(true)
    {
        return;
    }

    Poco::JSON::Object::Ptr blockDesc;

    bool changed;

    QString title;
    QStaticText titleText;

    std::vector<QStaticText> propertiesText;
    std::map<QString, QString> propertiesValues;
    std::map<QString, bool> propertiesPreview;

    std::vector<QStaticText> inputPortsText;
    std::vector<QRectF> inputPortRects;
    std::vector<QPointF> inputPortPoints;

    std::vector<QStaticText> outputPortsText;
    std::vector<QRectF> outputPortRects;
    std::vector<QPointF> outputPortPoints;

    QRectF mainBlockRect;
};

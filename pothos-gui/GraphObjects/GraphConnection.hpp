// Copyright (c) 2013-2014 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#pragma once
#include <Pothos/Config.hpp>
#include "GraphObjects/GraphObject.hpp"
#include <QObject>
#include <QString>
#include <QPointF>
#include <memory>

class GraphConnection : public GraphObject
{
    Q_OBJECT
public:
    GraphConnection(QObject *parent);

    void setupEndpoint(const GraphConnectionEndpoint &ep);

    const GraphConnectionEndpoint &getOutputEndpoint(void) const;
    const GraphConnectionEndpoint &getInputEndpoint(void) const;

    bool isPointing(const QRectF &rect) const;

    void render(QPainter &painter);

    Poco::JSON::Object::Ptr serialize(void) const;

    virtual void deserialize(Poco::JSON::Object::Ptr obj);

private slots:
    void handleEndPointDestroyed(QObject *obj);

private:
    struct Impl;
    std::shared_ptr<Impl> _impl;
};

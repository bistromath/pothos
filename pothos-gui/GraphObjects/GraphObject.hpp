// Copyright (c) 2013-2014 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#pragma once
#include "GraphObjects/GraphEndpoint.hpp"
#include <QList>
#include <QObject>
#include <QPointF>
#include <QRectF>
#include <vector>
#include <memory>
#include <Poco/JSON/Array.h>
#include <Poco/JSON/Object.h>

class QPainter;
class GraphObject;

//! Represent a list of graph objects
typedef QList<GraphObject *> GraphObjectList;

//! Base class for graph objects
class GraphObject : public QObject
{
public:
    GraphObject(QObject *parent);

    ~GraphObject(void);

    virtual void setId(const QString &id);
    const QString &getId(void) const;

    virtual void setSelected(const bool on);
    bool getSelected(void) const;

    virtual bool isPointing(const QPointF &pos) const;
    virtual bool isPointing(const QRectF &pos) const;

    virtual void render(QPainter &painter);

    virtual void setZIndex(const int index);
    int getZIndex(void) const;

    virtual void setPosition(const QPointF &pos);
    const QPointF &getPosition(void) const;
    virtual void move(const QPointF &delta);

    virtual void setRotation(const int degree);
    int getRotation(void) const;
    void rotateLeft(void);
    void rotateRight(void);

    //! empty string when not pointing, otherwise connectable key
    virtual std::vector<GraphConnectableKey> getConnectableKeys(void) const;
    virtual GraphConnectableKey isPointingToConnectable(const QPointF &pos) const;
    virtual GraphConnectableAttrs getConnectableAttrs(const GraphConnectableKey &key) const;
    virtual void renderConnectablePoints(QPainter &painter);

    bool isFlaggedForDelete(void) const;
    void flagForDelete(void);

    virtual Poco::JSON::Object::Ptr serialize(void) const;

    virtual void deserialize(Poco::JSON::Object::Ptr obj);

private:
    struct Impl;
    std::shared_ptr<Impl> _impl;
};

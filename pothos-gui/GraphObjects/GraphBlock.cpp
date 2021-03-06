// Copyright (c) 2013-2014 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include "GraphObjects/GraphBlockImpl.hpp"
#include "GraphEditor/Constants.hpp"
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <iostream>
#include <cassert>

GraphBlock::GraphBlock(QObject *parent):
    GraphObject(parent),
    _impl(new Impl())
{
    return;
}

void GraphBlock::setBlockDesc(const QByteArray &blockDesc)
{
    assert(not blockDesc.isEmpty());
    Poco::JSON::Parser p; p.parse(std::string(blockDesc.constData(), blockDesc.size()));
    _impl->blockDesc = p.getHandler()->asVar().extract<Poco::JSON::Object::Ptr>();
    assert(_impl->blockDesc);
    this->initPropertiesFromDesc();
}

std::string GraphBlock::getBlockDescPath(void) const
{
    return _impl->blockDesc->getValue<std::string>("path");
}

void GraphBlock::setTitle(const QString &title)
{
    _impl->title = title;
    _impl->changed = true;
}

QString GraphBlock::getTitle(void) const
{
    return _impl->title;
}

void GraphBlock::addProperty(const GraphBlockProp &prop)
{
    _properties.push_back(prop);
    _impl->changed = true;
}

const std::vector<GraphBlockProp> &GraphBlock::getProperties(void) const
{
    return _properties;
}

QString GraphBlock::getPropertyValue(const QString &key) const
{
    auto it = _impl->propertiesValues.find(key);
    if (it != _impl->propertiesValues.end()) return it->second;
    return "";
}

void GraphBlock::setPropertyValue(const QString &key, const QString &value)
{
    _impl->propertiesValues[key] = value;
}

bool GraphBlock::getPropertyPreview(const QString &key) const
{
    auto it = _impl->propertiesPreview.find(key);
    if (it != _impl->propertiesPreview.end()) return it->second;
    return true; //default is ON
}

void GraphBlock::setPropertyPreview(const QString &key, const bool value)
{
    _impl->propertiesPreview[key] = value;
}

void GraphBlock::addInputPort(const GraphBlockPort &port)
{
    _inputPorts.push_back(port);
    _impl->changed = true;
}

const std::vector<GraphBlockPort> &GraphBlock::getInputPorts(void) const
{
    return _inputPorts;
}

void GraphBlock::addOutputPort(const GraphBlockPort &port)
{
    _outputPorts.push_back(port);
    _impl->changed = true;
}

const std::vector<GraphBlockPort> &GraphBlock::getOutputPorts(void) const
{
    return _outputPorts;
}

bool GraphBlock::isPointing(const QRectF &rect) const
{
    //true if it points to a port
    for (const auto &portRect : _impl->inputPortRects)
    {
        if (portRect.intersects(rect)) return true;
    }
    for (const auto &portRect : _impl->outputPortRects)
    {
        if (portRect.intersects(rect)) return true;
    }
    //otherwise does it point to the main body
    return _impl->mainBlockRect.intersects(rect);
}

std::vector<GraphConnectableKey> GraphBlock::getConnectableKeys(void) const
{
    std::vector<GraphConnectableKey> keys;
    for (size_t i = 0; i < _inputPorts.size(); i++)
    {
        keys.push_back(GraphConnectableKey(_inputPorts[i].getKey(), true));
    }
    for (size_t i = 0; i < _outputPorts.size(); i++)
    {
        keys.push_back(GraphConnectableKey(_outputPorts[i].getKey(), false));
    }
    return keys;
}

GraphConnectableKey GraphBlock::isPointingToConnectable(const QPointF &pos) const
{
    for (size_t i = 0; i < _inputPorts.size(); i++)
    {
        if (_impl->inputPortRects[i].contains(pos))
            return GraphConnectableKey(_inputPorts[i].getKey(), true);
    }
    for (size_t i = 0; i < _outputPorts.size(); i++)
    {
        if (_impl->outputPortRects[i].contains(pos))
            return GraphConnectableKey(_outputPorts[i].getKey(), false);
    }
    return GraphConnectableKey();
}

GraphConnectableAttrs GraphBlock::getConnectableAttrs(const GraphConnectableKey &key) const
{
    GraphConnectableAttrs attrs;
    attrs.rotation = this->getRotation();
    if (key.isInput) for (size_t i = 0; i < _inputPorts.size(); i++)
    {
        if (key.id == _inputPorts[i].getKey())
        {
            if (_impl->inputPortPoints.size() > i) //may not be allocated yet
                attrs.point = _impl->inputPortPoints[i];
            attrs.isInput = true;
            return attrs;
        }
    }
    else for (size_t i = 0; i < _outputPorts.size(); i++)
    {
        if (key.id == _outputPorts[i].getKey())
        {
            if (_impl->outputPortPoints.size() > i) //may not be allocated yet
                attrs.point = _impl->outputPortPoints[i];
            attrs.isInput = false;
            return attrs;
        }
    }
    return attrs;
}

static QStaticText makeQStaticText(const QString &s)
{
    QStaticText st(s);
    QTextOption to;
    to.setWrapMode(QTextOption::NoWrap);
    st.setTextOption(to);
    return st;
}

void GraphBlock::renderStaticText(void)
{
    _impl->titleText = makeQStaticText(QString("<span style='font-size:%1;'><b>%2</b></span>")
        .arg(GraphBlockTitleFontSize)
        .arg(_impl->title.toHtmlEscaped()));

    _impl->propertiesText.clear();
    for (size_t i = 0; i < _properties.size(); i++)
    {
        if (not this->getPropertyPreview(_properties[i].getKey())) continue;
        auto text = makeQStaticText(QString("<span style='font-size:%1;'><b>%2: </b> %3</span>")
            .arg(GraphBlockPropFontSize)
            .arg(_properties[i].getName().toHtmlEscaped())
            .arg(this->getPropertyValue(_properties[i].getKey()).toHtmlEscaped()));
        _impl->propertiesText.push_back(text);
    }

    _impl->inputPortsText.resize(_inputPorts.size());
    for (size_t i = 0; i < _inputPorts.size(); i++)
    {
        _impl->inputPortsText[i] = QStaticText(QString("<span style='font-size:%1;'>%2</span>")
            .arg(GraphBlockPortFontSize)
            .arg(_inputPorts[i].getName().toHtmlEscaped()));
    }

    _impl->outputPortsText.resize(_outputPorts.size());
    for (size_t i = 0; i < _outputPorts.size(); i++)
    {
        _impl->outputPortsText[i] = QStaticText(QString("<span style='font-size:%1;'>%2</span>")
            .arg(GraphBlockPortFontSize)
            .arg(_outputPorts[i].getName().toHtmlEscaped()));
    }
}

void GraphBlock::render(QPainter &painter)
{
    //render text
    if (_impl->changed)
    {
        this->update(); //call first because this will set changed again
        _impl->changed = false;
        this->renderStaticText();
    }

    //setup rotations and translations
    QTransform trans;
    trans.translate(this->getPosition().x(), this->getPosition().y());
    painter.translate(this->getPosition());

    //dont rotate past 180 because we just do a port flip
    //this way text only ever has 2 rotations
    trans.rotate(this->getRotation() % 180);
    painter.rotate(this->getRotation() % 180);
    const bool portFlip = this->getRotation() >= 180;

    //calculate dimensions
    qreal inputPortsMinHeight = GraphBlockPortVOutterPad*2;
    if (_impl->inputPortsText.size() == 0) inputPortsMinHeight = 0;
    else inputPortsMinHeight += (_impl->inputPortsText.size()-1)*GraphBlockPortVGap;
    for (const auto &text : _impl->inputPortsText)
    {
        inputPortsMinHeight += text.size().height() + GraphBlockPortTextVPad*2;
    }

    qreal outputPortsMinHeight = GraphBlockPortVOutterPad*2;
    if (_impl->outputPortsText.size() == 0) outputPortsMinHeight = 0;
    else outputPortsMinHeight += (_impl->outputPortsText.size()-1)*GraphBlockPortVGap;
    for (const auto &text : _impl->outputPortsText)
    {
        outputPortsMinHeight += text.size().height() + GraphBlockPortTextVPad*2;
    }

    qreal propertiesMinHeight = 0;
    qreal propertiesMaxWidth = 0;
    for (const auto &text : _impl->propertiesText)
    {
        propertiesMinHeight += text.size().height() + GraphBlockPropTextVPad*2;
        propertiesMaxWidth = std::max<qreal>(propertiesMaxWidth, text.size().width() + GraphBlockPropTextHPad*2);
    }

    const qreal propertiesWithTitleMinHeight = GraphBlockTitleVPad + _impl->titleText.size().height() + GraphBlockTitleVPad + propertiesMinHeight;
    const qreal overallHeight = std::max(std::max(inputPortsMinHeight, outputPortsMinHeight), propertiesWithTitleMinHeight);
    const qreal overallWidth = std::max(GraphBlockTitleHPad + _impl->titleText.size().width() + GraphBlockTitleHPad, propertiesMaxWidth);

    //new dimensions for the main rectangle
    QRectF mainRect(QPointF(), QPointF(overallWidth, overallHeight));
    mainRect.moveCenter(QPointF());
    _impl->mainBlockRect = trans.mapRect(mainRect);
    auto p = mainRect.topLeft();

    //set painter for drawing the rectangles
    QPen pen(QColor(getSelected()?GraphObjectHighlightPenColor:GraphObjectDefaultPenColor));
    pen.setWidthF(GraphObjectBorderWidth);
    painter.setPen(pen);
    painter.setBrush(QBrush(QColor(GraphObjectDefaultFillColor)));

    //create input ports
    _impl->inputPortRects.clear();
    _impl->inputPortPoints.clear();
    qreal inPortVdelta = (overallHeight - inputPortsMinHeight)/2.0 + GraphBlockPortVOutterPad;
    for (const auto &text : _impl->inputPortsText)
    {
        QSizeF rectSize = text.size() + QSizeF(GraphBlockPortTextHPad*2, GraphBlockPortTextVPad*2);
        const qreal hOff = (portFlip)? overallWidth :  1-rectSize.width();
        QRectF portRect(p+QPointF(hOff, inPortVdelta), rectSize);
        inPortVdelta += rectSize.height() + GraphBlockPortVGap;
        painter.drawRect(portRect);
        _impl->inputPortRects.push_back(trans.mapRect(portRect));

        const qreal availablePortHPad = portRect.width() - text.size().width();
        const qreal availablePortVPad = portRect.height() - text.size().height();
        painter.drawStaticText(portRect.topLeft()+QPointF(availablePortHPad/2.0, availablePortVPad/2.0), text);

        //connection point logic
        const auto connPoint = portRect.topLeft() + QPointF(portFlip?rectSize.width():0, rectSize.height()/2);
        _impl->inputPortPoints.push_back(trans.map(connPoint));
    }

    //create output ports
    _impl->outputPortRects.clear();
    _impl->outputPortPoints.clear();
    qreal outPortVdelta = (overallHeight - outputPortsMinHeight)/2.0 + GraphBlockPortVOutterPad;
    for (const auto &text : _impl->outputPortsText)
    {
        QSizeF rectSize = text.size() + QSizeF(GraphBlockPortTextHPad*2+GraphBlockPortArc, GraphBlockPortTextVPad*2);
        const qreal hOff = (portFlip)? 1-rectSize.width() :  overallWidth;
        const qreal arcFix = (portFlip)? GraphBlockPortArc : -GraphBlockPortArc;
        QRectF portRect(p+QPointF(hOff+arcFix, outPortVdelta), rectSize);
        outPortVdelta += rectSize.height() + GraphBlockPortVGap;
        painter.drawRoundedRect(portRect, GraphBlockPortArc, GraphBlockPortArc);
        _impl->outputPortRects.push_back(trans.mapRect(portRect));

        const qreal availablePortHPad = portRect.width() - text.size().width() + arcFix;
        const qreal availablePortVPad = portRect.height() - text.size().height();
        painter.drawStaticText(portRect.topLeft()+QPointF(availablePortHPad/2.0-arcFix, availablePortVPad/2.0), text);

        //connection point logic
        const auto connPoint = portRect.topLeft() + QPointF(portFlip?0:rectSize.width(), rectSize.height()/2);
        _impl->outputPortPoints.push_back(trans.map(connPoint));
    }

    //draw main body of the block
    painter.drawRoundedRect(mainRect, GraphBlockMainArc, GraphBlockMainArc);

    //create title
    const qreal availableTitleHPad = overallWidth-_impl->titleText.size().width();
    painter.drawStaticText(p+QPointF(availableTitleHPad/2.0, GraphBlockTitleVPad), _impl->titleText);

    //create params
    qreal propVdelta = GraphBlockTitleVPad + _impl->titleText.size().height() + GraphBlockTitleVPad;
    for (const auto &text : _impl->propertiesText)
    {
        painter.drawStaticText(p+QPointF(GraphBlockPropTextHPad, propVdelta), text);
        propVdelta += GraphBlockPropTextVPad + text.size().height() + GraphBlockPropTextVPad;
    }
}

Poco::JSON::Object::Ptr GraphBlock::serialize(void) const
{
    auto obj = GraphObject::serialize();
    obj->set("what", std::string("Block"));
    obj->set("path", this->getBlockDescPath());

    Poco::JSON::Array jPropsObj;
    for (const auto &property : this->getProperties())
    {
        Poco::JSON::Object jPropObj;
        jPropObj.set("key", property.getKey().toStdString());
        jPropObj.set("name", property.getName().toStdString());
        jPropObj.set("value", this->getPropertyValue(property.getKey()).toStdString());
        jPropsObj.add(jPropObj);
    }
    obj->set("properties", jPropsObj);

    Poco::JSON::Array jInPortsObj;
    for (const auto &port : this->getInputPorts())
    {
        Poco::JSON::Object jPortObj;
        jPortObj.set("key", port.getKey().toStdString());
        jPortObj.set("name", port.getName().toStdString());
        jInPortsObj.add(jPortObj);
    }
    obj->set("inputPorts", jInPortsObj);

    Poco::JSON::Array jOutPortsObj;
    for (const auto &port : this->getOutputPorts())
    {
        Poco::JSON::Object jPortObj;
        jPortObj.set("key", port.getKey().toStdString());
        jPortObj.set("name", port.getName().toStdString());
        jOutPortsObj.add(jPortObj);
    }
    obj->set("outputPorts", jOutPortsObj);
    return obj;
}

QByteArray blockDescFromPath(const std::string &path);

void GraphBlock::deserialize(Poco::JSON::Object::Ptr obj)
{
    auto path = obj->getValue<std::string>("path");
    auto properties = obj->getArray("properties");

    //init the block with the description
    this->setBlockDesc(blockDescFromPath(path));

    assert(properties);
    for (size_t i = 0; i < properties->size(); i++)
    {
        const auto jPropObj = properties->getObject(i);
        GraphBlockProp prop(
            QString::fromStdString(jPropObj->getValue<std::string>("key")),
            QString::fromStdString(jPropObj->getValue<std::string>("name")));
        /*
        this->addProperty(prop);
        */
        this->setPropertyValue(prop.getKey(), QString::fromStdString(jPropObj->getValue<std::string>("value")));
    }

    /*
    auto inputPorts = jsonObj->getArray("inputPorts");
    assert(inputPorts);
    for (size_t i = 0; i < inputPorts->size(); i++)
    {
        const auto jInPortsObj = inputPorts->getObject(i);
        GraphBlockPort port(
            QString::fromStdString(jInPortsObj->getValue<std::string>("key")),
            QString::fromStdString(jInPortsObj->getValue<std::string>("name")));
        block->addInputPort(port);
    }

    auto outputPorts = jsonObj->getArray("outputPorts");
    assert(outputPorts);
    for (size_t i = 0; i < outputPorts->size(); i++)
    {
        const auto jOutPortsObj = outputPorts->getObject(i);
        GraphBlockPort port(
            QString::fromStdString(jOutPortsObj->getValue<std::string>("key")),
            QString::fromStdString(jOutPortsObj->getValue<std::string>("name")));
        block->addOutputPort(port);
    }
    */

    GraphObject::deserialize(obj);
}

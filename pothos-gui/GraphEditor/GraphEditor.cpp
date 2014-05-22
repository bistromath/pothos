// Copyright (c) 2013-2014 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include "GraphEditor/GraphEditor.hpp"
#include "GraphEditor/GraphDraw.hpp"
#include "GraphEditor/Constants.hpp"
#include "GraphObjects/GraphBlock.hpp"
#include "GraphObjects/GraphBreaker.hpp"
#include "GraphObjects/GraphConnection.hpp"
#include <QTabBar>
#include <QInputDialog>
#include <QAction>
#include <QMenu>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalMapper>
#include <QDockWidget>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QFile>
#include <fstream>
#include <iostream>
#include <cassert>
#include <set>
#include <sstream>
#include <Poco/Path.h>
#include <Poco/UUID.h>
#include <Poco/UUIDGenerator.h>
#include <Poco/JSON/Parser.h>

GraphEditor::GraphEditor(QWidget *parent):
    QTabWidget(parent),
    _parentTabWidget(dynamic_cast<QTabWidget *>(parent)),
    _moveGraphObjectsMapper(new QSignalMapper(this)),
    _stateManager(new GraphStateManager(this))
{
    this->setMovable(true);
    this->setUsesScrollButtons(true);
    this->setTabPosition(QTabWidget::West);
    this->makeDefaultPage();

    this->tabBar()->setStyleSheet("font-size:8pt;");

    //connect handlers that work at the page-level of control
    connect(QApplication::clipboard(), SIGNAL(dataChanged(void)), this, SLOT(handleClipboardDataChange(void)));
    connect(this, SIGNAL(currentChanged(int)), this, SLOT(handleCurrentChanged(int)));
    connect(_stateManager, SIGNAL(newStateSelected(int)), this, SLOT(handleResetState(int)));
    connect(getActionMap()["createGraphPage"], SIGNAL(triggered(void)), this, SLOT(handleCreateGraphPage(void)));
    connect(getActionMap()["renameGraphPage"], SIGNAL(triggered(void)), this, SLOT(handleRenameGraphPage(void)));
    connect(getActionMap()["deleteGraphPage"], SIGNAL(triggered(void)), this, SLOT(handleDeleteGraphPage(void)));
    connect(getActionMap()["createInputBreaker"], SIGNAL(triggered(void)), this, SLOT(handleCreateInputBreaker(void)));
    connect(getActionMap()["createOutputBreaker"], SIGNAL(triggered(void)), this, SLOT(handleCreateOutputBreaker(void)));
    connect(getActionMap()["cut"], SIGNAL(triggered(void)), this, SLOT(handleCut(void)));
    connect(getActionMap()["copy"], SIGNAL(triggered(void)), this, SLOT(handleCopy(void)));
    connect(getActionMap()["paste"], SIGNAL(triggered(void)), this, SLOT(handlePaste(void)));
    connect(getWidgetMap()["blockTree"], SIGNAL(addBlockEvent(const QByteArray &)), this, SLOT(handleAddBlock(const QByteArray &)));
    connect(getActionMap()["selectAll"], SIGNAL(triggered(void)), this, SLOT(handleSelectAll(void)));
    connect(getActionMap()["delete"], SIGNAL(triggered(void)), this, SLOT(handleDelete(void)));
    connect(getActionMap()["rotateLeft"], SIGNAL(triggered(void)), this, SLOT(handleRotateLeft(void)));
    connect(getActionMap()["rotateRight"], SIGNAL(triggered(void)), this, SLOT(handleRotateRight(void)));
    connect(getActionMap()["properties"], SIGNAL(triggered(void)), this, SLOT(handleProperties(void)));
    connect(getActionMap()["zoomIn"], SIGNAL(triggered(void)), this, SLOT(handleZoomIn(void)));
    connect(getActionMap()["zoomOut"], SIGNAL(triggered(void)), this, SLOT(handleZoomOut(void)));
    connect(getActionMap()["zoomOriginal"], SIGNAL(triggered(void)), this, SLOT(handleZoomOriginal(void)));
    connect(getActionMap()["undo"], SIGNAL(triggered(void)), this, SLOT(handleUndo(void)));
    connect(getActionMap()["redo"], SIGNAL(triggered(void)), this, SLOT(handleRedo(void)));
    connect(_moveGraphObjectsMapper, SIGNAL(mapped(int)), this, SLOT(handleMoveGraphObjects(int)));
    connect(this, SIGNAL(newTitleSubtext(const QString &)), getWidgetMap()["mainWindow"], SLOT(handleNewTitleSubtext(const QString &)));
}

QString GraphEditor::newId(const QString &hint) const
{
    std::set<QString> allIds;
    for (int pageNo = 0; pageNo < this->count(); pageNo++)
    {
        for (auto graphObj : this->getGraphDraw(pageNo)->getGraphObjects())
        {
            allIds.insert(graphObj->getId());
        }
    }

    //either use the hint or UUID if blank
    QString idBase = hint;
    if (idBase.isEmpty())
    {
        Poco::UUIDGenerator &generator = Poco::UUIDGenerator::defaultGenerator();
        idBase = QString::fromStdString(generator.createRandom().toString());
    }

    //loop for a unique ID name
    QString possibleId = idBase;
    size_t index = 0;
    while (allIds.find(possibleId) != allIds.end())
    {
        possibleId = QString("%1%2").arg(idBase).arg(++index);
    }

    return possibleId;
}

void GraphEditor::showEvent(QShowEvent *event)
{
    //load our state monitor into the actions dock
    auto actionsDock = dynamic_cast<QDockWidget *>(getWidgetMap()["graphActionsDock"]);
    assert(actionsDock != nullptr);
    actionsDock->setWidget(_stateManager);

    this->setupMoveGraphObjectsMenu();
    this->updateEnabledActions();
    QWidget::showEvent(event);
}

void GraphEditor::updateEnabledActions(void)
{
    if (not this->isVisible()) return;

    getActionMap()["undo"]->setEnabled(_stateManager->isPreviousAvailable());
    getActionMap()["redo"]->setEnabled(_stateManager->isSubsequentAvailable());
    getActionMap()["save"]->setEnabled(not _stateManager->isCurrentSaved());
    getActionMap()["reload"]->setEnabled(not this->getCurrentFilePath().isEmpty());

    //can we paste something from the clipboard?
    auto mimeData = QApplication::clipboard()->mimeData();
    const bool canPaste = mimeData->hasFormat("text/json/pothos_object_array") and
                      not mimeData->data("text/json/pothos_object_array").isEmpty();
    getActionMap()["paste"]->setEnabled(canPaste);

    //update window title
    QString subtext = this->getCurrentFilePath();
    QString title(tr("Editing "));
    if (subtext.isEmpty())
    {
        title += tr("untitled");
    }
    else
    {
        title += subtext;
        auto file = new QFile(subtext);
        bool readOnly = !(file->permissions() & QFileDevice::WriteUser);
        if (readOnly) title += " (read-only)";
    }
    emit this->newTitleSubtext(title);
}

void GraphEditor::handleCurrentChanged(int)
{
    if (not this->isVisible()) return;
    this->setupMoveGraphObjectsMenu();
}

void GraphEditor::handleCreateGraphPage(void)
{
    if (not this->isVisible()) return;
    const QString newName = QInputDialog::getText(this, tr("Create page"),
        tr("New page name"), QLineEdit::Normal, tr("untitled"));
    if (newName.isEmpty()) return;
    this->addTab(makeGraphPage(this), newName);
    this->setupMoveGraphObjectsMenu();

    handleStateChange(GraphState("document-new", tr("Create graph page ") + newName));
}

void GraphEditor::handleRenameGraphPage(void)
{
    if (not this->isVisible()) return;
    const auto oldName = this->tabText(this->currentIndex());
    const QString newName = QInputDialog::getText(this, tr("Rename page"),
        tr("New page name"), QLineEdit::Normal, oldName);
    if (newName.isEmpty()) return;
    this->setTabText(this->currentIndex(), newName);
    this->setupMoveGraphObjectsMenu();

    handleStateChange(GraphState("edit-rename", tr("Rename graph page ") + oldName + " -> " + newName));
}

void GraphEditor::handleDeleteGraphPage(void)
{
    if (not this->isVisible()) return;
    const auto oldName = this->tabText(this->currentIndex());
    this->removeTab(this->currentIndex());
    if (this->count() == 0) this->makeDefaultPage();
    this->setupMoveGraphObjectsMenu();

    handleStateChange(GraphState("edit-delete", tr("Delete graph page ") + oldName));
}

GraphConnection *GraphEditor::makeConnection(const GraphConnectionEndpoint &ep0, const GraphConnectionEndpoint &ep1)
{
    auto conn = new GraphConnection(ep0.getObj()->parent());
    conn->setupEndpoint(ep0);
    conn->setupEndpoint(ep1);

    const auto idHint = QString("Connection_%1%2_%3%4").arg(
        conn->getOutputEndpoint().getObj()->getId(),
        conn->getOutputEndpoint().getKey().id,
        conn->getInputEndpoint().getObj()->getId(),
        conn->getInputEndpoint().getKey().id
    );
    conn->setId(this->newId(idHint));
    assert(conn->getInputEndpoint().isValid());
    assert(conn->getOutputEndpoint().isValid());

    return conn;
}

//TODO traverse breakers and find one in the node mass that already exists

static GraphBreaker *findInputBreaker(GraphEditor *editor, const GraphConnectionEndpoint &ep)
{
    for (auto obj : editor->getGraphObjects())
    {
        auto conn = dynamic_cast<GraphConnection *>(obj);
        if (conn == nullptr) continue;
        if (not (conn->getOutputEndpoint().getObj()->parent() == conn->getInputEndpoint().getObj()->parent())) continue;
        if (not (conn->getOutputEndpoint() == ep)) continue;
        auto breaker = dynamic_cast<GraphBreaker *>(conn->getInputEndpoint().getObj().data());
        if (breaker == nullptr) continue;
        return breaker;
    }
    return nullptr;
}

void GraphEditor::handleMoveGraphObjects(const int index)
{
    if (not this->isVisible()) return;
    if (index >= this->count()) return;
    auto draw = this->getGraphDraw(this->currentIndex());
    auto desc = tr("Move %1 to %2").arg(draw->getSelectionDescription(), this->tabText(index));

    //move all selected objects
    for (auto obj : draw->getObjectsSelected())
    {
        obj->setSelected(false);
        obj->setParent(this->getGraphDraw(index));
    }

    //reparent all connections based on endpoints:
    std::vector<GraphConnection *> boundaryConnections;
    for (auto obj : this->getGraphObjects())
    {
        auto conn = dynamic_cast<GraphConnection *>(obj);
        if (conn == nullptr) continue;

        //Connection has the same endpoints, so make sure that the parent is corrected to the endpoint
        if (conn->getOutputEndpoint().getObj()->parent() == conn->getInputEndpoint().getObj()->parent())
        {
            conn->setParent(conn->getInputEndpoint().getObj()->parent());
        }

        //otherwise stash it for more processing
        else
        {
            boundaryConnections.push_back(conn);
        }
    }

    //create breakers for output endpoints that have to cross
    for (auto conn : boundaryConnections)
    {
        const auto &epOut = conn->getOutputEndpoint();
        const auto &epIn = conn->getInputEndpoint();

        auto breaker = findInputBreaker(this, epOut);
        if (breaker != nullptr) continue;

        breaker = new GraphBreaker(epOut.getObj()->parent());
        breaker->setInput(true);
        const auto name = QString("%1[%2]").arg(epOut.getObj()->getId(), epOut.getKey().id);
        breaker->setId(this->newId(name));
        breaker->setNodeName(breaker->getId()); //the first of its name
        breaker->setRotation(epIn.getObj()->getRotation());
        breaker->setPosition(epIn.getObj()->getPosition());

        auto outConn = this->makeConnection(epOut, GraphConnectionEndpoint(breaker, breaker->getConnectableKeys().at(0)));
        outConn->setParent(breaker->parent());
    }

    //create breakers for input endpoints that have to cross
    for (auto conn : boundaryConnections)
    {
        const auto &epOut = conn->getOutputEndpoint();
        const auto &epIn = conn->getInputEndpoint();

        //find the existing breaker or make a new one
        const auto name = findInputBreaker(this, epOut)->getNodeName();
        GraphBreaker *breaker = nullptr;
        for (auto obj : this->getGraphObjects())
        {
            if (obj->parent() != epIn.getObj()->parent()) continue;
            auto outBreaker = dynamic_cast<GraphBreaker *>(obj);
            if (outBreaker == nullptr) continue;
            if (outBreaker->isInput()) continue;
            if (outBreaker->getNodeName() != name) continue;
            breaker = outBreaker;
            break;
        }

        //make a new output breaker
        if (breaker == nullptr)
        {
            breaker = new GraphBreaker(epIn.getObj()->parent());
            breaker->setInput(false);
            breaker->setId(this->newId(name));
            breaker->setNodeName(name);
            breaker->setRotation(epOut.getObj()->getRotation());
            breaker->setPosition(epOut.getObj()->getPosition());
        }

        //connect to this breaker
        auto inConn = this->makeConnection(epIn, GraphConnectionEndpoint(breaker, breaker->getConnectableKeys().at(0)));
        inConn->setParent(breaker->parent());

        delete conn;
    }

    handleStateChange(GraphState("transform-move", desc));
}

void GraphEditor::handleAddBlock(const QByteArray &json)
{
    if (not this->isVisible()) return;
    QPoint where(std::rand()%100, std::rand()%100);

    //determine where, a nice point on the visible drawing area sort of upper left
    auto scrollArea = dynamic_cast<QScrollArea *>(this->currentWidget());
    if (scrollArea != nullptr) where += QPoint(
        scrollArea->horizontalScrollBar()->value() + scrollArea->size().width()/4,
        scrollArea->verticalScrollBar()->value() + scrollArea->size().height()/4);

    this->handleAddBlock(json, where);
}

void GraphEditor::handleAddBlock(const QByteArray &blockDesc, const QPoint &where)
{
    if (blockDesc.isEmpty()) return;
    auto draw = this->getGraphDraw(this->currentIndex());
    auto block = new GraphBlock(draw);
    block->setBlockDesc(blockDesc);

    QString hint;
    const auto title = block->getTitle();
    for (int i = 0; i < title.length(); i++)
    {
        if (i == 0 and title.at(i).isNumber()) hint.append(QChar('_'));
        if (title.at(i).isLetterOrNumber() or title.at(i).toLatin1() == '_')
        {
            hint.append(title.at(i));
        }
    }
    block->setId(this->newId(hint));

    //TODO set a top most z index

    block->setPosition(QPointF(where)/draw->zoomScale());
    block->setRotation(0);
    handleStateChange(GraphState("list-add", tr("Create block %1").arg(title)));
}

void GraphEditor::handleCreateBreaker(const bool isInput)
{
    if (not this->isVisible()) return;

    const auto dirName = isInput?tr("input"):tr("output");
    const auto newName = QInputDialog::getText(this, tr("Create %1 breaker").arg(dirName),
        tr("New breaker node name"), QLineEdit::Normal, tr("untitled"));
    if (newName.isEmpty()) return;

    auto draw = this->getGraphDraw(this->currentIndex());
    auto breaker = new GraphBreaker(draw);
    breaker->setInput(isInput);
    breaker->setNodeName(newName);
    breaker->setId(this->newId(newName));
    breaker->setPosition(draw->getLastContextMenuPos()/draw->zoomScale());

    handleStateChange(GraphState("document-new", tr("Create %1 breaker %2").arg(dirName, newName)));
}

void GraphEditor::handleCreateInputBreaker(void)
{
    this->handleCreateBreaker(true);
}

void GraphEditor::handleCreateOutputBreaker(void)
{
    this->handleCreateBreaker(false);
}

void GraphEditor::handleCut(void)
{
    auto draw = this->getGraphDraw(this->currentIndex());
    auto desc = tr("Cut %1").arg(draw->getSelectionDescription());

    //load up the clipboard
    this->handleCopy();

    //delete all selected graph objects
    for (auto obj : draw->getObjectsSelected())
    {
        delete obj;
    }

    this->deleteFlagged();

    handleStateChange(GraphState("edit-cut", desc));
}

void GraphEditor::handleCopy(void)
{
    if (not this->isVisible()) return;
    auto draw = this->getGraphDraw(this->currentIndex());

    Poco::JSON::Array jsonObjs;
    for (auto obj : draw->getObjectsSelected())
    {
        jsonObjs.add(obj->serialize());
    }

    //to byte array
    std::ostringstream oss;
    jsonObjs.stringify(oss);
    QByteArray byteArray(oss.str().data(), oss.str().size());

    //load the clipboard
    auto mimeData = new QMimeData();
    mimeData->setData("text/json/pothos_object_array", byteArray);
    QApplication::clipboard()->setMimeData(mimeData);
}

void GraphEditor::handlePaste(void)
{
    if (not this->isVisible()) return;
    auto draw = this->getGraphDraw(this->currentIndex());

    auto mimeData = QApplication::clipboard()->mimeData();
    const bool canPaste = mimeData->hasFormat("text/json/pothos_object_array") and
                      not mimeData->data("text/json/pothos_object_array").isEmpty();
    if (not canPaste) return;

    //extract object array
    const auto data = mimeData->data("text/json/pothos_object_array");
    const std::string dataStr(data.constData(), data.size());
    std::istringstream iss(dataStr);
    Poco::JSON::Parser p; p.parse(iss);
    auto graphObjects = p.getHandler()->asVar().extract<Poco::JSON::Array::Ptr>();
    assert(graphObjects);

    //deal with initial positions of pasted objects
    GraphObjectList newObjects;
    QPointF cornerest(1e6, 1e6);

    //unselect all objects
    for (auto obj : draw->getGraphObjects(false))
    {
        obj->setSelected(false);
    }

    //create objects
    std::map<QString, QPointer<GraphObject>> oldIdToObj;
    for (size_t objIndex = 0; objIndex < graphObjects->size(); objIndex++)
    {
        const auto jGraphObj = graphObjects->getObject(objIndex);
        const auto what = jGraphObj->getValue<std::string>("what");
        GraphObject *obj = nullptr;
        if (what == "Block") obj = new GraphBlock(draw);
        if (what == "Breaker") obj = new GraphBreaker(draw);
        if (obj != nullptr)
        {
            obj->deserialize(jGraphObj);
            const auto oldId = obj->getId();
            oldIdToObj[oldId] = obj;
            obj->setId(this->newId(oldId)); //make sure id is unique
            obj->setSelected(true);
            newObjects.push_back(obj);
            cornerest.setX(std::min(cornerest.x(), obj->getPosition().x()));
            cornerest.setY(std::min(cornerest.y(), obj->getPosition().y()));
        }
    }

    //move objects into position
    //TODO min max on this position
    const auto myPos = QPointF(draw->mapFromGlobal(QCursor::pos()))/draw->zoomScale();
    for (auto obj : newObjects)
    {
        obj->setPosition(obj->getPosition()-cornerest+myPos);
    }

    //create connections
    for (size_t objIndex = 0; objIndex < graphObjects->size(); objIndex++)
    {
        const auto jGraphObj = graphObjects->getObject(objIndex);
        const auto what = jGraphObj->getValue<std::string>("what");
        if (what != "Connection") continue;

        //extract fields
        auto outputId = QString::fromStdString(jGraphObj->getValue<std::string>("outputId"));
        auto inputId = QString::fromStdString(jGraphObj->getValue<std::string>("inputId"));
        auto outputKey = QString::fromStdString(jGraphObj->getValue<std::string>("outputKey"));
        auto inputKey = QString::fromStdString(jGraphObj->getValue<std::string>("inputKey"));

        //get the objects
        auto inputObj = oldIdToObj[inputId];
        auto outputObj = oldIdToObj[outputId];

        //did we get both endpoints in this paste? if not its ok, we dont make the connection
        if (inputObj.isNull()) continue;
        if (outputObj.isNull()) continue;

        this->makeConnection(
            GraphConnectionEndpoint(inputObj, GraphConnectableKey(inputKey, true)),
            GraphConnectionEndpoint(outputObj, GraphConnectableKey(outputKey, false)));
    }

    handleStateChange(GraphState("edit-paste", tr("Paste %1").arg(draw->getSelectionDescription())));
}

void GraphEditor::handleClipboardDataChange(void)
{
    if (not this->isVisible()) return;
    this->updateEnabledActions();
}

void GraphEditor::handleSelectAll(void)
{
    if (not this->isVisible()) return;
    auto draw = this->getGraphDraw(this->currentIndex());
    for (auto obj : draw->getGraphObjects(false/*nosort*/))
    {
        obj->setSelected(true);
    }
    this->render();
}

void GraphEditor::deleteFlagged(void)
{
    //delete all objects flagged for deletion
    while (true)
    {
        bool deletionOccured = false;
        for (auto obj : this->getGraphObjects())
        {
            if (obj->isFlaggedForDelete())
            {
                delete obj;
                deletionOccured = true;
            }
        }
        if (not deletionOccured) break;
    }
}

void GraphEditor::handleDelete(void)
{
    if (not this->isVisible()) return;
    auto draw = this->getGraphDraw(this->currentIndex());
    auto desc = tr("Delete %1").arg(draw->getSelectionDescription());

    //delete all selected graph objects
    for (auto obj : draw->getObjectsSelected())
    {
        delete obj;
    }

    this->deleteFlagged();

    handleStateChange(GraphState("edit-delete", desc));
}

void GraphEditor::handleRotateLeft(void)
{
    if (not this->isVisible()) return;
    auto draw = this->getGraphDraw(this->currentIndex());
    for (auto obj : draw->getObjectsSelected())
    {
        obj->rotateLeft();
    }
    handleStateChange(GraphState("object-rotate-left", tr("Rotate %1 left").arg(draw->getSelectionDescription())));
}

void GraphEditor::handleRotateRight(void)
{
    if (not this->isVisible()) return;
    auto draw = this->getGraphDraw(this->currentIndex());
    for (auto obj : draw->getObjectsSelected())
    {
        obj->rotateRight();
    }
    handleStateChange(GraphState("object-rotate-right", tr("Rotate %1 right").arg(draw->getSelectionDescription())));
}

void GraphEditor::handleProperties(void)
{
    if (not this->isVisible()) return;
    auto draw = this->getGraphDraw(this->currentIndex());
    const auto objs = draw->getObjectsSelected();
    if (not objs.isEmpty()) emit draw->modifyProperties(objs.at(0));
}

void GraphEditor::handleZoomIn(void)
{
    if (not this->isVisible()) return;
    auto draw = this->getGraphDraw(this->currentIndex());
    if (draw->zoomScale() >= GraphDrawZoomMax) return;
    draw->setZoomScale(draw->zoomScale() + GraphDrawZoomStep);
}

void GraphEditor::handleZoomOut(void)
{
    if (not this->isVisible()) return;
    auto draw = this->getGraphDraw(this->currentIndex());
    if (draw->zoomScale() <= GraphDrawZoomMin) return;
    draw->setZoomScale(draw->zoomScale() - GraphDrawZoomStep);
}

void GraphEditor::handleZoomOriginal(void)
{
    if (not this->isVisible()) return;
    auto draw = this->getGraphDraw(this->currentIndex());
    draw->setZoomScale(1.0);
}

void GraphEditor::handleUndo(void)
{
    if (not this->isVisible()) return;
    assert(_stateManager->isPreviousAvailable());
    this->handleResetState(_stateManager->getCurrentIndex()-1);
}

void GraphEditor::handleRedo(void)
{
    if (not this->isVisible()) return;
    assert(_stateManager->isSubsequentAvailable());
    this->handleResetState(_stateManager->getCurrentIndex()+1);
}

void GraphEditor::handleResetState(int stateNo)
{
    if (not this->isVisible()) return;
    _stateManager->resetTo(stateNo);
    const auto dump = _stateManager->current().dump;
    std::istringstream iss(std::string(dump.constData(), dump.size()));
    this->loadState(iss);
    this->setupMoveGraphObjectsMenu();
    this->render();
}

void GraphEditor::handleStateChange(const GraphState &state)
{
    //serialize the graph into the state manager
    std::ostringstream oss;
    this->dumpState(oss);
    GraphState stateWithDump = state;
    stateWithDump.dump = QByteArray(oss.str().data(), oss.str().size());
    _stateManager->post(stateWithDump);
    this->render();
}

void GraphEditor::save(void)
{
    assert(not this->getCurrentFilePath().isEmpty());

    auto fileName = this->getCurrentFilePath().toStdString();
    try
    {
        std::ofstream outFile(fileName.c_str());
        this->dumpState(outFile);
        _stateManager->saveCurrent();
        this->render();
    }
    catch (const std::exception &ex)
    {
        //TODO log
    }
}

void GraphEditor::load(void)
{
    auto fileName = this->getCurrentFilePath().toStdString();

    if (fileName.empty())
    {
        _stateManager->resetToDefault();
        handleStateChange(GraphState("document-new", tr("Create new topology")));
        _stateManager->saveCurrent();
        this->render();
        return;
    }

    try
    {
        std::ifstream inFile(fileName.c_str());
        this->loadState(inFile);
        _stateManager->resetToDefault();
        handleStateChange(GraphState("document-new", tr("Load topology from file")));
        _stateManager->saveCurrent();
        this->setupMoveGraphObjectsMenu();
        this->render();
    }
    catch (const std::exception &ex)
    {
        //TODO log
    }
}

void GraphEditor::render(void)
{
    //generate a title
    QString title = tr("untitled");
    if (not this->getCurrentFilePath().isEmpty())
    {
        auto name = Poco::Path(this->getCurrentFilePath().toStdString()).getBaseName();
        title = QString::fromStdString(name);
    }
    if (this->hasUnsavedChanges()) title += "*";

    //set the tab text
    for (int i = 0; i < _parentTabWidget->count(); i++)
    {
        if (_parentTabWidget->widget(i) != this) continue;
        _parentTabWidget->setTabText(i, title);
    }

    this->getGraphDraw(this->currentIndex())->render();
    this->updateEnabledActions();
}

void GraphEditor::setupMoveGraphObjectsMenu(void)
{
    if (not this->isVisible()) return;
    auto menu = getMenuMap()["moveGraphObjects"];
    menu->clear();
    for (int i = 0; i < this->count(); i++)
    {
        if (i == this->currentIndex()) continue;
        auto action = menu->addAction(QString("%1 (%2)").arg(this->tabText(i)).arg(i));
        connect(action, SIGNAL(triggered(void)), _moveGraphObjectsMapper, SLOT(map(void)));
        _moveGraphObjectsMapper->setMapping(action, i);
    }
}

GraphDraw *GraphEditor::getGraphDraw(const int index) const
{
    auto scroll = dynamic_cast<QScrollArea *>(this->widget(index));
    assert(scroll != nullptr);
    auto draw = dynamic_cast<GraphDraw *>(scroll->widget());
    assert(draw != nullptr);
    return draw;
}

GraphObjectList GraphEditor::getGraphObjects(void) const
{
    GraphObjectList all;
    for (int i = 0; i < this->count(); i++)
    {
        for (auto obj : this->getGraphDraw(i)->getGraphObjects(false))
        {
            all.push_back(obj);
        }
    }
    return all;
}

void GraphEditor::makeDefaultPage(void)
{
    this->insertTab(0, makeGraphPage(this), tr("Main"));
}

QWidget *makeGraphEditor(QWidget *parent)
{
    return new GraphEditor(parent);
};

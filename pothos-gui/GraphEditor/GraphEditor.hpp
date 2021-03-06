// Copyright (c) 2014-2014 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#pragma once
#include "PothosGui.hpp"
#include "GraphObjects/GraphObject.hpp"
#include "GraphEditor/GraphState.hpp"
#include <QTabWidget>
#include <ios>

class GraphConnection;
class GraphDraw;
class QSignalMapper;
class QTabWidget;

class GraphEditor : public QTabWidget
{
    Q_OBJECT
public:
    GraphEditor(QWidget *parent);

    void dumpState(std::ostream &os) const;

    void loadState(std::istream &os);

    QString newId(const QString &hint = "") const;

    //! Serializes the editor and saves to file.
    void save(void);

    //! Deserializes the editor from the file.
    void load(void);

    const QString &getCurrentFilePath(void) const
    {
        return _currentFilePath;
    }

    void setCurrentFilePath(const QString &path)
    {
        _currentFilePath = path;
    }

    bool hasUnsavedChanges(void) const
    {
        return not _stateManager->isCurrentSaved();
    }

    void handleAddBlock(const QByteArray &, const QPoint &);

    //! force a re-rendering of the graph page
    void render(void);

    GraphDraw *getGraphDraw(const int index) const;

    //! Get a list of all graph objects in all pages
    GraphObjectList getGraphObjects(void) const;

    //! Make a connection between two endpoints
    GraphConnection *makeConnection(const GraphConnectionEndpoint &ep0, const GraphConnectionEndpoint &ep1);

signals:
    void newTitleSubtext(const QString &);

protected:
    //this widget is visible, populate menu with its tabs
    void showEvent(QShowEvent *event);

private slots:
    void handleCurrentChanged(int);
    void handleCreateGraphPage(void);
    void handleRenameGraphPage(void);
    void handleDeleteGraphPage(void);
    void handleMoveGraphObjects(const int index);
    void handleAddBlock(const QByteArray &);
    void handleCreateBreaker(const bool isInput);
    void handleCreateInputBreaker(void);
    void handleCreateOutputBreaker(void);
    void handleCut(void);
    void handleCopy(void);
    void handlePaste(void);
    void handleClipboardDataChange(void);
    void handleSelectAll(void);
    void handleDelete(void);
    void handleRotateLeft(void);
    void handleRotateRight(void);
    void handleProperties(void);
    void handleZoomIn(void);
    void handleZoomOut(void);
    void handleZoomOriginal(void);
    void handleUndo(void);
    void handleRedo(void);
    void handleResetState(int);
    void handleStateChange(const GraphState &state);

private:
    QTabWidget *_parentTabWidget;

    void setupMoveGraphObjectsMenu(void);

    void makeDefaultPage(void);

    void deleteFlagged(void);

    QSignalMapper *_moveGraphObjectsMapper;

    QString _currentFilePath;
    GraphStateManager *_stateManager;

    //! update enabled actions based on state - after a change or when editor becomes visible
    void updateEnabledActions(void);
};

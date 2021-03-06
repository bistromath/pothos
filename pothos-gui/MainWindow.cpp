// Copyright (c) 2013-2014 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include "PothosGui.hpp"
#include <Pothos/System.hpp>
#include <QMainWindow>
#include <QGridLayout>
#include <QSettings>
#include <QDockWidget>
#include <QMenuBar>
#include <QToolBar>
#include <QAction>
#include <QTabWidget>
#include <QMessageBox>
#include <QSplitter>
#include <QMap>
#include <Poco/SingletonHolder.h>
#include <iostream>

QMap<QString, QAction *> &getActionMap(void)
{
    static Poco::SingletonHolder<QMap<QString, QAction *>> sh;
    return *sh.get();
}

QMap<QString, QMenu *> &getMenuMap(void)
{
    static Poco::SingletonHolder<QMap<QString, QMenu *>> sh;
    return *sh.get();
}

QMap<QString, QWidget *> &getWidgetMap(void)
{
    static Poco::SingletonHolder<QMap<QString, QWidget *>> sh;
    return *sh.get();
}

class PothosGuiMainWindow : public QMainWindow
{
    Q_OBJECT
public:

    PothosGuiMainWindow(QWidget *parent):
        QMainWindow(parent),
        _actionMap(getActionMap()),
        _menuMap(getMenuMap()),
        _topLevelSplitter(new QSplitter(this))
    {
        getWidgetMap()["mainWindow"] = this;

        this->setMinimumSize(800, 600);
        this->setWindowTitle("Pothos GUI");
        this->setCentralWidget(_topLevelSplitter);

        //initialize actions and action buttons
        this->createActions();
        this->createMainToolBar();

        //create message window dock
        _messageWindowDock = new QDockWidget(this);
        _messageWindowDock->setObjectName("MessageWindowDock");
        _messageWindowDock->setWindowTitle(tr("Message Window"));
        _messageWindowDock->setWidget(makeMessageWindow(_messageWindowDock));
        this->addDockWidget(Qt::BottomDockWidgetArea, _messageWindowDock);

        //create graph actions dock
        _graphActionsDock = new QDockWidget(this);
        _graphActionsDock->setObjectName("_graphActionsDock");
        _graphActionsDock->setWindowTitle(tr("Graph Actions"));
        this->addDockWidget(Qt::BottomDockWidgetArea, _graphActionsDock);
        getWidgetMap()["graphActionsDock"] = _graphActionsDock;

        //create remote nodes dock
        _remoteNodesDock = new QDockWidget(this);
        _remoteNodesDock->setObjectName("RemoteNodesDock");
        _remoteNodesDock->setWindowTitle(tr("Remote Nodes"));
        _remoteNodesDock->setWidget(makeRemoteNodesWindow(_remoteNodesDock));
        this->addDockWidget(Qt::TopDockWidgetArea, _remoteNodesDock);
        _remoteNodesDock->hide(); //default is hidden

        //create topology editor tabbed widget
        auto editorTabs = makeGraphEditorTabs(_topLevelSplitter);
        getWidgetMap()["editorTabs"] = editorTabs;

        //create properties panel
        auto propertiesPanel = makePropertiesPanel(this);
        getWidgetMap()["propertiesPanel"] = propertiesPanel;

        //load the splitter between editor and properties
        _topLevelSplitter->addWidget(editorTabs);
        _topLevelSplitter->addWidget(propertiesPanel);

        //restore main window settings from file
        this->restoreGeometry(getSettings().value("MainWindow/geometry").toByteArray());
        this->restoreState(getSettings().value("MainWindow/state").toByteArray());
        _topLevelSplitter->restoreState(getSettings().value("MainSplitter/state").toByteArray());

        //create menus after docks and tool bars (view menu calls their toggleViewAction())
        this->createMenus();

        //we do this last so all of the connections and logging is setup
        emit this->initDone();
    }

signals:
    void initDone(void);
    void exitBegin(QCloseEvent *);

private slots:

    void handleNewTitleSubtext(const QString &s)
    {
        this->setWindowTitle("Pothos GUI - " + s);
    }

    void handleShowAbout(void)
    {
        QMessageBox::about(this, "About Pothos", QString::fromStdString(Pothos::System::getApiVersion()));
    }

    void handleShowAboutQt(void)
    {
        QMessageBox::aboutQt(this);
    }

protected:
    void closeEvent(QCloseEvent *event)
    {
        emit this->exitBegin(event);
        getSettings().setValue("MainWindow/geometry", saveGeometry());
        getSettings().setValue("MainWindow/state", saveState());
        getSettings().setValue("MainSplitter/state", _topLevelSplitter->saveState());
    }

    void showEvent(QShowEvent *event)
    {
        QMainWindow::showEvent(event);
    }

private:

    void createActions(void);
    QAction *_newAction;
    QAction *_openAction;
    QAction *_saveAction;
    QAction *_saveAsAction;
    QAction *_saveAllAction;
    QAction *_reloadAction;
    QAction *_closeAction;
    QAction *_exitAction;
    QAction *_undoAction;
    QAction *_redoAction;
    QAction *_cutAction;
    QAction *_copyAction;
    QAction *_pasteAction;
    QAction *_deleteAction;
    QAction *_selectAllAction;
    QAction *_propertiesAction;
    QAction *_createGraphPageAction;
    QAction *_renameGraphPageAction;
    QAction *_deleteGraphPageAction;
    QAction *_createInputBreakerAction;
    QAction *_createOutputBreakerAction;
    QAction *_rotateLeftAction;
    QAction *_rotateRightAction;
    QAction *_zoomInAction;
    QAction *_zoomOutAction;
    QAction *_zoomOriginalAction;
    QAction *_showMessageWindowAction;
    QAction *_showMainToolBarAction;
    QAction *_showAboutAction;
    QAction *_showAboutQtAction;
    QMap<QString, QAction *> &_actionMap;

    void createMenus(void);
    QMenu *_fileMenu;
    QMenu *_editMenu;
    QMenu *_viewMenu;
    QMenu *_helpMenu;
    QMap<QString, QMenu *> &_menuMap;

    void createMainToolBar(void);
    QToolBar *_mainToolBar;
    QSplitter *_topLevelSplitter;
    QDockWidget *_remoteNodesDock;
    QDockWidget *_messageWindowDock;
    QDockWidget *_graphActionsDock;
};

void PothosGuiMainWindow::createActions(void)
{
    _newAction = new QAction(makeIconFromTheme("document-new"), tr("&New"), this);
    _newAction->setShortcut(QKeySequence::New);
    _actionMap["new"] = _newAction;

    _openAction = new QAction(makeIconFromTheme("document-open"), tr("&Open"), this);
    _openAction->setShortcut(QKeySequence::Open);
    _actionMap["open"] = _openAction;

    _saveAction = new QAction(makeIconFromTheme("document-save"), tr("&Save"), this);
    _saveAction->setShortcut(QKeySequence::Save);
    _actionMap["save"] = _saveAction;

    _saveAsAction = new QAction(makeIconFromTheme("document-save-as"), tr("Save &As"), this);
    _saveAsAction->setShortcut(QKeySequence::SaveAs);
    _actionMap["saveAs"] = _saveAsAction;

    _saveAllAction = new QAction(makeIconFromTheme("document-save-all"), tr("Save A&ll"), this);
    _saveAllAction->setShortcut(QKeySequence("CTRL+SHIFT+A"));
    _actionMap["saveAll"] = _saveAllAction;

    _reloadAction = new QAction(makeIconFromTheme("view-refresh"), tr("&Reload"), this);
    QList<QKeySequence> reloadShortcuts;
        reloadShortcuts.push_back(QKeySequence("CTRL+R"));
        reloadShortcuts.push_back(QKeySequence::Refresh);
    _reloadAction->setShortcuts(reloadShortcuts);
    _actionMap["reload"] = _reloadAction;

    _closeAction = new QAction(makeIconFromTheme("document-close"), tr("&Close"), this);
    _closeAction->setShortcut(QKeySequence::Close);
    _actionMap["close"] = _closeAction;

    _exitAction = new QAction(makeIconFromTheme("application-exit"), tr("&Exit Pothos GUI"), this);
    _exitAction->setShortcut(QKeySequence::Quit);
    connect(_exitAction, SIGNAL(triggered(void)), this, SLOT(close(void)));
    _actionMap["exit"] = _exitAction;

    _undoAction = new QAction(makeIconFromTheme("edit-undo"), tr("&Undo"), this);
    _undoAction->setShortcut(QKeySequence::Undo);
    _actionMap["undo"] = _undoAction;

    _redoAction = new QAction(makeIconFromTheme("edit-redo"), tr("&Redo"), this);
    QList<QKeySequence> redoShortcuts;
        redoShortcuts.push_back(QKeySequence("CTRL+Y"));
        redoShortcuts.push_back(QKeySequence::Redo);
    _redoAction->setShortcuts(redoShortcuts);
    _actionMap["redo"] = _redoAction;

    _cutAction = new QAction(makeIconFromTheme("edit-cut"), tr("Cu&t"), this);
    _cutAction->setShortcut(QKeySequence::Cut);
    _actionMap["cut"] = _cutAction;

    _copyAction = new QAction(makeIconFromTheme("edit-copy"), tr("&Copy"), this);
    _copyAction->setShortcut(QKeySequence::Copy);
    _actionMap["copy"] = _copyAction;

    _pasteAction = new QAction(makeIconFromTheme("edit-paste"), tr("&Paste"), this);
    _pasteAction->setShortcut(QKeySequence::Paste);
    _actionMap["paste"] = _pasteAction;

    _deleteAction = new QAction(makeIconFromTheme("edit-delete"), tr("&Delete"), this);
    _deleteAction->setShortcut(QKeySequence::Delete);
    _actionMap["delete"] = _deleteAction;

    _selectAllAction = new QAction(makeIconFromTheme("edit-select-all"), tr("Select &All"), this);
    _selectAllAction->setShortcut(QKeySequence::SelectAll);
    _actionMap["selectAll"] = _selectAllAction;

    _propertiesAction = new QAction(makeIconFromTheme("document-properties"), tr("Properti&es"), this);
    _selectAllAction->setShortcut(QKeySequence("Enter"));
    _actionMap["properties"] = _propertiesAction;

    _createGraphPageAction = new QAction(makeIconFromTheme("document-new"), tr("Create new graph page"), this);
    _actionMap["createGraphPage"] = _createGraphPageAction;

    _renameGraphPageAction = new QAction(makeIconFromTheme("edit-rename"), tr("Rename this graph page"), this);
    _actionMap["renameGraphPage"] = _renameGraphPageAction;

    _deleteGraphPageAction = new QAction(makeIconFromTheme("edit-delete"), tr("Delete this graph page"), this);
    _actionMap["deleteGraphPage"] = _deleteGraphPageAction;

    _createInputBreakerAction = new QAction(makeIconFromTheme("document-new"), tr("Create input breaker"), this);
    _actionMap["createInputBreaker"] = _createInputBreakerAction;

    _createOutputBreakerAction = new QAction(makeIconFromTheme("document-new"), tr("Create output breaker"), this);
    _actionMap["createOutputBreaker"] = _createOutputBreakerAction;

    _rotateLeftAction = new QAction(makeIconFromTheme("object-rotate-left"), tr("Rotate Left"), this);
    _rotateLeftAction->setShortcut(QKeySequence("Left"));
    _actionMap["rotateLeft"] = _rotateLeftAction;

    _rotateRightAction = new QAction(makeIconFromTheme("object-rotate-right"), tr("Rotate Right"), this);
    _rotateRightAction->setShortcut(QKeySequence("Right"));
    _actionMap["rotateRight"] = _rotateRightAction;

    _zoomInAction = new QAction(makeIconFromTheme("zoom-in"), tr("Zoom in"), this);
    _zoomInAction->setShortcut(QKeySequence::ZoomIn);
    _actionMap["zoomIn"] = _zoomInAction;

    _zoomOutAction = new QAction(makeIconFromTheme("zoom-out"), tr("Zoom out"), this);
    _zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    _actionMap["zoomOut"] = _zoomOutAction;

    _zoomOriginalAction = new QAction(makeIconFromTheme("zoom-original"), tr("Normal size"), this);
    _zoomOriginalAction->setShortcut(QKeySequence("CTRL+0"));
    _actionMap["zoomOriginal"] = _zoomOriginalAction;

    _showAboutAction = new QAction(makeIconFromTheme("help-about"), tr("&About Pothos"), this);
    _showAboutAction->setStatusTip(tr("Information about this version of Pothos"));
    connect(_showAboutAction, SIGNAL(triggered(void)), this, SLOT(handleShowAbout(void)));

    _showAboutQtAction = new QAction(makeIconFromTheme("help-about"), tr("About &Qt"), this);
    _showAboutQtAction->setStatusTip(tr("Information about this version of QT"));
    connect(_showAboutQtAction, SIGNAL(triggered(void)), this, SLOT(handleShowAboutQt(void)));
}

void PothosGuiMainWindow::createMenus(void)
{
    _fileMenu = menuBar()->addMenu(tr("&File"));
    _menuMap["file"] = _fileMenu;
    _fileMenu->addAction(_newAction);
    _fileMenu->addAction(_openAction);
    _fileMenu->addAction(_saveAction);
    _fileMenu->addAction(_saveAsAction);
    _fileMenu->addAction(_saveAllAction);
    _fileMenu->addAction(_reloadAction);
    _fileMenu->addAction(_closeAction);
    _fileMenu->addSeparator();
    _fileMenu->addAction(_exitAction);

    _editMenu = menuBar()->addMenu(tr("&Edit"));
    _menuMap["edit"] = _editMenu;
    _editMenu->addAction(_undoAction);
    _editMenu->addAction(_redoAction);
    _editMenu->addSeparator();
    _editMenu->addAction(_cutAction);
    _editMenu->addAction(_copyAction);
    _editMenu->addAction(_pasteAction);
    _editMenu->addAction(_deleteAction);
    _editMenu->addSeparator();
    _editMenu->addAction(_selectAllAction);
    _editMenu->addSeparator();
    _editMenu->addAction(_createGraphPageAction);
    _editMenu->addAction(_renameGraphPageAction);
    _editMenu->addAction(_deleteGraphPageAction);
    _menuMap["moveGraphObjects"] = _editMenu->addMenu(makeIconFromTheme("transform-move"), tr("Move selected graph objects..."));
    _editMenu->addSeparator();
    _editMenu->addAction(_createInputBreakerAction);
    _editMenu->addAction(_createOutputBreakerAction);
    _editMenu->addSeparator();
    _editMenu->addAction(_rotateLeftAction);
    _editMenu->addAction(_rotateRightAction);
    _editMenu->addAction(_propertiesAction);

    _viewMenu = menuBar()->addMenu(tr("&View"));
    _menuMap["view"] = _viewMenu;
    _viewMenu->addAction(_remoteNodesDock->toggleViewAction());
    _viewMenu->addAction(_messageWindowDock->toggleViewAction());
    _viewMenu->addAction(_graphActionsDock->toggleViewAction());
    _viewMenu->addAction(_mainToolBar->toggleViewAction());
    _fileMenu->addSeparator();
    _viewMenu->addAction(_zoomInAction);
    _viewMenu->addAction(_zoomOutAction);
    _viewMenu->addAction(_zoomOriginalAction);

    _helpMenu = menuBar()->addMenu(tr("&Help"));
    _menuMap["help"] = _helpMenu;
    _helpMenu->addAction(_showAboutAction);
    _helpMenu->addAction(_showAboutQtAction);
}

void PothosGuiMainWindow::createMainToolBar(void)
{
    _mainToolBar = new QToolBar(tr("Main Tool Bar"), this);
    _mainToolBar->setObjectName("MainToolBar");
    this->addToolBar(_mainToolBar);

    _mainToolBar->addAction(_newAction);
    _mainToolBar->addAction(_openAction);
    _mainToolBar->addAction(_saveAction);
    _mainToolBar->addAction(_saveAsAction);
    _mainToolBar->addAction(_saveAllAction);
    _mainToolBar->addAction(_reloadAction);
    _mainToolBar->addAction(_closeAction);
    _mainToolBar->addSeparator();

    _mainToolBar->addAction(_zoomInAction);
    _mainToolBar->addAction(_zoomOutAction);
    _mainToolBar->addAction(_zoomOriginalAction);
    _mainToolBar->addSeparator();

    _mainToolBar->addAction(_undoAction);
    _mainToolBar->addAction(_redoAction);
    _mainToolBar->addSeparator();
    _mainToolBar->addAction(_cutAction);
    _mainToolBar->addAction(_copyAction);
    _mainToolBar->addAction(_pasteAction);
    _mainToolBar->addAction(_deleteAction);
    _mainToolBar->addSeparator();
    _mainToolBar->addAction(_selectAllAction);
    _mainToolBar->addSeparator();
    _mainToolBar->addAction(_rotateLeftAction);
    _mainToolBar->addAction(_rotateRightAction);
    _mainToolBar->addAction(_propertiesAction);
}

QWidget *makeMainWindow(QWidget *parent)
{
    return new PothosGuiMainWindow(parent);
}

#include "MainWindow.moc"

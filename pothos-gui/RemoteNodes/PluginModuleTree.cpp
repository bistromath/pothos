// Copyright (c) 2013-2014 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include "PothosGui.hpp"
#include <QTreeWidget>
#include <Pothos/Remote.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Plugin.hpp>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrent>
#include <Poco/Logger.h>
#include <map>

/***********************************************************************
 * recursive algorithm to create widget information
 **********************************************************************/
typedef std::map<std::string, std::vector<std::string>> ModMapType;

static void loadModuleMap(ModMapType &modMap, const Pothos::PluginRegistryInfoDump &dump)
{
    if (not dump.objectType.empty())
    {
        modMap[dump.modulePath].push_back(dump.pluginPath);
    }

    for (const auto &subInfo : dump.subInfo)
    {
        loadModuleMap(modMap, subInfo);
    }
}

/***********************************************************************
 * information aquisition
 **********************************************************************/
static ModMapType getRegistryDump(const Pothos::RemoteNode &node)
{
    ModMapType modMap;
    try
    {
        auto env = Pothos::RemoteNode(node).makeClient("info").makeEnvironment("managed");
        const auto dump = env->findProxy("Pothos/PluginRegistry").call<Pothos::PluginRegistryInfoDump>("dump");
        loadModuleMap(modMap, dump);
    }
    catch (const Pothos::Exception &ex)
    {
        poco_error_f2(Poco::Logger::get("PothosGui.PluginModuleTree"), "Failed to dump registry %s - %s", node.getUri(), ex.displayText());
    }
    return modMap;
}

/***********************************************************************
 * plugin module tree widget
 **********************************************************************/
class PluginModuleTree : public QTreeWidget
{
    Q_OBJECT
public:
    PluginModuleTree(QWidget *parent):
        QTreeWidget(parent),
        _watcher(new QFutureWatcher<ModMapType>(this))
    {
        QStringList columnNames;
        columnNames.push_back(tr("Plugin Path"));
        this->setColumnCount(columnNames.size());
        this->setHeaderLabels(columnNames);

        connect(
            _watcher, SIGNAL(finished(void)),
            this, SLOT(handleWatcherDone(void)));
    }

signals:
    void startLoad(void);
    void stopLoad(void);

private slots:

    void handeNodeInfoRequest(const Pothos::RemoteNode &node)
    {
        if (_watcher->isRunning()) return;
        while (this->topLevelItemCount() > 0) delete this->topLevelItem(0);
        _watcher->setFuture(QtConcurrent::run(std::bind(&getRegistryDump, node)));
        emit startLoad();
    }

    void handleWatcherDone(void)
    {
        const auto modMap = _watcher->result();

        for (const auto &entry : modMap)
        {
            std::string name = entry.first;
            if (name.empty()) name = "Builtin";
            auto modRoot = new QTreeWidgetItem(this, QStringList(QString::fromStdString(name)));
            const auto &pluginPaths = entry.second;
            modRoot->setExpanded(pluginPaths.size() < 21);
            for (size_t i = 0; i < pluginPaths.size(); i++)
            {
                new QTreeWidgetItem(modRoot, QStringList(QString::fromStdString(pluginPaths[i])));
            }
        }

        this->resizeColumnToContents(0);
        emit stopLoad();
    }

private:
    QFutureWatcher<ModMapType> *_watcher;
};

QWidget *makePluginModuleTree(QWidget *parent)
{
    return new PluginModuleTree(parent);
}

#include "PluginModuleTree.moc"

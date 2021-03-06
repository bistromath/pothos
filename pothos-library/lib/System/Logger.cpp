// Copyright (c) 2013-2014 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/System/Logger.hpp>
#include <Pothos/Plugin/Static.hpp> //static block
#include <Poco/Logger.h>
#include <Poco/Format.h>
#include <Poco/Environment.h>
#include <Poco/ConsoleChannel.h>
#include <Poco/NullChannel.h>
#include <Poco/SimpleFileChannel.h>
#include <Poco/FormattingChannel.h>
#include <Poco/PatternFormatter.h>
#include <Poco/Net/RemoteSyslogChannel.h>
#include <Poco/Net/RemoteSyslogListener.h>
#include <Poco/Net/DatagramSocket.h>

std::string Pothos::System::startSyslogListener(void)
{
    //find an available udp port
    Poco::Net::DatagramSocket sock;
    sock.bind(Poco::Net::SocketAddress("0.0.0.0:0"));
    const auto port = sock.address().port();
    sock.close();

    //create a new listener and feed it the root channel
    auto listener = new Poco::Net::RemoteSyslogListener(port);
    listener->addChannel(Poco::Logger::get("").getChannel());
    listener->open();

    //return the port number of the log service
    return listener->getProperty(Poco::Net::RemoteSyslogListener::PROP_PORT);
}

void Pothos::System::startSyslogForwarding(const std::string &addr)
{
    auto *ch = new Poco::Net::RemoteSyslogChannel(addr, ""/*empty name*/);
    Poco::Logger::get("").setChannel(ch);
}

void Pothos::System::setupDefaultLogging(void)
{
    const std::string logLevel = Poco::Environment::get("POTHOS_LOG_LEVEL", "notice");
    const std::string logChannel = Poco::Environment::get("POTHOS_LOG_CHANNEL", "color");
    const std::string logFile = Poco::Environment::get("POTHOS_LOG_FILE", "pothos.log");

    //set the logging level at the chosen root of logging inheritance
    Poco::Logger::get("").setLevel(logLevel);

    //create and set the channel with the type string specified
    Poco::Channel *channel = nullptr;
    if (logChannel == "null") channel = new Poco::NullChannel();
    else if (logChannel == "console") channel = new Poco::ConsoleChannel();
    else if (logChannel == "color") channel = new Poco::ColorConsoleChannel();
    else if (logChannel == "file" and not logFile.empty())
    {
        channel = new Poco::SimpleFileChannel();
        channel->setProperty("path", logFile);
    }
    if (channel == nullptr) return;

    //setup formatting
    Poco::Formatter *formatter = new Poco::PatternFormatter();
    formatter->setProperty("pattern", "%Y-%m-%d %H:%M:%S %s: %t");
    Poco::Channel *formattingChannel = new Poco::FormattingChannel(formatter, channel);
    Poco::Logger::get("").setChannel(formattingChannel);
}

pothos_static_block(pothosLoggingInit)
{
    Pothos::System::setupDefaultLogging();
}

#include <Pothos/Managed.hpp>
struct DummyLoggerClass{};

static auto managedLogger = Pothos::ManagedClass()
    .registerClass<DummyLoggerClass>()
    .registerStaticMethod(POTHOS_FCN_TUPLE(Pothos::System, startSyslogListener))
    .registerStaticMethod(POTHOS_FCN_TUPLE(Pothos::System, startSyslogForwarding))
    .registerStaticMethod(POTHOS_FCN_TUPLE(Pothos::System, setupDefaultLogging))
    .commit("Pothos/System/Logger");

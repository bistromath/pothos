// Copyright (c) 2013-2014 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include "PothosUtil.hpp"
#include <Pothos/Remote.hpp>
#include <Pothos/Plugin/Loader.hpp>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/SocketStream.h>
#include <Poco/Net/TCPServer.h>
#include <Poco/Process.h>
#include <Poco/URI.h>
#include <iostream>

class MyTCPServerConnection : public Poco::Net::TCPServerConnection
{
public:
    MyTCPServerConnection(const Poco::Net::StreamSocket &socket):
        Poco::Net::TCPServerConnection(socket)
    {
        return;
    }

    void run(void)
    {
        Poco::Net::SocketStream socketStream(this->socket());
        Pothos::RemoteServer::runHandler(socketStream, socketStream);
    }
};

class MyTCPServerConnectionFactory : public Poco::Net::TCPServerConnectionFactory
{
public:
    MyTCPServerConnectionFactory(void)
    {
        return;
    }

    Poco::Net::TCPServerConnection *createConnection(const Poco::Net::StreamSocket &socket)
    {
        return new MyTCPServerConnection(socket);
    }
};

struct MyTCPConnectionMonitor: public Poco::Runnable
{
    MyTCPConnectionMonitor(Poco::Net::TCPServer &server):
        running(true),
        server(server)
    {
        return;
    }

    virtual void run(void)
    {
        while (running)
        {
            Poco::Thread::sleep(100/*ms*/);
            if (server.currentConnections() == 0)
            {
                std::cerr << "Proxy server: No active connections - killing server" << std::endl;
                Poco::Process::kill(Poco::Process::id());
                return;
            }
        }
    }

    bool running;
    Poco::Net::TCPServer &server;
};

void PothosUtilBase::proxyServer(const std::string &, const std::string &uriStr)
{
    Pothos::init();

    //parse the URI
    const std::string defaultUri = "tcp://0.0.0.0:"+Pothos::RemoteServer::getLocatorPort();
    Poco::URI uri(uriStr.empty()?defaultUri:uriStr);
    const std::string &host = uri.getHost();
    const std::string &port = std::to_string(uri.getPort());
    if (uri.getScheme() != "tcp")
    {
        throw Pothos::Exception("PothosUtil::proxyServer("+uriStr+")", "unsupported URI scheme");
    }

    //create server socket
    Poco::Net::SocketAddress sa(host, port);
    Poco::Net::ServerSocket serverSocket(sa);
    Poco::Net::TCPServerConnectionFactory::Ptr factory(new MyTCPServerConnectionFactory());
    Poco::Net::TCPServer tcpServer(factory, serverSocket);

    //start the server
    std::cout << "Host: " << serverSocket.address().host().toString() << std::endl;
    std::cout << "Port: " << serverSocket.address().port() << std::endl;
    serverSocket.listen();
    tcpServer.start();

    //monitor active connections in monitor mode
    if (this->config().hasOption("requireActive"))
    {
        //create a TCP connection monitor thread
        MyTCPConnectionMonitor monitor(tcpServer);
        Poco::Thread monitorThread;
        monitorThread.start(monitor);

        //wait here until the term signal is received
        this->waitForTerminationRequest();

        //end the monitor thread
        monitor.running = false;
        monitorThread.join();
    }
    else
    {
        //wait here until the term signal is received
        this->waitForTerminationRequest();
    }
}

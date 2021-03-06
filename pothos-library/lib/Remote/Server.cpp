// Copyright (c) 2013-2014 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Remote.hpp>
#include <Pothos/System/Paths.hpp>
#include <Poco/Pipe.h>
#include <Poco/PipeStream.h>
#include <Poco/Process.h>
#include <Poco/StringTokenizer.h>
#include <Poco/URI.h>
#include <cassert>

std::string Pothos::RemoteServer::getLocatorPort(void)
{
    return "16415";
}

struct Pothos::RemoteServer::Impl
{
    Impl(const Poco::ProcessHandle &ph):
        ph(ph)
    {
        return;
    }

    ~Impl(void)
    {
        client = RemoteClient(); //reset
        Poco::Process::kill(ph.id());
    }

    Poco::ProcessHandle ph;
    std::string actualPort;
    RemoteClient client;
};

Pothos::RemoteServer::RemoteServer(void)
{
    assert(this->null());
}

Pothos::RemoteServer::RemoteServer(const std::string &uriStr)
{
    //validate the URI first
    if (not uriStr.empty()) try
    {
        RemoteNode node(uriStr); //validates
    }
    catch (const Exception &ex)
    {
        throw Pothos::RemoteClientError("Pothos::RemoteServer("+uriStr+")", ex);
    }

    //create args
    Poco::Process::Args args;
    args.push_back("--require-active");
    args.push_back("--proxy-server");
    args.push_back(uriStr);

    //launch
    Poco::Pipe outPipe, errPipe;
    Poco::Process::Env env;
    Poco::ProcessHandle ph(Poco::Process::launch(
        System::getPothosUtilExecutablePath(),
        args, nullptr, &outPipe, &errPipe, env));
    _impl.reset(new Impl(ph));

    //read port and close pipe
    Poco::PipeInputStream is(outPipe);
    while (is.good() and not is.eof())
    {
        char buff[1024]; is.getline(buff, 1024);
        const std::string line(buff, size_t(is.gcount()));
        if (line.empty()) continue;
        const Poco::StringTokenizer tok(line, " ");
        if (tok.count() >= 2 and tok[0] == "Port:")
        {
            _impl->actualPort = tok[1];
            break;
        }
    }
    outPipe.close();

    //An error spawning the process above could have caused a problem,
    //which can be detected when the parsing of the port fails.
    if (_impl->actualPort.empty())
    {
        _impl.reset(); //kills process

        //collect error message
        Poco::PipeInputStream ies(errPipe);
        const std::string errMsg = std::string(
            std::istreambuf_iterator<char>(ies),
            std::istreambuf_iterator<char>());

        throw Pothos::RemoteServerError("Pothos::RemoteServer("+uriStr+")", errMsg);
    }

    //Try to connect to the server.
    //Store an open connection within this server wrapper.
    {
        Poco::URI uri(uriStr);
        uri.setHost("localhost");
        uri.setPort(std::atoi(this->getActualPort().c_str()));
        _impl->client = RemoteClient(uri.toString());
    }
}

bool Pothos::RemoteServer::null(void) const
{
    return not _impl;
}

std::string Pothos::RemoteServer::getActualPort(void) const
{
    return _impl->actualPort;
}

#include <Pothos/Managed.hpp>

static auto managedRemoteServer = Pothos::ManagedClass()
    .registerConstructor<Pothos::RemoteServer>()
    .registerConstructor<Pothos::RemoteServer, std::string>()
    .registerMethod(POTHOS_FCN_TUPLE(Pothos::RemoteServer, getActualPort))
    .registerStaticMethod<std::iostream &>(POTHOS_FCN_TUPLE(Pothos::RemoteServer, runHandler))
    .registerStaticMethod<std::istream &, std::ostream &>(POTHOS_FCN_TUPLE(Pothos::RemoteServer, runHandler))
    .commit("Pothos/RemoteServer");

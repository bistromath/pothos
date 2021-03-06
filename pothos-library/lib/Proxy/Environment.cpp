// Copyright (c) 2013-2013 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Proxy/Exception.hpp>
#include <Pothos/Proxy/Environment.hpp>
#include <Pothos/Callable.hpp>
#include <Pothos/Plugin.hpp>

Pothos::ProxyEnvironment::Sptr Pothos::ProxyEnvironment::make(const std::string &name, const ProxyEnvironmentArgs &args)
{
    Sptr environment;
    try
    {
        auto plugin = Pothos::PluginRegistry::get(Pothos::PluginPath("/proxy/environment").join(name));
        auto callable = plugin.getObject().extract<Pothos::Callable>();
        environment = callable.call<Sptr>(args);
    }
    catch(const Exception &ex)
    {
        throw Pothos::ProxyEnvironmentFactoryError("Pothos::ProxyEnvironment::make("+name+")", ex);
    }
    return environment;
}

Pothos::ProxyEnvironment::~ProxyEnvironment(void)
{
    return;
}

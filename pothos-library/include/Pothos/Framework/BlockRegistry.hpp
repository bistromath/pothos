//
// Framework/BlockRegistry.hpp
//
// A BlockRegistry registers a block's factory function.
//
// Copyright (c) 2014-2014 Josh Blum
// SPDX-License-Identifier: BSL-1.0
//

#pragma once
#include <Pothos/Config.hpp>
#include <Pothos/Callable/Callable.hpp>
#include <string>
#include <memory>

namespace Pothos {

class Block; //forward declaration

/*!
 * The BlockRegistry class registers a block's factory function.
 * A BlockRegistry can be created at static initialization time
 * so that modules providing blocks will automatically register.
 * Usage example (put this at the bottom of your c++ source file)
 * static Pothos::BlockRegistry registerMyBlock("/my/factory/path", &MyBlock::make);
 */
class POTHOS_API BlockRegistry
{
public:

    /*!
     * Register a factory function into the plugin registry.
     * The resulting factory path will be /blocks/path.
     * Example: a path of /foo/bar will register to /blocks/foo/bar.
     *
     * Because this call is used at static initialization time,
     * it does not throw. However, registration errors are logged,
     * and the block will not be available at runtime.
     *
     * \param path the factory path begining with a slash ("/")
     * \param factory the bound factory function returning Block*
     */
    BlockRegistry(const std::string &path, const Callable &factory);

    /*!
     * Lookup a block factory in the plugin registry.
     * Path fallows the same rules as in the BlockRegistry constructor.
     * \throws PluginRegistryError if no factory registration is found
     * \param path the factory path begining with a slash ("/")
     * \return the factory function that was registered to path
     */
    static Callable lookup(const std::string &path);

};

} //namespace Pothos

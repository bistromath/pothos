// Copyright (c) 2013-2014 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Plugin.hpp>
#include <Pothos/Util/RingDeque.hpp>
#include <Pothos/Framework/BufferManager.hpp>
#include <cassert>

/***********************************************************************
 * generic buffer implementation
 **********************************************************************/
class GenericBufferManager :
    public Pothos::BufferManager,
    public std::enable_shared_from_this<GenericBufferManager>
{
public:
    GenericBufferManager(void)
    {
        return;
    }

    void init(const Pothos::BufferManagerArgs &args)
    {
        _readyBuffs.set_capacity(args.numBuffers);
        for (size_t i = 0; i < args.numBuffers; i++)
        {
            auto sharedBuff = Pothos::SharedBuffer::make(
                args.bufferSize, args.nodeAffinity);
            Pothos::ManagedBuffer buffer;
            buffer.reset(this->shared_from_this(), sharedBuff);
        }
    }

    bool empty(void) const
    {
        return _readyBuffs.empty();
    }

    const Pothos::ManagedBuffer &front(void) const
    {
        assert(not _readyBuffs.empty());
        return _readyBuffs.front();
    }

    void pop(const size_t /*numBytes*/)
    {
        assert(not _readyBuffs.empty());
        return _readyBuffs.pop_front();
    }

    void push(const Pothos::ManagedBuffer &buff)
    {
        assert(not _readyBuffs.full());
        _readyBuffs.push_back(buff);
    }

private:
    Pothos::Util::RingDeque<Pothos::ManagedBuffer> _readyBuffs;
};

/***********************************************************************
 * factory and registration
 **********************************************************************/
Pothos::BufferManager::Sptr makeGenericBufferManager(void)
{
    return std::make_shared<GenericBufferManager>();
}

pothos_static_block(pothosFrameworkRegisterGenericBufferManager)
{
    Pothos::PluginRegistry::addCall(
        "/framework/buffer_manager/generic",
        &makeGenericBufferManager);
}

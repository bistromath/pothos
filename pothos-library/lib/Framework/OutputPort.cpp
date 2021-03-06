// Copyright (c) 2014-2014 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include "Framework/OutputPortImpl.hpp"
#include "Framework/WorkerActor.hpp"

Pothos::OutputPort::OutputPort(OutputPortImpl *impl):
    _impl(impl),
    _index(-1),
    _elements(0),
    _totalElements(0),
    _totalMessages(0),
    _pendingElements(0)
{
    _impl->bufferManager = BufferManager::make("generic", BufferManagerArgs());
}

Pothos::OutputPort::~OutputPort(void)
{
    delete _impl;
}

void Pothos::OutputPort::popBuffer(const size_t numBytes)
{
    assert(_impl);
    _impl->bufferManager->pop(numBytes);
}

void Pothos::OutputPort::postLabel(const Label &label)
{
    assert(_impl);
    assert(_impl->actor != nullptr);
    _impl->actor->sendPortMessage(_impl->subscribers, label);
}

void Pothos::OutputPort::postMessage(const Object &message)
{
    assert(_impl);
    assert(_impl->actor != nullptr);
    _impl->actor->sendPortMessage(_impl->subscribers, message);
    _totalMessages++;
}

void Pothos::OutputPort::postBuffer(const BufferChunk &buffer)
{
    assert(_impl);
    auto &queue = _impl->postedBuffers;
    if (queue.full()) queue.set_capacity(queue.size()*2);
    queue.push_back(buffer);
}

#include <Pothos/Managed.hpp>

static auto managedOutputPort = Pothos::ManagedClass()
    .registerClass<Pothos::OutputPort>()
    .registerMethod(POTHOS_FCN_TUPLE(Pothos::OutputPort, index))
    .registerMethod(POTHOS_FCN_TUPLE(Pothos::OutputPort, name))
    .registerMethod(POTHOS_FCN_TUPLE(Pothos::OutputPort, dtype))
    .registerMethod(POTHOS_FCN_TUPLE(Pothos::OutputPort, buffer))
    .registerMethod(POTHOS_FCN_TUPLE(Pothos::OutputPort, elements))
    .registerMethod(POTHOS_FCN_TUPLE(Pothos::OutputPort, totalElements))
    .registerMethod(POTHOS_FCN_TUPLE(Pothos::OutputPort, totalMessages))
    .registerMethod(POTHOS_FCN_TUPLE(Pothos::OutputPort, produce))
    .registerMethod(POTHOS_FCN_TUPLE(Pothos::OutputPort, popBuffer))
    .registerMethod(POTHOS_FCN_TUPLE(Pothos::OutputPort, postLabel))
    .registerMethod(POTHOS_FCN_TUPLE(Pothos::OutputPort, postMessage))
    .registerMethod(POTHOS_FCN_TUPLE(Pothos::OutputPort, postBuffer))
    .commit("Pothos/OutputPort");

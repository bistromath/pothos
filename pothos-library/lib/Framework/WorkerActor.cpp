// Copyright (c) 2014-2014 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include "Framework/WorkerActor.hpp"
#include <Poco/Logger.h>
#include <cassert>

/***********************************************************************
 * pre-work
 **********************************************************************/
bool Pothos::WorkerActor::preWorkTasks(void)
{
    const size_t BIG = (1 << 30);

    bool allOutputsReady = true;
    bool allInputsReady = true;
    bool hasInputMessage = false;

    //////////////// output state calculation ///////////////////
    workInfo.minOutElements = BIG;
    workInfo.minAllOutElements = BIG;
    for (auto &entry : this->outputs)
    {
        auto &port = *entry.second;
        if (port._impl->bufferManager->empty()) port._buffer = BufferChunk();
        else port._buffer = port._impl->bufferManager->front();
        port._elements = port._buffer.length/port.dtype().size();
        if (port._elements == 0) allOutputsReady = false;
        port._pendingElements = 0;
        if (port.index() != -1)
        {
            assert(workInfo.outputPointers.size() > size_t(port.index()));
            workInfo.outputPointers[port.index()] = port._buffer.as<void *>();
            workInfo.minOutElements = std::min(workInfo.minOutElements, port._elements);
        }
        workInfo.minAllOutElements = std::min(workInfo.minAllOutElements, port._elements);
    }

    //////////////// input state calculation ///////////////////
    workInfo.minInElements = BIG;
    workInfo.minAllInElements = BIG;
    for (auto &entry : this->inputs)
    {
        auto &port = *entry.second;
        const size_t reserveBytes = port._reserveElements*port.dtype().size();
        port._impl->bufferAccumulator.require(reserveBytes);
        port._buffer = port._impl->bufferAccumulator.front();
        port._elements = port._buffer.length/port.dtype().size();
        if (port._elements < port._reserveElements) allInputsReady = false;
        if (not port._impl->asyncMessages.empty()) hasInputMessage = true;
        port._pendingElements = 0;
        port._labelIter = port._impl->inlineMessages;
        if (port.index() != -1)
        {
            assert(workInfo.inputPointers.size() > size_t(port.index()));
            workInfo.inputPointers[port.index()] = port._buffer.as<const void *>();
            workInfo.minInElements = std::min(workInfo.minInElements, port._elements);
        }
        workInfo.minAllInElements = std::min(workInfo.minAllInElements, port._elements);
    }

    //calculate overall minimums
    workInfo.minElements = std::min(workInfo.minInElements, workInfo.minOutElements);
    workInfo.minAllElements = std::min(workInfo.minAllInElements, workInfo.minAllOutElements);

    //arbitrary time, but its small
    workInfo.maxTimeoutNs = 1000000; //1 millisecond

    return allOutputsReady and (allInputsReady or hasInputMessage);
}

/***********************************************************************
 * post-work
 **********************************************************************/
void Pothos::WorkerActor::postWorkTasks(void)
{
    ///////////////////// input handling ////////////////////////

    unsigned long long bytesConsumed = 0;
    unsigned long long msgsConsumed = 0;

    for (auto &entry : this->inputs)
    {
        auto &port = *entry.second;
        const size_t bytes = port._pendingElements*port.dtype().size();
        bytesConsumed += bytes;
        msgsConsumed += port._totalMessages;

        //set the buffer length, send it, pop from manager, clear reference
        if (bytes != 0)
        {
            port._buffer = BufferChunk(); //clear reference
            port._impl->bufferAccumulator.pop(bytes);
        }

        //move consumed elements into total
        port._totalElements += port._pendingElements;

        //propagate labels and delete old
        size_t numLabels = 0;
        auto &allLabels = port._impl->inlineMessages;
        for (size_t i = 0; i < allLabels.size(); i++)
        {
            if (allLabels[i].index < port.totalElements()) numLabels++;
            else break;
        }
        if (numLabels != 0)
        {
            LabelIteratorRange iter(allLabels.begin(), allLabels.begin()+numLabels);
            try
            {
                block->propagateLabels(&port, iter);
            }
            //TODO need to identify the block by name
            catch (const Pothos::Exception &ex)
            {
                poco_error_f1(Poco::Logger::get("Pothos.WorkerActor.propagateLabels"),
                    "Block TODO threw in overloaded call to propagateLabels() - %s", ex.displayText());
            }
            catch (const std::exception &ex)
            {
                poco_error_f1(Poco::Logger::get("Pothos.WorkerActor.propagateLabels"),
                    "Block TODO threw in overloaded call to propagateLabels() - %s", std::string(ex.what()));
            }
            catch (...)
            {
                poco_error_f1(Poco::Logger::get("Pothos.WorkerActor.propagateLabels"),
                    "Block TODO threw in overloaded call to propagateLabels() - %s", std::string("unknown"));
            }
            allLabels.erase(allLabels.begin(), allLabels.begin()+numLabels);
        }
    }

    //update consumption stats, bytes are incremental, messages cumulative
    if (bytesConsumed != 0 or this->workStats.msgsConsumed != msgsConsumed)
    {
        this->workStats.ticksLastConsumed = Theron::Detail::Clock::GetTicks();
    }
    this->workStats.bytesConsumed += bytesConsumed;
    this->workStats.msgsConsumed = msgsConsumed;

    ///////////////////// output handling ////////////////////////
    //Note: output buffer production must come after propagateLabels()

    unsigned long long bytesProduced = 0;
    unsigned long long msgsProduced = 0;

    for (auto &entry : this->outputs)
    {
        auto &port = *entry.second;
        const size_t bytes = port._pendingElements*port.dtype().size();
        bytesProduced += bytes;
        msgsProduced += port._totalMessages;

        //set the buffer length, send it, pop from manager, clear reference
        if (bytes != 0)
        {
            auto buffer = port._buffer;
            port._buffer = BufferChunk(); //clear reference
            buffer.length = bytes;
            port._impl->bufferManager->pop(buffer.length);
            this->sendPortMessage(port._impl->subscribers, buffer);
        }

        //send the external buffers in the queue
        while (not port._impl->postedBuffers.empty())
        {
            auto &buffer = port._impl->postedBuffers.front();
            bytesProduced += buffer.length;
            port._totalElements += buffer.length/port.dtype().size();
            this->sendPortMessage(port._impl->subscribers, buffer);
            port._impl->postedBuffers.pop_front();
        }

        //move produced elements into total
        port._totalElements += port._pendingElements;
    }

    //update production stats, bytes are incremental, messages cumulative
    if (bytesProduced != 0 or this->workStats.msgsProduced != msgsProduced)
    {
        this->workStats.ticksLastProduced = Theron::Detail::Clock::GetTicks();
    }
    this->workStats.bytesProduced += bytesProduced;
    this->workStats.msgsProduced = msgsProduced;

    //TODO bump for blocks that did work
    //actually probably just blocks that marked a fail and the framework tried to solve it with buffer popping or accumulation

    //always bump for sources
    //if (this->inputs.size() == 0)
    this->bump();
}

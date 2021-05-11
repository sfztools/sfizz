// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "SfizzVstUpdates.h"

template <class T>
IPtr<Vst::IMessage> IConvertibleToMessage<T>::convertToMessage(Vst::ComponentBase* sender) const
{
    IPtr<Vst::IMessage> message = Steinberg::owned(sender->allocateMessage());
    if (!message)
        return nullptr;
    message->setMessageID(static_cast<const T*>(this)->isA());
    if (!saveToAttributes(message->getAttributes()))
        return nullptr;
    return message;
}

template <class T>
IPtr<T> IConvertibleToMessage<T>::createFromMessage(Vst::IMessage& message)
{
    IPtr<T> object;
    if (!strcmp(T::getFClassID(), message.getMessageID())) {
        object = Steinberg::owned(new T);
        if (!object->loadFromAttributes(message.getAttributes()))
            object = nullptr;
    }
    return object;
}

template <class T>
bool IConvertibleToMessage<T>::convertFromMessage(Vst::IMessage& message)
{
    bool success = false;
    if (!strcmp(T::getFClassID(), message.getMessageID()))
        success = loadFromAttributes(message.getAttributes());
    return success;
}

// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

#pragma once

// Fix syntax highlighting
#ifdef __INTELLISENSE__
#    include "notifications/NotificationManager.h"
#endif
#include "RTTR_Assert.h"
#include <algorithm>
#include <stdexcept>
#include <utility>

template<class T_Note>
class NotificationManager::CallbackUnregistrar
{
    NotificationManager& noteMgr;

public:
    explicit CallbackUnregistrar(NotificationManager& noteMgr) : noteMgr(noteMgr) {}
    void operator()(void* subscription)
    {
        if(!subscription)
            return; // Nothing to do
        auto* callback = static_cast<NoteCallback<T_Note>*>(subscription);
        // Check if we are still subscribed
        if(callback->IsSubscribed())
            noteMgr.unsubscribe<T_Note>(callback);
        delete callback;
    }
};

struct NotificationManager::NoteCallbackBase
{
    NoteCallbackBase() : isSubscribed(true) {}
    void SetUnsubscribed() { isSubscribed = false; }
    bool IsSubscribed() const { return isSubscribed; }

private:
    /// Used internally to detect unsubscribed callbacks.
    /// This allows unsubscribing all callbacks when the manager is destroyed
    bool isSubscribed;
};

template<class T_Note>
struct NotificationManager::NoteCallback : NoteCallbackBase
{
    using Callback = std::function<void(const T_Note&)>;
    explicit NoteCallback(Callback callback) : execute(std::move(callback)) {}
    const Callback execute;
};

inline NotificationManager::NotificationManager() : isPublishing(false) {}

inline NotificationManager::~NotificationManager()
{
    RTTR_Assert(!isPublishing);
    // Unsubscribe all callbacks so we don't get accesses to this class after destruction
    for(auto& itSubscribers : noteId2Subscriber)
        for(auto& itCallback : itSubscribers.second)
            static_cast<NoteCallbackBase*>(itCallback)->SetUnsubscribed();
}

inline void NotificationManager::unsubscribe(Subscription& subscription)
{
    // We can simply call reset as this calls the deleter which releases the subscription
    subscription.reset();
}

template<class T_Note>
Subscription NotificationManager::subscribe(std::function<void(const T_Note&)> callback)
{
    if(isPublishing)
        throw std::runtime_error("Cannot subscribe during publishing of messages");
    auto* subscriber = new NoteCallback<T_Note>(std::move(callback));
    noteId2Subscriber[T_Note::getNoteId()].push_back(subscriber);
    return Subscription(subscriber, CallbackUnregistrar<T_Note>(*this));
}

template<class T_Note>
void NotificationManager::unsubscribe(NoteCallback<T_Note>* callback)
{
    if(!callback->IsSubscribed())
        return;
    if(isPublishing)
        throw std::runtime_error("Cannot unsubscribe during publishing of messages");
    CallbackList& callbacks = noteId2Subscriber[T_Note::getNoteId()];
    auto itEl = std::find(callbacks.begin(), callbacks.end(), callback);
    if(itEl != callbacks.end())
        callbacks.erase(itEl);
    else
        RTTR_Assert(false);
    // Set only the flag. Actual deletion of the callback must be handled by the shared_ptr deleter
    callback->SetUnsubscribed();
}

template<class T_Note>
void NotificationManager::publish(const T_Note& notification)
{
    isPublishing = true;
    for(auto* it : noteId2Subscriber[T_Note::getNoteId()])
        static_cast<NoteCallback<T_Note>*>(it)->execute(notification);
    isPublishing = false;
}

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

struct NotificationManager::NoteCallbackBase
{
    NoteCallbackBase() noexcept : isSubscribed(true) {}
    void SetUnsubscribed() noexcept { isSubscribed = false; }
    bool IsSubscribed() const noexcept { return isSubscribed; }

private:
    /// Used internally to detect unsubscribed callbacks.
    /// This allows unsubscribing all callbacks when the manager is destroyed
    bool isSubscribed;
};

template<class T_Note>
struct NotificationManager::NoteCallback final : NoteCallbackBase
{
    using Callback = std::function<void(const T_Note&)>;
    explicit NoteCallback(Callback callback) noexcept : execute(std::move(callback)) {}
    const Callback execute;
};

inline NotificationManager::~NotificationManager()
{
    RTTR_Assert(!isPublishing);
    // Unsubscribe all callbacks so we don't get accesses to this class after destruction
    for(const auto& subscribers : noteId2Subscriber)
        for(void* callback : *subscribers.second)
        {
            if(callback)
                static_cast<NoteCallbackBase*>(callback)->SetUnsubscribed();
        }
}

inline void NotificationManager::unsubscribe(Subscription& subscription) noexcept
{
    // We can simply call reset as this calls the deleter which releases the subscription
    subscription.reset();
}

template<class T_Note>
Subscription NotificationManager::subscribe(std::function<void(const T_Note&)> callback) noexcept
{
    auto* subscriber = new NoteCallback<T_Note>(std::move(callback));
    noteId2Subscriber[T_Note::getNoteId()]->push_back(subscriber);

    return Subscription(subscriber, [this](void* subscription) {
        RTTR_Assert(subscription); // As we use this in a shared_ptr, this can never be nullptr
        auto* callback = static_cast<NoteCallback<T_Note>*>(subscription);
        if(callback->IsSubscribed())
            this->unsubscribe<T_Note>(callback);
        delete callback;
    });
}

template<class T_Note>
void NotificationManager::unsubscribe(NoteCallback<T_Note>* callback) noexcept
{
    RTTR_Assert(callback->IsSubscribed());
    CallbackList& callbacks = *noteId2Subscriber[T_Note::getNoteId()];
    auto itEl = std::find(callbacks.begin(), callbacks.end(), callback);
    RTTR_Assert(itEl != callbacks.end());
    // We can't modify the list while iterating over it
    if(isPublishing)
        *itEl = nullptr;
    else
        callbacks.erase(itEl);
    // Set only the flag. Actual deletion of the callback must be handled by the shared_ptr deleter
    callback->SetUnsubscribed();
}

template<class T_Note>
void NotificationManager::publish(const T_Note& notification)
{
    ++isPublishing;
    try
    {
        // Note: We have to support subscribe and unsubscribe during the execute call.
        // - noteId2Subscriber must have elements with stable adresses even on insertion (subscribe)
        // - CallbackList must not invalidate any iterator on insert
        // - Since erase usually invalidates iterators unsubscribe only clears the pointer which we have to handle here
        CallbackList& callbacks = *noteId2Subscriber[T_Note::getNoteId()];
        for(auto it = callbacks.begin(); it != callbacks.end();)
        {
            if(*it)
            {
                static_cast<NoteCallback<T_Note>*>(*it)->execute(notification);
                ++it;
            } else
                it = callbacks.erase(it);
        }
        RTTR_Assert(isPublishing);
        --isPublishing;
    } catch(...)
    {
        RTTR_Assert(isPublishing);
        --isPublishing;
        throw;
    }
}

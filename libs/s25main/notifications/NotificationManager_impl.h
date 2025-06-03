// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// Fix syntax highlighting
#ifdef __INTELLISENSE__
#    include "notifications/NotificationManager.h"
#endif
#include "RTTR_Assert.h"
#include "helpers/containerUtils.h"
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
    // Unsubscribe all callbacks so we don't get accesses to this class after destruction
    for(const auto& subscribers : noteId2Subscriber)
    {
        RTTR_Assert(!subscribers.second.isPublishing);
        for(void* callback : subscribers.second.callbacks)
        {
            if(callback)
                static_cast<NoteCallbackBase*>(callback)->SetUnsubscribed();
        }
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
    noteId2Subscriber[T_Note::getNoteId()].callbacks.push_back(subscriber);

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
    Subscribers& subs = noteId2Subscriber[T_Note::getNoteId()];
    CallbackList& callbacks = subs.callbacks;
    auto itEl = helpers::find(callbacks, callback);
    RTTR_Assert(itEl != callbacks.end());
    // We can't modify the list while iterating over it
    if(subs.isPublishing)
        *itEl = nullptr;
    else
        callbacks.erase(itEl);
    // Set only the flag. Actual deletion of the callback must be handled by the shared_ptr deleter
    callback->SetUnsubscribed();
}

template<class T_Note>
void NotificationManager::publish(const T_Note& notification)
{
    Subscribers& subs = noteId2Subscriber[T_Note::getNoteId()];
    ++subs.isPublishing;
    try
    {
        // Note: We have to support subscribe and unsubscribe during the execute call.
        // - noteId2Subscriber must have elements with stable adresses even on insertion (subscribe)
        // - CallbackList must not invalidate any references on insert
        // - Since erase usually invalidates iterators unsubscribe only clears the pointer which we have to handle here
        CallbackList& callbacks = subs.callbacks;
        bool hasEmpty = false;
        for(unsigned i = 0; i < callbacks.size(); ++i) // NOLINT(modernize-loop-convert)
        {
            auto* cb = static_cast<NoteCallback<T_Note>*>(callbacks[i]);
            if(cb)
                cb->execute(notification);
            else
                hasEmpty = true;
        }
        RTTR_Assert(subs.isPublishing);
        --subs.isPublishing;
        if(!subs.isPublishing && hasEmpty)
            helpers::erase(callbacks, nullptr);
    } catch(...)
    {
        RTTR_Assert(subs.isPublishing);
        --subs.isPublishing;
        throw;
    }
}

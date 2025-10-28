// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "notifications/Subscription.h"
#include <cstdint>
#include <deque>
#include <functional>
#include <cstdint>
#include <unordered_map>

class NotificationManager
{
    /// Non-template base class for callbacks
    struct NoteCallbackBase;
    /// Type of the callback for a Notification(Note)
    template<class T_Note>
    struct NoteCallback;

public:
    ~NotificationManager();

    /// Subscribe to a specific notification.
    /// Unsubscribes when the subscription has no references left
    template<class T_Note>
    Subscription subscribe(std::function<void(const T_Note&)> callback) noexcept;
    /// Manually unsubscribes the callback
    static void unsubscribe(Subscription& subscription) noexcept;
    /// Call the registred callbacks for the note
    template<class T_Note>
    void publish(const T_Note& notification);

private:
    /// We cannot store the real type of the callback in C++ (no mixed type list) so we store it as a void*
    /// and use a cast based on the NoteId
    using CallbackList = std::deque<void*>;
    struct Subscribers
    {
        CallbackList callbacks;
        /// >0 when we are in the publish method
        unsigned isPublishing = 0;
    };
    std::unordered_map<uint32_t, Subscribers> noteId2Subscriber;

    template<class T_Note>
    void unsubscribe(NoteCallback<T_Note>* callback) noexcept;
};

#include "notifications/NotificationManager_impl.h"

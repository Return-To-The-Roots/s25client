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

#include "notifications/Subscription.h"
#include <deque>
#include <functional>
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
    std::unordered_map<uint32_t, CallbackList> noteId2Subscriber;
    /// >0 when we are in the publish method
    unsigned isPublishing = 0;

    template<class T_Note>
    void unsubscribe(NoteCallback<T_Note>* callback) noexcept;
};

#include "notifications/NotificationManager_impl.h"

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

// Fix syntax highlighting
#ifdef __INTELLISENSE__
#include "notifications/NotificationManager.h"
#endif
#include <stdexcept>

template<class T_Note>
class NotificationManager::CallbackUnregistrar
{
    NotificationManager& noteMgr;

public:
    explicit CallbackUnregistrar(NotificationManager& noteMgr) : noteMgr(noteMgr) {}
    void operator()(void* subscribtion)
    {
        if(!subscribtion)
            return; // Nothing to do
        NoteCallback<T_Note>* callback = static_cast<NoteCallback<T_Note>*>(subscribtion);
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
    typedef boost::function<void(const T_Note&)> Callback;
    explicit NoteCallback(Callback callback) : execute(callback) {}
    const Callback execute;
};

NotificationManager::NotificationManager() : isPublishing(false) {}

NotificationManager::~NotificationManager()
{
    RTTR_Assert(!isPublishing);
    // Unsubscribe all callbacks so we don't get accesses to this class after destruction
    for(SubscriberMap::iterator itSubscribers = noteId2Subscriber.begin(); itSubscribers != noteId2Subscriber.end(); ++itSubscribers)
        for(CallbackList::iterator itCallback = itSubscribers->second.begin(); itCallback != itSubscribers->second.end(); ++itCallback)
            static_cast<NoteCallbackBase*>(*itCallback)->SetUnsubscribed();
}

inline void NotificationManager::unsubscribe(Subscribtion& subscription)
{
    // We can simply call reset as this calls the deleter which releases the subscription
    subscription.reset();
}

template<class T_Note>
Subscribtion NotificationManager::subscribe(boost::function<void(T_Note)> callback)
{
    if(isPublishing)
        throw std::runtime_error("Cannot subscribe during publishing of messages");
    NoteCallback<T_Note>* subscriber = new NoteCallback<T_Note>(callback);
    noteId2Subscriber[T_Note::getNoteId()].push_back(subscriber);
    return Subscribtion(subscriber, CallbackUnregistrar<T_Note>(*this));
}

template<class T_Note>
void NotificationManager::unsubscribe(NoteCallback<T_Note>* callback)
{
    if(!callback->IsSubscribed())
        return;
    if(isPublishing)
        throw std::runtime_error("Cannot unsubscribe during publishing of messages");
    CallbackList& callbacks = noteId2Subscriber[T_Note::getNoteId()];
    CallbackList::iterator itEl = std::find(callbacks.begin(), callbacks.end(), callback);
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
    CallbackList& callbacks = noteId2Subscriber[T_Note::getNoteId()];
    for(CallbackList::const_iterator it = callbacks.begin(); it != callbacks.end(); ++it)
        static_cast<NoteCallback<T_Note>*>(*it)->execute(notification);
    isPublishing = false;
}

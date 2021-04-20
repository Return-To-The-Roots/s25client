// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <list>

class IDataChangedListener;

class DataChangedObservable
{
    std::list<IDataChangedListener*> listeners;

public:
    void AddListener(IDataChangedListener* listener);
    void RemoveListener(IDataChangedListener* listener);

protected:
    virtual ~DataChangedObservable();
    /// Notifies all registered listeners
    /// ID=0 means, this is removed and should no longer be referenced
    void NotifyListeners(unsigned changeId);
};

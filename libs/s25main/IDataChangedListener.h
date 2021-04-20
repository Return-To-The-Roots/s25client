// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

/// Interface for a listener that gets notifed on specific changes
class IDataChangedListener
{
protected:
    ~IDataChangedListener() = default;

public:
    /// Handles a change, changeId is the type of the change (object dependent)
    virtual void OnChange(unsigned changeId) = 0;
};

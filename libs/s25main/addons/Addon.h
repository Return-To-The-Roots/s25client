// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "const_addons.h"
#include <memory>
#include <string>

class Window;
class Addon;

class AddonGui
{
public:
    AddonGui(const Addon& addon, Window& window, bool readonly);
    virtual ~AddonGui() = default;
    virtual void setStatus(Window& window, unsigned status) = 0;
    virtual unsigned getStatus(const Window& window) = 0;
};

/**
 *  Addon baseclass
 */
class Addon
{
public:
    Addon(const AddonId id, AddonGroup groups, std::string name, std::string description, unsigned default_status)
        : id_(id), groups_(groups), name_(std::move(name)), description_(std::move(description)),
          defaultStatus_(default_status)
    {}
    virtual ~Addon() = default;

    virtual std::unique_ptr<AddonGui> createGui(Window& window, bool readonly) const = 0;

    AddonId getId() const { return id_; }
    AddonGroup getGroups() const { return groups_; }
    std::string getName() const { return name_; }
    std::string getDescription() const { return description_; }
    unsigned getDefaultStatus() const { return defaultStatus_; }
    virtual unsigned getNumOptions() const = 0;

private:
    AddonId id_;
    AddonGroup groups_;
    std::string name_;
    std::string description_;
    unsigned defaultStatus_;
};

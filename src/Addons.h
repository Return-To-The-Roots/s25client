// $Id: Addons.h 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef ADDONS_H_INCLUDED
#define ADDONS_H_INCLUDED

#pragma once

#include "const_addons.h"

class Window;

///////////////////////////////////////////////////////////////////////////////
/**
 *  Addon baseclass
 *
 *  @author FloSoft
 */
class Addon
{
    public:
        Addon(const AddonId id, const unsigned int groups, const std::string& name, const std::string& description, const unsigned int default_status)
            : id(id), groups(groups), name(name), description(description), default_status(default_status)  {   }
        virtual ~Addon() {  }

        virtual void hideGui(Window* window, unsigned int id) const;
        virtual void createGui(Window* window, unsigned int id, unsigned short& y, bool readonly, unsigned int status) const;
        virtual void setGuiStatus(Window* window, unsigned int id, unsigned int status) const { }

        virtual unsigned int getGuiStatus(Window* window, unsigned int id, bool& failed) const
        {
            failed = false;
            return getDefaultStatus();
        }

        AddonId getId() const { return id; }
        unsigned int getGroups() const { return (ADDONGROUP_ALL | groups); }
        std::string getName() const { return name; }
        std::string getDescription() const { return description; }
        unsigned int getDefaultStatus() const { return default_status; }

    private:
        AddonId id;
        unsigned int groups;
        std::string name;
        std::string description;
        unsigned int default_status;
};

///////////////////////////////////////////////////////////////////////////////
/**
 *  Addon baseclass for option-list addons
 *  first option added will be the default one
 *
 *  @author FloSoft
 */
class AddonList : public Addon
{
    public:
        AddonList(const AddonId id, const unsigned int groups, const std::string& name, const std::string& description, const unsigned int default_status)
            : Addon(id, groups, name, description, default_status) { }

        virtual void hideGui(Window* window, unsigned int id) const;
        virtual void createGui(Window* window, unsigned int id, unsigned short& y, bool readonly, unsigned int status) const;
        virtual void setGuiStatus(Window* window, unsigned int id, unsigned int status) const;
        virtual unsigned int getGuiStatus(Window* window, unsigned int id, bool& failed) const;

    protected:
        void removeOptions()
        {
            options.clear();
        }

        void addOption(const std::string& name)
        {
            if(std::find(options.begin(), options.end(), name) == options.end())
                options.push_back(name);
        }

    private:
        std::vector<std::string> options;
};

///////////////////////////////////////////////////////////////////////////////
/**
 *  Addon baseclass for boolean addons
 *
 *  @author FloSoft
 */
class AddonBool : public Addon
{
    public:
        AddonBool(const AddonId id, const unsigned int groups, const std::string& name, const std::string& description, const unsigned int default_status)
            : Addon(id, groups, name, description, default_status) { }

        virtual void hideGui(Window* window, unsigned int id) const;
        virtual void createGui(Window* window, unsigned int id, unsigned short& y, bool readonly, unsigned int status) const;
        virtual void setGuiStatus(Window* window, unsigned int id, unsigned int status) const;
        virtual unsigned int getGuiStatus(Window* window, unsigned int id, bool& failed) const;
};

#endif // !ADDONS_H_INCLUDED

// Copyright (C) 2017 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "DescriptionContainer.h"
#include "EdgeDesc.h"
#include "LandscapeDesc.h"
#include "TerrainDesc.h"
#include <stdexcept>

struct GameDataError : public std::runtime_error
{
    GameDataError(const std::string& desc) : std::runtime_error("Invalid game data: " + desc) {}
};

struct GameDataLoadError : public std::runtime_error
{
    GameDataLoadError(const std::string& desc) : std::runtime_error("Failed to load game data: " + desc) {}
};

struct WorldDescription
{
    WorldDescription();
    ~WorldDescription();
    DescriptionContainer<LandscapeDesc> landscapes;
    DescriptionContainer<EdgeDesc> edges;
    DescriptionContainer<TerrainDesc> terrain;
    // Convenience accessors
    template<class T>
    const T& get(DescIdx<T> idx) const
    {
        return getContainer<T>().get(idx);
    }
    template<class T>
    const DescriptionContainer<T>& getContainer() const;
};

template<>
inline const DescriptionContainer<LandscapeDesc>& WorldDescription::getContainer() const
{
    return landscapes;
}

template<>
inline const DescriptionContainer<EdgeDesc>& WorldDescription::getContainer() const
{
    return edges;
}

template<>
inline const DescriptionContainer<TerrainDesc>& WorldDescription::getContainer() const
{
    return terrain;
}

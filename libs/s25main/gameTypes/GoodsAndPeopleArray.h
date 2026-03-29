// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "GoodTypes.h"
#include "JobTypes.h"
#include "helpers/EnumArray.h"

/// Array for people with typed accessors
template<typename T>
struct PeopleArray
{
    helpers::EnumArray<T, Job> people = {};
    /// Subset of the respective soldier job count that have armor
    /// Must always be in the range [0,Maximum number of soldiers of this type]
    helpers::EnumArray<T, ArmoredSoldier> armoredSoldiers = {};

    const T& operator[](Job job) const { return people[job]; }
    T& operator[](Job job) { return people[job]; }
    T& operator[](ArmoredSoldier soldier) { return armoredSoldiers[soldier]; }
    const T& operator[](ArmoredSoldier soldier) const { return armoredSoldiers[soldier]; }
};

template<typename T>
struct GoodsArray
{
    helpers::EnumArray<T, GoodType> goods = {};
    const T& operator[](GoodType good) const { return goods[good]; }
    T& operator[](GoodType good) { return goods[good]; }
};

/// Combined array for goods and people with typed accessors
template<typename T>
struct GoodsAndPeopleArray : PeopleArray<T>, GoodsArray<T>
{
    using PeopleArray<T>::operator[];
    using GoodsArray<T>::operator[];
};

/// Raw count of goods
using GoodCounts = GoodsArray<unsigned>;
/// Raw count of people
using PeopleCounts = PeopleArray<unsigned>;
/// Raw count of goods and people
using GoodsAndPeopleCounts = GoodsAndPeopleArray<unsigned>;

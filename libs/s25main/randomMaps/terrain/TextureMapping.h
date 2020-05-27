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

#ifndef TextureMapping_h__
#define TextureMapping_h__

#include "randomMaps/terrain/TextureType.h"

class TextureMapping
{
public:
    TextureMapping() {}
    
    virtual ~TextureMapping() {}
    virtual unsigned char Get(TextureType texture) = 0;
    virtual unsigned char GetType() = 0;
};

class GreenlandMapping : public TextureMapping
{
public:
    GreenlandMapping() {}
    ~GreenlandMapping() {}
    
    unsigned char Get(TextureType texture);
    unsigned char GetType();
};

class WastelandMapping : public TextureMapping
{
public:
    WastelandMapping() {}
    ~WastelandMapping() {}
    
    unsigned char Get(TextureType texture);
    unsigned char GetType();
};

class WinterMapping : public TextureMapping
{
public:
    WinterMapping() {}
    ~WinterMapping() {}
    
    unsigned char Get(TextureType texture);
    unsigned char GetType();
};

#endif // TextureMapping_h__

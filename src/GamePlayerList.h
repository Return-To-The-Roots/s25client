// $Id: GamePlayerList.h 6458 2010-05-31 11:38:51Z FloSoft $
//
// Copyright (c) 2005-2008 Settlers Freaks (sf-team at siedler25.org)
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2, or (at your option) any
// later version.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
#ifndef GAMEPLAYERLIST_H_INCLUDED
#define GAMEPLAYERLIST_H_INCLUDED


#include "SerializableArray.h"
#include "GameClientPlayer.h"
#include "GameServerPlayer.h"

class GamePlayerList : public SerializableArray<unsigned>
{
};

class GameClientPlayerList : public SerializableArray<GameClientPlayer>
{
};

class GameServerPlayerList : public SerializableArray<GameServerPlayer>
{
};

#endif // GAMEPLAYERLIST_H_INCLUDED

// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef FramesInfo_h__
#define FramesInfo_h__

/// Struct that stores information about the frames, like GF status...
struct FramesInfo
{
public:
    FramesInfo();
    void Clear();
    /// Changes the GF length to GFLengthNew and adapts the NWF length accordingly
    void ApplyNewGFLength();

    /// Current GameFrame (GF) (from start of the game)
    unsigned gf_nr;
    /// Lenght of one GF in ms (~ 1/speed of the game)
    unsigned gf_length;
    /// New length of a GF (applied on next NWF)
    unsigned gfLenghtNew;
    /// New length of a GF (applied on second next NWF)
    unsigned gfLenghtNew2;
    /// Length of a NWF (network frame) in GFs
    unsigned nwf_length;
    /// Time since last GF in ms (valid range: [0, gfLength) )
    unsigned frameTime;
    /// Timestamp of the last processed GF (--> FrameTime = CurrentTime - LastTime (except for lags) )
    unsigned lastTime;
    /// True if the game is paused (no processing of GFs)
    bool isPaused;
};

/// Same as FramesInfo but with additional data that is only meaningfull for the client
struct FramesInfoClient: public FramesInfo
{
public:
    FramesInfoClient();
    void Clear();

    /// Requested length of GF (for multiple changes between a NWF)
    unsigned gfLengthReq;
    /// Number of the GF that the server acknowledged -> Run only to this one -> gfNr <= gfNrServer
    unsigned gfNrServer;
    /// GF at wich we should pause the game
    unsigned pause_gf;
    /// Force pause the game (start TS and length) e.g. to compensate for lags
    unsigned forcePauseStart, forcePauseLen;
};

#endif // FramesInfo_h__

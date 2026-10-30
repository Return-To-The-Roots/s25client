// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

class HeadlessReplay;
struct ReplayDesync;

/// Replay metadata and the player roster, printed before playback starts
void printInitialInfo(const HeadlessReplay& replay);

/// With verbose, also dumps the async log of the random number generator.
void printDesync(const ReplayDesync& desync, bool verbose);

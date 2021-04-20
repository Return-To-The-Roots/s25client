// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#ifdef _WIN32
#    include <windows.h>
BOOL WINAPI ConsoleSignalHandler(DWORD dwCtrlType);
#else
/// Kill instantly on CTRL-C or just terminate
extern bool killme;
void ConsoleSignalHandler(int sig);
#endif // _WIN32

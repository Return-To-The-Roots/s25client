// Copyright (c) 2021 - 2021 Settlers Freaks (sf-team at siedler25.org)
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

#pragma once

// The purpose of this whole testsuite is to check sending and receiving Notes
// across different translation units where each only sees 1 Note.
// This allows to detect issues which can appear in practice.
// Hence these functions must not use the TestNote<N> classes in the interface

#include <cstdint>
class NotificationManager;

void subscribeTestNote1(NotificationManager& mgr);
int getLastTestNote1();
void sendTestNote1(NotificationManager& mgr, int value);
uint32_t getTestNote1IdSend();
uint32_t getTestNote1IdRecv();

void subscribeTestNote2(NotificationManager& mgr);
int getLastTestNote2();
void sendTestNote2(NotificationManager& mgr, int value);
uint32_t getTestNote2IdSend();
uint32_t getTestNote2IdRecv();

// Copyright (c) 2021 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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

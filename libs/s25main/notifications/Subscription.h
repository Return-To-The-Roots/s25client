// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>

/// This defines a subscription to a notification.
/// When there are no references left, the subscription is canceled
/// Note: Treat this as a kind of opaque handle
using Subscription = std::shared_ptr<void>;

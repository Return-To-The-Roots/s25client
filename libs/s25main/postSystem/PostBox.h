// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <functional>
#include <memory>
#include <string>

class PostMsg;

class PostBox
{
public:
    using NewMsgCallback = std::function<void(const PostMsg&, unsigned)>;
    using MsgDeletedCallback = std::function<void(unsigned)>;

    PostBox();
    ~PostBox();

    /// Add the message, possibly deleting the oldest message
    void AddMsg(std::unique_ptr<const PostMsg> msg);
    /// Delete the message. Return true, when it existed
    bool DeleteMsg(const PostMsg* msg);
    /// Delete the message by its index. Return true, if it was valid
    /// Oldest message is at index 0
    bool DeleteMsg(unsigned idx);
    void Clear();
    unsigned GetNumMsgs() const { return numMessages; }
    static constexpr unsigned GetMaxMsgs() { return MAX_MESSAGES; }
    /// Get message by index or nullptr if invalid index
    /// Oldest message is at index 0
    const PostMsg* GetMsg(unsigned idx) const;
    /// Set callback that receives new message and message count everytime a message is added
    void ObserveNewMsg(const NewMsgCallback& callback) { evNewMsg = callback; }
    /// Set callback that receives new message count everytime a message is deleted
    void ObserveDeletedMsg(const MsgDeletedCallback& callback) { evDelMsg = callback; }

    /// Sets the current goal as shown in the post window
    void SetCurrentMissionGoal(const std::string& goal);
    std::string GetCurrentMissionGoal() const;

private:
    static constexpr unsigned MAX_MESSAGES = 20;
    bool DeleteMsg(unsigned idx, bool notify);
    std::array<std::unique_ptr<const PostMsg>, MAX_MESSAGES> messages;
    unsigned numMessages = 0;
    /// Current mission goal. Shown as a special message
    std::string currentMissionGoal;
    NewMsgCallback evNewMsg;
    MsgDeletedCallback evDelMsg;
};

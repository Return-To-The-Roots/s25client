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

#ifndef PostBox_h__
#define PostBox_h__

#include <boost/array.hpp>
#include <boost/function.hpp>

class PostMsg;

class PostBox
{
public:
    typedef boost::function<void(unsigned)> Callback;

    PostBox();
    ~PostBox();

    /// Add the message, possibly deleting the oldest message
    void AddMsg(PostMsg* msg);
    /// Delete the message. Return true, when it existed
    bool DeleteMsg(PostMsg* msg);
    /// Delete the message by its index. Return true, if it was valid
    /// Oldest message is at index 0
    bool DeleteMsg(unsigned idx);
    unsigned GetNumMsgs() const { return numMessages; }
    static BOOST_CONSTEXPR unsigned GetMaxMsgs() { return MAX_MESSAGES; }
    /// Get message by index or NULL if invalid index
    /// Oldest message is at index 0
    PostMsg* GetMsg(unsigned idx) const;
    /// Set callback that receives new message count everytime a message is added
    void ObserveNewMsg(const Callback& callback) { evNewMsg = callback; }
    /// Set callback that receives new message count everytime a message is deleted
    void ObserveDeletedMsg(const Callback& callback) { evDelMsg = callback; }
private:
    BOOST_STATIC_CONSTEXPR unsigned MAX_MESSAGES = 20;
    bool DeleteMsg(unsigned idx, bool notify);
    boost::array<PostMsg*, MAX_MESSAGES> messages;
    unsigned numMessages;
    Callback evNewMsg, evDelMsg;
};

#endif // PostBox_h__

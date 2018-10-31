// Copyright (c) 2018 - 2018 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef Handle_h__
#define Handle_h__

#include <boost/move/move.hpp>

/// Class similar to shared_pointer but with a deleter like unqiue_ptr.
/// Not thread safe!
/// Expects:
/// T_Deleter::pointer Type of handle to be used, trivial (copy, ...)
/// T_Deleter()(T_Deleter::pointer) Release the handle
template<typename T_Deleter>
class shared_handle
{
public:
    typedef typename T_Deleter::pointer Pointer;

private:
    unsigned* refCt_;
    Pointer handle_;
    BOOST_COPYABLE_AND_MOVABLE(shared_handle)
public:
    explicit shared_handle(Pointer handle) : refCt_(new unsigned(1)), handle_(handle) {}
    shared_handle(const shared_handle& other) : refCt_(other.refCt_), handle_(other.handle_) { ++*refCt_; }
    shared_handle(BOOST_RV_REF(shared_handle) other) : refCt_(other.refCt_), handle_(other.handle_) { other.refCt_ = NULL; }
    ~shared_handle()
    {
        RTTR_Assert(!refCt_ || *refCt_ > 0u);
        if(refCt_ && --*refCt_ == 0)
        {
            T_Deleter()(handle_);
            delete refCt_;
        }
    }
    shared_handle& operator=(shared_handle other) { swap(other); }
    void swap(shared_handle& other)
    {
        using std::swap;
        swap(handle_, other.handle_);
        swap(refCt_, other.refCt_);
    }
    friend void swap(shared_handle& a, shared_handle& b) { a.swap(b); }
    Pointer get() const { return handle_; }
};

#endif // Handle_h__

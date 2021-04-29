// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

/// Class similar to shared_pointer but with a deleter like unqiue_ptr.
/// Not thread safe!
/// Expects:
/// T_Deleter::pointer Type of handle to be used, trivial (copy, ...)
/// T_Deleter()(T_Deleter::pointer) Release the handle
template<typename T_Deleter>
class shared_handle
{
public:
    using Pointer = typename T_Deleter::pointer;

private:
    unsigned* refCt_;
    Pointer handle_;

public:
    explicit shared_handle(Pointer handle) : refCt_(new unsigned(1)), handle_(handle) {}
    shared_handle(const shared_handle& other) : refCt_(other.refCt_), handle_(other.handle_) { ++*refCt_; }
    shared_handle(shared_handle&& other) : refCt_(other.refCt_), handle_(other.handle_) { other.refCt_ = nullptr; }
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

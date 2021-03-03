// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "RTTR_Assert.h"
#include <boost/container/small_vector.hpp>
#include <limits>

/// Just for occasional temporary debugging, all should be covered by tests and this is SLOW
#define RTTR_SLOW_DEBUG_CHECKS 0

template<typename T, class T_GetKey>
class OpenListBinaryHeap;

/// Class used to store the position in the heap. Heap elements must inherit from this
struct BinaryHeapPosMarker
{
private:
    unsigned pos;

    template<typename T, class T_GetKey>
    friend class OpenListBinaryHeap;
};

template<typename T, class T_GetKey>
class OpenListBinaryHeap : T_GetKey
{
public:
    using size_type = decltype(BinaryHeapPosMarker::pos);
    using value_type = T;
    using key_type = unsigned;

private:
    struct Element
    {
        key_type key;
        value_type* el;
        constexpr Element() = default; //-V730
        constexpr Element(key_type key, value_type* el) : key(key), el(el) {}
    };

public:
    size_type size() const { return elements.size(); }
    bool empty() const { return elements.empty(); }
    void clear() { elements.clear(); }

    T* top() const;
    void push(T* newEl);
    T* pop();
    void decreasedKey(T* el);
    void rearrange(T* el) { decreasedKey(el); }

protected:
    static size_type NoPos() { return std::numeric_limits<size_type>::max(); }
    static size_type ParentPos(size_type pos) { return (pos - 1) / 2; }
    static size_type LeftChildPos(size_type pos) { return (2 * pos) + 1; }
    static size_type RightChildPos(size_type pos) { return (2 * pos) + 2; }

    bool isHeap() const;
    bool arePositionsValid() const;
    static size_type& GetPos(T* el) { return static_cast<BinaryHeapPosMarker*>(el)->pos; }
    key_type GetKey(T* el) const { return T_GetKey::operator()(*el); }
    key_type GetKey(size_type idx) const { return GetKey(this->elements[idx].el); }

    boost::container::small_vector<Element, 64> elements;
};

//////////////////////////////////////////////////////////////////////////
// Implementation
//////////////////////////////////////////////////////////////////////////

#if RTTR_SLOW_DEBUG_CHECKS
#    define RTTR_VALIDATE_HEAP() \
        RTTR_Assert(isHeap());   \
        RTTR_Assert(arePositionsValid())
#else
#    define RTTR_VALIDATE_HEAP() (void)0
#endif

template<typename T, class T_GetKey>
bool OpenListBinaryHeap<T, T_GetKey>::isHeap() const
{
    size_type size = this->size();
    for(size_type i = 0; i < size; i++)
    {
        const size_type left = LeftChildPos(i);
        const size_type right = RightChildPos(i);
        // If child exist, parent must be "less" than child
        if(left < size && GetKey(left) < GetKey(i))
            return false;
        if(right < size && GetKey(right) < GetKey(i))
            return false;
    }
    return true;
}

template<typename T, class T_GetKey>
bool OpenListBinaryHeap<T, T_GetKey>::arePositionsValid() const
{
    for(size_type i = 0; i < this->size(); i++)
    {
        if(i != GetPos(this->elements[i].el))
            return false;
    }
    return true;
}

template<typename T, class T_GetKey>
inline T* OpenListBinaryHeap<T, T_GetKey>::top() const
{
    RTTR_Assert(!this->empty());
    return this->elements.front().el;
}

template<typename T, class T_GetKey>
inline void OpenListBinaryHeap<T, T_GetKey>::push(T* newEl)
{
    RTTR_VALIDATE_HEAP();
    GetPos(newEl) = this->size();
    this->elements.push_back(Element(GetKey(newEl), newEl));
    decreasedKey(newEl);
}

template<typename T, class T_GetKey>
inline void OpenListBinaryHeap<T, T_GetKey>::decreasedKey(T* el)
{
    size_type i = GetPos(el);
    unsigned elVal = this->elements[i].key = GetKey(el);
    RTTR_Assert(i < this->size());
    while(i > 0)
    {
        const unsigned parentPos = ParentPos(i);
        if(elVal >= this->elements[parentPos].key)
            break;
        using std::swap;
        GetPos(this->elements[parentPos].el) = i;
        swap(this->elements[parentPos], this->elements[i]);
        i = parentPos;
    }
    GetPos(el) = i;
    RTTR_VALIDATE_HEAP();
}

template<typename T, class T_GetKey>
inline T* OpenListBinaryHeap<T, T_GetKey>::pop()
{
    RTTR_Assert(!this->empty());
    RTTR_VALIDATE_HEAP();

    // Return value is the current minimum element
    T* const result = top();
    const unsigned size = this->size() - 1;

    if(size == 0)
    {
        // If this is the last element, just remove it
        this->elements.pop_back();
        return result;
    }
    // Else move the last element to the front and let it sink

    // First store the last element in a temporary
    Element el = this->elements.back();
    this->elements.pop_back();

    // We do not move it till we know its final destination, but just assume it was at the front (i=0)
    size_type i = 0;
    do
    {
        // Now check if the heap condition is violated for the current position
        const size_type left = LeftChildPos(i);
        if(left >= size)
            break; // No child? -> All ok
        const size_type right = RightChildPos(i);
        const unsigned leftVal = this->elements[left].key;
        if(leftVal < el.key) // left < i
        {
            if(right >= size || leftVal < this->elements[right].key) // left < right
            {
                this->elements[i] = this->elements[left];
                GetPos(this->elements[i].el) = i;
                i = left;
                continue;
            }
        }
        // left >= i || (left < i && left >= right)
        if(right < size && this->elements[right].key < el.key) // right < i
        {
            this->elements[i] = this->elements[right];
            GetPos(this->elements[i].el) = i;
            i = right;
        } else
            break;
    } while(true);

    this->elements[i] = el;
    GetPos(el.el) = i;

    return result;
}

#undef RTTR_SLOW_DEBUG_CHECKS
#undef RTTR_VALIDATE_HEAP

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

#include <limits>
#include <vector>

template<typename T>
class OpenListBinaryHeapBase
{
public:
    using size_type = unsigned;
    using value_type = T;
    using key_type = unsigned;
    struct Element
    {
        key_type key;
        value_type* el;
        Element() = default; //-V730
        Element(key_type key, value_type* el) : key(key), el(el) {}
    };

    /// Class used to store the position in the heap
    struct PosMarker
    {
    private:
        size_type pos;

        friend class OpenListBinaryHeapBase;
    };

    OpenListBinaryHeapBase() { elements.reserve(128); }
    size_type size() const { return elements.size(); }
    bool empty() const { return elements.empty(); }

protected:
    std::vector<Element> elements;
    static size_type& GetPos(PosMarker& posMarker) { return posMarker.pos; }
};

template<class T_Heap>
struct DefaultGetPosMarker
{
    typename T_Heap::PosMarker& operator()(typename T_Heap::value_type* el) { return el->posMarker; }
};

template<typename T, class T_GetKey, class GetPosMarker = DefaultGetPosMarker<OpenListBinaryHeapBase<T>>>
class OpenListBinaryHeap : public OpenListBinaryHeapBase<T>
{
    using Parent = OpenListBinaryHeapBase<T>;
    using Element = typename Parent::Element;

public:
    using size_type = typename Parent::size_type;
    using key_type = typename Parent::key_type;

    T* top() const;
    void push(T* newEl);
    T* pop();
    void decreasedKey(T* el);
    void rearrange(T* el) { decreasedKey(el); }

private:
    static size_type NoPos() { return std::numeric_limits<size_type>::max(); }
    static size_type ParentPos(size_type pos) { return (pos - 1) / 2; }
    static size_type LeftChildPos(size_type pos) { return (2 * pos) + 1; }
    static size_type RightChildPos(size_type pos) { return (2 * pos) + 2; }

    bool isHeap(size_type pos = 0) const;
    bool arePositionsValid() const;
    static size_type& GetPos(T* el) { return Parent::GetPos(GetPosMarker()(el)); }
    static key_type GetKey(T* el) { return T_GetKey()(*el); }
    key_type GetKey(size_type idx) const { return GetKey(this->elements[idx].el); }
};

//////////////////////////////////////////////////////////////////////////
// Implementation
//////////////////////////////////////////////////////////////////////////

template<typename T, class T_GetKey, class GetPosMarker>
bool OpenListBinaryHeap<T, T_GetKey, GetPosMarker>::isHeap(size_type pos) const
{
    size_type size = this->size();
    if(pos >= size)
        return true;
    if(pos == 0)
    {
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
    } else
    {
        const size_type left = LeftChildPos(pos);
        const size_type right = RightChildPos(pos);
        // If child exist, parent must be "less" than child
        if(left < size && GetKey(left) < GetKey(pos))
            return false;
        if(right < size && GetKey(right) < GetKey(pos))
            return false;
        return isHeap(left) && isHeap(right);
    }
}

template<typename T, class T_GetKey, class GetPosMarker>
bool OpenListBinaryHeap<T, T_GetKey, GetPosMarker>::arePositionsValid() const
{
    for(size_type i = 0; i < this->size(); i++)
    {
        if(i != GetPos(this->elements[i].el))
            return false;
    }
    return true;
}

template<typename T, class T_GetKey, class GetPosMarker>
inline T* OpenListBinaryHeap<T, T_GetKey, GetPosMarker>::top() const
{
    return this->elements.front().el;
}

template<typename T, class T_GetKey, class GetPosMarker>
inline void OpenListBinaryHeap<T, T_GetKey, GetPosMarker>::push(T* newEl)
{
    RTTR_Assert(isHeap());
    RTTR_Assert(arePositionsValid());
    GetPos(newEl) = this->size();
    this->elements.push_back(Element(GetKey(newEl), newEl));
    decreasedKey(newEl);
    RTTR_Assert(isHeap());
}

template<typename T, class T_GetKey, class GetPosMarker>
inline void OpenListBinaryHeap<T, T_GetKey, GetPosMarker>::decreasedKey(T* el)
{
    RTTR_Assert(arePositionsValid());
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
    RTTR_Assert(isHeap());
    RTTR_Assert(arePositionsValid());
}

template<typename T, class T_GetKey, class GetPosMarker>
inline T* OpenListBinaryHeap<T, T_GetKey, GetPosMarker>::pop()
{
    RTTR_Assert(arePositionsValid());
    RTTR_Assert(isHeap());
    RTTR_Assert(!this->empty());

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
        RTTR_Assert(isHeap(left));
        if(left >= size)
            break; // No child? -> All ok
        const size_type right = RightChildPos(i);
        RTTR_Assert(isHeap(right));
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

    RTTR_Assert(isHeap());
    RTTR_Assert(arePositionsValid());
    return result;
}

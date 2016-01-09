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

#ifndef OpenListBinaryHeap_h__
#define OpenListBinaryHeap_h__

#include <vector>
#include <limits>

template<typename T>
class OpenListBinaryHeapBase
{
public:
    typedef unsigned size_type;
    typedef T value_type;

    /// Class used to store the position in the heap
    struct PosMarker{
    private:
        size_type pos;

        friend class OpenListBinaryHeapBase;
    };

    OpenListBinaryHeapBase(){ elements.reserve(128); }
    size_type size() const{ return elements.size(); }
    bool empty() const{ return elements.empty(); }

protected:
    std::vector<value_type*> elements;
    static size_type& GetPos(PosMarker& posMarker){ return posMarker.pos; }
}; 

template<class T_Heap>
struct DefaultGetPosMarker
{
    typename T_Heap::PosMarker& operator()(typename T_Heap::value_type* el)
    {
        return el->posMarker;
    }
};

template<typename T, class Pr, class GetPosMarker = DefaultGetPosMarker<OpenListBinaryHeapBase<T> > >
class OpenListBinaryHeap: public OpenListBinaryHeapBase<T>
{
    typedef OpenListBinaryHeapBase<T> Parent;
public:
    T* top() const;
    void push(T* newEl);
    T* pop();
    void decreasedKey(T* el);
    void rearrange(T* el){ decreasedKey(el); }

private:

    static size_type NoPos(){ return std::numeric_limits<size_type>::max(); }
    static size_type ParentPos(size_type pos){ return (pos - 1) / 2; }
    static size_type LeftChildPos(size_type pos){ return (2 * pos) + 1; }
    static size_type RightChildPos(size_type pos){ return (2 * pos) + 2; }

    bool isHeap(size_type pos = 0) const;
    bool arePositionsValid() const;
    static size_type& GetPos(T* el){ return Parent::GetPos(GetPosMarker()(el)); }
};

//////////////////////////////////////////////////////////////////////////
// Implementation
//////////////////////////////////////////////////////////////////////////

template<typename T, class Pr, class GetPosMarker>
bool OpenListBinaryHeap<T, Pr, GetPosMarker>::isHeap(size_type pos) const
{
    if(pos >= size())
        return true;
    if(pos == 0)
    {
        for(size_type i=0; i<size(); i++)
        {
            const size_type left = LeftChildPos(i);
            const size_type right = RightChildPos(i);
            // If child exist, parent must be "less" than child
            if(left < size() && Pr()(*elements[left], *elements[i]))
                return false;
            if(right < size() && Pr()(*elements[right], *elements[i]))
                return false;
        }
        return true;
    }else
    {
        const size_type left = LeftChildPos(pos);
        const size_type right = RightChildPos(pos);
        // If child exist, parent must be "less" than child
        if(left < size() && Pr()(*elements[left], *elements[pos]))
            return false;
        if(right < size() && Pr()(*elements[right], *elements[pos]))
            return false;
        return isHeap(left) && isHeap(right);
    }
}

template<typename T, class Pr, class GetPosMarker>
bool OpenListBinaryHeap<T, Pr, GetPosMarker>::arePositionsValid() const
{
    for(size_type i=0; i<size(); i++)
    {
        if(i != GetPos(elements[i]))
            return false;
    }
    return true;
}

template<typename T, class Pr, class GetPosMarker>
inline T* OpenListBinaryHeap<T, Pr, GetPosMarker>::top() const
{
    return elements[0];
}

template<typename T, class Pr, class GetPosMarker>
inline void OpenListBinaryHeap<T, Pr, GetPosMarker>::push(T* newEl)
{
    assert(isHeap());
    assert(arePositionsValid());
    GetPos(newEl) = size();
    elements.push_back(newEl);
    decreasedKey(newEl);
    assert(isHeap());
}

template<typename T, class Pr, class GetPosMarker>
inline void OpenListBinaryHeap<T, Pr, GetPosMarker>::decreasedKey(T* el)
{
    assert(arePositionsValid());
    size_type& i = GetPos(el);
    assert(i < size());
    while(i > 0)
    {
        const unsigned parentPos = ParentPos(i);
        if(!Pr()(*el, *elements[parentPos]))
            break;
        using std::swap;
        GetPos(elements[parentPos]) = i;
        swap(elements[parentPos], elements[i]);
        i = parentPos;
    }
    assert(isHeap());
    assert(arePositionsValid());
}

template<typename T, class Pr, class GetPosMarker>
inline T* OpenListBinaryHeap<T, Pr, GetPosMarker>::pop()
{
    assert(arePositionsValid());
    assert(isHeap());
    assert(!empty());

    T* result = top();

    if(size() == 1)
    {
        // If this is the last element, just remove it
        elements.pop_back();
        return result;
    }

    elements[0] = elements[size() - 1];
    elements.pop_back();

    using std::swap;
    GetPos(elements[0]) = 0;
    size_type i = 0;
    do{
        const size_type left = LeftChildPos(i);
        assert(isHeap(left));
        if(left >= size())
            break;
        const size_type right = RightChildPos(i);
        assert(isHeap(right));
        if(Pr()(*elements[left], *elements[i])) // left < i
        {
            if(right >= size() || Pr()(*elements[left], *elements[right])) // left < right
            {
                swap(elements[i], elements[left]);
                GetPos(elements[i]) = i;
                GetPos(elements[left]) = left;
                i = left;
                continue;
            }

        }
        // left >= i || (left < i && left >= right)
        if(right < size() && Pr()(*elements[right], *elements[i])) // right < i
        {
            swap(elements[i], elements[right]);
            GetPos(elements[i]) = i;
            GetPos(elements[right]) = right;
            i = right;
        }else
            break;
    }while(true);

    assert(isHeap());
    assert(arePositionsValid());
    return result;
}

#endif // OpenListBinaryHeap_h__
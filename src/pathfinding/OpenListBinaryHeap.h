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
    struct Element {
        unsigned key;
        T* el;
        Element() {}
        Element(unsigned key, T* el) :key(key), el(el) {}
    };


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
    std::vector<Element> elements;
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
            if(left < size() && Pr()(*elements[left].el, *elements[i].el))
                return false;
            if(right < size() && Pr()(*elements[right].el, *elements[i].el))
                return false;
        }
        return true;
    }else
    {
        const size_type left = LeftChildPos(pos);
        const size_type right = RightChildPos(pos);
        // If child exist, parent must be "less" than child
        if(left < size() && Pr()(*elements[left].el, *elements[pos].el))
            return false;
        if(right < size() && Pr()(*elements[right].el, *elements[pos].el))
            return false;
        return isHeap(left) && isHeap(right);
    }
}

template<typename T, class Pr, class GetPosMarker>
bool OpenListBinaryHeap<T, Pr, GetPosMarker>::arePositionsValid() const
{
    for(size_type i=0; i<size(); i++)
    {
        if(i != GetPos(elements[i].el))
            return false;
    }
    return true;
}

template<typename T, class Pr, class GetPosMarker>
inline T* OpenListBinaryHeap<T, Pr, GetPosMarker>::top() const
{
    return elements[0].el;
}

template<typename T, class Pr, class GetPosMarker>
inline void OpenListBinaryHeap<T, Pr, GetPosMarker>::push(T* newEl)
{
    assert(isHeap());
    assert(arePositionsValid());
    GetPos(newEl) = size();
    elements.push_back(Element(newEl->estimatedDistance, newEl));
    decreasedKey(newEl);
    assert(isHeap());
}

template<typename T, class Pr, class GetPosMarker>
inline void OpenListBinaryHeap<T, Pr, GetPosMarker>::decreasedKey(T* el)
{
    assert(arePositionsValid());
    size_type& i = GetPos(el);
    elements[i].key = el->estimatedDistance;
    unsigned elVal = el->estimatedDistance;
    assert(i < size());
    while(i > 0)
    {
        const unsigned parentPos = ParentPos(i);
        if(elVal >= elements[parentPos].key)
            break;
        using std::swap;
        GetPos(elements[parentPos].el) = i;
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
    const unsigned size = this->size() - 1;

    if(size == 0)
    {
        // If this is the last element, just remove it
        elements.pop_back();
        return result;
    }

    Element el = elements.back();
    elements.pop_back();

    size_type i = 0;
    do{
        const size_type left = LeftChildPos(i);
        assert(isHeap(left));
        if(left >= size)
            break;
        const size_type right = RightChildPos(i);
        assert(isHeap(right));
        unsigned leftVal = elements[left].key;
        if(leftVal < el.key) // left < i
        {
            if(right >= size || leftVal<elements[right].key) // left < right
            {
                elements[i] = elements[left];
                GetPos(elements[i].el) = i;
                i = left;
                continue;
            }

        }
        // left >= i || (left < i && left >= right)
        if(right < size && elements[right].key < el.key) // right < i
        {
            elements[i] = elements[right];
            GetPos(elements[i].el) = i;
            i = right;
        }else
            break;
    }while(true);

    elements[i] = el;
    GetPos(el.el) = i;

    assert(isHeap());
    assert(arePositionsValid());
    return result;
}

#endif // OpenListBinaryHeap_h__
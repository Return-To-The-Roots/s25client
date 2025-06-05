//
// Created by pavel on 04.06.25.
//

#ifndef RATEDPOINT_H
#define RATEDPOINT_H
#include "ai/aijh/AIMap.h"
#include <optional>
#include <set>

struct RatedPoint
{
    MapPoint pt;
    int rating;
};

struct CompareByRating
{
    bool operator()(const RatedPoint& a, const RatedPoint& b) const { return a.rating > b.rating; }
};
class RatedPointSet
{
public:
    using SetType = std::multiset<RatedPoint, CompareByRating>;

    RatedPointSet(size_t maxSize) : maxSize_(maxSize) {}

    std::optional<RatedPoint> insert(const RatedPoint& value)
    {
        elements_.insert(value);
        return evictIfNeeded();
    }

    std::optional<RatedPoint> insert(RatedPoint&& value)
    {
        elements_.insert(std::move(value));
        return evictIfNeeded();
    }
    auto begin() const { return elements_.begin(); }
    auto end() const { return elements_.end(); }
    const SetType& data() const { return elements_; }

private:
    size_t maxSize_;
    SetType elements_;

    std::optional<RatedPoint> evictIfNeeded()
    {
        if(elements_.size() > maxSize_)
        {
            auto it = std::prev(elements_.end());  // least important due to reversed sorting
            RatedPoint evicted = std::move(*it);
            elements_.erase(it);
            return evicted;
        }
        return std::nullopt;
    }
};
#endif //RATEDPOINT_H

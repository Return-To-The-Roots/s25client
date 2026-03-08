// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <iosfwd>
#include <type_traits>

namespace helpers {

/// Template for declaring strong types used for IDs
/// Very thin wrapper mainly to distinguish different IDs.
/// The "invalid" value is just a marker and could be used as any other value.
/// StrongId can be used as indices into 1-based containers, i.e. the entry at index 0 is the invalid entry.
/// See `StrongIdVector` for avoiding that entry.
/// Usage: using MyId = StrongId<unsigned, struct MyIdTag>
template<class Underlying, class Tag>
class StrongId
{
    static_assert(std::is_integral_v<Underlying> && std::is_unsigned_v<Underlying>);
    static constexpr Underlying invalidValue_ = 0;

public:
    using underlying_t = Underlying;

    /// Factory function to get the invalid ID
    static constexpr StrongId invalidValue() { return StrongId(invalidValue_); }

    /// Construct an invalid ID
    constexpr StrongId() : value_(invalidValue_){};
    constexpr explicit StrongId(underlying_t value) : value_(value) {}

    constexpr explicit operator bool() const noexcept { return isValid(); }
    constexpr bool isValid() const noexcept { return value_ != invalidValue_; }

    constexpr underlying_t value() const { return value_; }

    /// Return the ID following this ID
    constexpr StrongId next() noexcept { return StrongId(value_ + 1); }
    /// Reset to invalid ID (default value)
    constexpr void reset() noexcept { value_ = invalidValue_; }

    explicit constexpr operator Underlying() const { return value_; }

    friend constexpr bool operator==(const StrongId r, const StrongId l) { return r.value_ == l.value_; };
    friend constexpr bool operator!=(const StrongId r, const StrongId l) { return r.value_ != l.value_; };

    friend std::ostream& operator<<(std::ostream& os, const StrongId t) { return os << t.value_; }

private:
    underlying_t value_;
};
} // namespace helpers

#ifndef enum_cast_h__
#define enum_cast_h__

#include "helpers/EnumTraits.h"
#include <type_traits>

namespace rttr {
template<typename T_Enum>
constexpr auto enum_cast(T_Enum val)
{
    using RealEnum = helpers::wrapped_enum_t<T_Enum>; // Support for "fake" strong enums. TODO: Remove
    return static_cast<std::underlying_type_t<RealEnum>>(val);
}
} // namespace rttr

#endif // enum_cast_h__

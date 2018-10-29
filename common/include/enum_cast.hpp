#ifndef enum_cast_h__
#define enum_cast_h__

#include <boost/core/scoped_enum.hpp>
#ifdef BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/core/underlying_type.hpp>
#else
#include <type_traits>
#endif

namespace rttr {
template<typename EnumType>
struct underlying_type
{
#ifdef BOOST_NO_CXX11_SCOPED_ENUMS
    typedef typename EnumType::underlying_type type;
#else
    typedef typename std::underlying_type<EnumType>::type type;
#endif
};
template<typename T_Enum>
typename underlying_type<T_Enum>::type enum_cast(T_Enum val)
{
    return boost::underlying_cast<typename underlying_type<T_Enum>::type>(val);
}
} // namespace rttr

#endif // enum_cast_h__

#ifndef BOOST_CORE_SCOPED_ENUM_HPP
#define BOOST_CORE_SCOPED_ENUM_HPP

#include <boost/detail/scoped_enum_emulation.hpp>

#ifdef BOOST_NO_CXX11_SCOPED_ENUMS
    // Bugfix: Boost 1.55 expects native_value_ member
#   undef BOOST_SCOPED_ENUM_DECLARE_END
#   define BOOST_SCOPED_ENUM_DECLARE_END(EnumType)                                     \
        ;                                                                              \
        EnumType(enum_type v) BOOST_NOEXCEPT : v_(v) {}                                \
	    enum_type native_value_() const BOOST_NOEXCEPT { return get_native_value_(); } \
	    BOOST_SCOPED_ENUM_DECLARE_END2()
#endif

#endif // BOOST_CORE_SCOPED_ENUM_HPP


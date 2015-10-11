#pragma once

#include <boost/array.hpp>
#include <string>

/// Völker (dont forget to change shield-count in iwWares too ...)
enum Nation
{
	NAT_AFRICANS = 0,
	NAT_JAPANESES,
	NAT_ROMANS,
	NAT_VIKINGS,
	NAT_BABYLONIANS,
	NAT_COUNT,
	NAT_INVALID = 0xFFFFFFFF
};

const std::string NationNames[NAT_COUNT] = { "Africans", "Japaneses", "Romans", "Vikings", "Babylonians" };

#define NATIVE_NAT_COUNT 4

/// Konvertierungstabelle von RttR-Nation-Indizes in Original-S2-Nation-Indizes
const boost::array<unsigned char, NAT_COUNT> NATION_RTTR_TO_S2 =
{{
    3,
    2,
    0,
    1,
    0 /* Babylonians get the roman figures where no others are used */
}};
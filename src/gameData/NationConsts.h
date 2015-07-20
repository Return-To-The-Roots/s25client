#pragma once

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

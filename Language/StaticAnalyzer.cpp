#include "StaticAnalyzer.hpp"
#include "ErrorHandler.hpp"
#include <iostream>
#include <sstream>

std::unordered_map<std::string, int32_t> StaticAnalyzer::all_static_i32;

void StaticAnalyzer::new_static_i32(std::string name, int32_t value)
{
	all_static_i32[name] = value;
	std::cout << "StaticAnalyzer: " << name << " added!\n";
}

void StaticAnalyzer::check_static_i32(int32_t a, int32_t b)
{
	if ( b > 0 && a > std::numeric_limits<int32_t>::max() - b )
	{
		std::cout << "Overflow!\n";
		exit(1);
	}
	else if ( b < 0 && a < std::numeric_limits<int32_t>::min() - b )
	{
		std::cout << "Underflow!\n";
		exit(1);
	}
}

void StaticAnalyzer::add_static_i32(std::string name, int32_t value)
{
	if (all_static_i32.find(name) == all_static_i32.end())
		return;

	int32_t last_value = all_static_i32[name];

	last_value += value;

	check_static_i32(all_static_i32[name], value);

	all_static_i32[name] = last_value;
}

void StaticAnalyzer::sub_static_i32(std::string name, int32_t value)
{
	if (all_static_i32.find(name) == all_static_i32.end())
		return;

	int32_t last_value = all_static_i32[name];

	last_value -= value;

	check_static_i32(all_static_i32[name], -value);

	all_static_i32[name] = last_value;
}
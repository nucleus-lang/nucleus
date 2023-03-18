#ifndef STATIC_ANALYZER_HPP
#define STATIC_ANALYZER_HPP

#include <map>
#include <unordered_map>
#include <limits>
#include <string>
#include <cstdint>

struct StaticAnalyzer
{
	static std::unordered_map<std::string, int32_t> all_static_i32;

	static void new_static_i32(std::string name, int32_t value);
	static void check_static_i32(int32_t a, int32_t b);
	
	static void add_static_i32(std::string name, int32_t value);
	static void sub_static_i32(std::string name, int32_t value);
};

#endif
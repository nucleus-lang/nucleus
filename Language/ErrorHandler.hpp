#ifndef ERROR_HANDLER_HPP
#define ERROR_HANDLER_HPP

#include <string>

struct ErrorHandler
{
	static void print(std::string error, int line, int column, std::string line_of_code, int ty = 0, std::string help = "");
	static void print_line(std::string error, int line, std::string line_of_code, int ty = 0, std::string help = "");
};

#endif
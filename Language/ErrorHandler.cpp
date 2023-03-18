#include "ErrorHandler.hpp"
#include <iostream>

void ErrorHandler::print(std::string error, int line, int column, std::string line_of_code, int ty, std::string help)
{
	std::string error_type;

	if (ty == 0) error_type = "Error";
	else if (ty == 1) error_type = "Warning";

	std::cout << error_type << " in: " << "main.nk" << " at -> " << line << ":" << column << " >>> \n\n";

	std::string line_number_as_string = std::to_string(line) + " ";

	std::string amount_of_spaces = "";

	for (auto i : line_number_as_string)
	{
		amount_of_spaces += ' ';
	}

	std::string locate_error = "";

	for (int i = 0; i < column; i++)
	{
		if (line_of_code[i] == '\t') locate_error += "  ";
		else locate_error += ' ';
	}

	std::string add_end = "";

	if (line_of_code.back() != '\n') add_end = "\n";

	std::cout << amount_of_spaces << "|\n";
	std::cout << line_number_as_string << "|" << line_of_code << add_end;
	std::cout << amount_of_spaces << "|" << locate_error << "^\n";
	std::cout << amount_of_spaces << "| " << error << "\n";

	if (help == "")
	{
		std::cout << "\n";
		return;
	}

	std::cout << "Suggestion: " << help << "\n\n";
}

void ErrorHandler::print_line(std::string error, int line, std::string line_of_code, int ty, std::string help)
{
	std::string error_type;

	if (ty == 0) error_type = "Error";
	else if (ty == 1) error_type = "Warning";

	std::cout << error_type << " in: " << "main.nk" << " at -> " << line << " >>> \n\n";

	// ===============

	std::string line_number_as_string = std::to_string(line) + " ";

	std::string amount_of_spaces = "";

	for (auto i : line_number_as_string) amount_of_spaces += ' ';

	std::string add_end = "";

	if (line_of_code.back() != '\n') add_end = "\n";

	std::cout << amount_of_spaces << "|\n";
	std::cout << line_number_as_string << "|" << line_of_code << add_end;
	std::cout << amount_of_spaces << "|\n";
	std::cout << amount_of_spaces << "| " << error << "\n";

	if (help == "")
	{
		std::cout << "\n";
		return;
	}

	std::cout << "Suggestion: " << help << "\n\n";
}
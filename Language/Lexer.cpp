#include "Lexer.hpp"

std::string Lexer::Content;
std::string Lexer::NumValString;
std::string Lexer::IdentifierStr;

int Lexer::Line;
int Lexer::Column;
int Lexer::LastChar;

std::string Lexer::line_as_string;

std::vector<std::string> Lexer::all_lines_vector;

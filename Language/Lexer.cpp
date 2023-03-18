#include "Lexer.hpp"

std::string Lexer::Content;

std::string Lexer::IdentifierStr;
std::string Lexer::NumValString;
int Lexer::CurrentToken;
int Lexer::Position;

int Lexer::Column;
int Lexer::Line;

std::string Lexer::line_as_string;

std::vector<std::string> Lexer::all_lines_vector;

int Lexer::LastChar;
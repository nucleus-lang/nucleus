#include "Lexer.hpp"

std::string Lexer::Content;

std::string Lexer::IdentifierStr;
std::string Lexer::NumValString;
int Lexer::CurrentToken;
int Lexer::Position;

int Lexer::LastChar;
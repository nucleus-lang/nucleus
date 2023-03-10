#ifndef LEXER_HPP
#define LEXER_HPP

#include <string>
#include <iostream>

enum Token
{
	EndOfFile = -1,

	Function = -2,
	Extern = -3,

	Identifier = -4,
	Number = -5,

	Return = -6,
	Alloca = -7,
	Store = -8,
	Load = -9,

	Add = -10,
	Sub = -11,
	Mul = -12,
	Div = -13,

	Link = -14,
	Verify = -15,
};

struct Lexer
{
	static std::string Content;

	static std::string IdentifierStr;
	static std::string NumValString;

	static void AddContent(std::string c)
	{
		Content += c;
	}

	static int CurrentToken;
	static int Position;

	static void Start()
	{
		Position = -1;
		LastChar = ' ';
	}

	static int Advance()
	{
		Position += 1;
		return Content[Position];
	}

	static void GetNextToken()
	{
		CurrentToken = GetToken();
	}

	static int LastChar;

	static int GetToken()
	{
		while (isspace(LastChar))
		{
			LastChar = Advance();
		}

		if(isalpha(LastChar) || LastChar == '@')
			return GetIdentifier();

    	if (isdigit(LastChar)) 
        	return GetNumber();
    
      	if (LastChar == '#')
      	{
      		// Comment until end of line.
      		do
      		{
      			LastChar = Advance();
      		}
      		while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');
    	
      		if (LastChar != EOF)
      			return GetToken();
      	}

      	if (LastChar == EOF)
        	return Token::EndOfFile;

        int ThisChar = LastChar;
		LastChar = Advance();

		// This is a fail-safe in case memory corruption appears.
		// Since at this point, we're looking for normal characters,
		// it makes no sense to find characters that are below space in
		// the ASCII table. Meaning that if we find one like that at this point,
		// its undefined behavior.
		if(ThisChar < 32)
		{
		 	ThisChar = Token::EndOfFile;
		}

		return ThisChar;
	}

	static bool IsIdentifier(std::string s)
	{
		return IdentifierStr == s;
	}

	static bool is_still_identifier(char c) {

		return isalnum(c) || c == '_';
	}

	static void throw_identifier_syntax_warning(std::string message, std::string recommendation)
	{
		std::cout << "\n========================\n";
		std::cout << "Syntax Warning: " << message << "\n";
		std::cout << "Recommendation: " << recommendation << "\n";
		std::cout << "\n";
	}

	static void check_if_identifier_follows_format(unsigned int casing = 0, unsigned int type = 0)
	{
		std::string final_ident_str;

		bool print_message = false;

		for(auto i : IdentifierStr)
		{
			if(i < 32)
				break;

			if(isupper(i) && casing == 0)
			{
				print_message = true;
				final_ident_str += "_";
				final_ident_str += char(i + 32);
			}
			else
			{
				final_ident_str += i;
			}
		}

		if(print_message)
		{
			std::string casing_issue;

			if(casing == 0)
				casing_issue = "snake_casing";

			throw_identifier_syntax_warning(
				std::string(std::string("\"") + IdentifierStr + "\" doesn't follow " + casing_issue + "."),
				std::string("Try replacing \"" + IdentifierStr + "\" with \"" + final_ident_str + "\".")
				);
		}
	}

	static int GetIdentifier()
	{
		IdentifierStr = LastChar;

		while (is_still_identifier((LastChar = Advance())))
		{
        	IdentifierStr += LastChar;
		}

        if(IsIdentifier("fn"))
        	return Token::Function;

        if(IsIdentifier("return"))
        	return Token::Return;

        if(IsIdentifier("alloc"))
        	return Token::Alloca;

        if(IsIdentifier("store"))
        	return Token::Store;

        if(IsIdentifier("load"))
        	return Token::Load;

        if(IsIdentifier("add"))
        	return Token::Add;

        if(IsIdentifier("sub"))
        	return Token::Sub;

        if(IsIdentifier("link"))
        	return Token::Link;

        if(IsIdentifier("verify"))
        	return Token::Verify;

        return Token::Identifier;
	}

	static int GetNumber()
	{
		std::string NumStr;

        do
        {
        	NumStr += LastChar;
	       	LastChar = Advance();
        } while (isdigit(LastChar) || LastChar == '.' || LastChar == 'f');
    
        NumValString = NumStr;
        return Token::Number;
	}
};

#endif
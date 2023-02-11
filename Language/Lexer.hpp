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
		while (LastChar == ' ' || LastChar == '\n' || LastChar == '\r' || LastChar == '\t') 
			LastChar = Advance();

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

		if(ThisChar < 0)
		{
		 	ThisChar = 0;
		}

		return ThisChar;
	}

	static bool IsIdentifier(std::string s)
	{
		return IdentifierStr == s;
	}

	static int GetIdentifier()
	{
		IdentifierStr = LastChar;

		while (isalnum((LastChar = Advance())))
		{
        	IdentifierStr += LastChar;
		}

        if(IsIdentifier("fn"))
        	return Token::Function;

        if(IsIdentifier("@return"))
        	return Token::Return;

        if(IsIdentifier("@alloca"))
        	return Token::Alloca;

        if(IsIdentifier("@store"))
        	return Token::Store;

        if(IsIdentifier("@load"))
        	return Token::Load;

        if(IsIdentifier("@add"))
        	return Token::Add;

        if(IsIdentifier("@sub"))
        	return Token::Sub;

        if(IsIdentifier("@link"))
        	return Token::Link;

        if(IsIdentifier("@verify"))
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
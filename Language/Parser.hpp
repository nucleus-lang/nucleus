#ifndef PARSER_HPP
#define PARSER_HPP

#include "Lexer.hpp"
#include "AST.hpp"

struct Parser
{
	static std::unique_ptr<AST::Expression> ParseNumber() 
	{
		auto Result = std::make_unique<AST::Number>(Lexer::NumValString);
	
		Lexer::GetNextToken();
	
		return std::move(Result);
	}

	static std::unique_ptr<AST::Expression> ParseExpression() 
	{
  		auto LHS = ParsePrimary();
  		if (!LHS)
  		  return nullptr;

  		//Lexer::GetNextToken();

  		return ParseBinaryOperator(std::move(LHS));
  	}

  	static std::unique_ptr<AST::Expression> ParseBinaryOperator(std::unique_ptr<AST::Expression> L)
  	{
  		if(Lexer::CurrentToken == '=')
  		{
  			Lexer::GetNextToken();
  			auto R = ParseExpression();

  			return ParseBinaryOperator(std::make_unique<AST::Store>(std::move(L), std::move(R)));
  		}
  		else if(Lexer::CurrentToken == '+')
  		{
  			Lexer::GetNextToken();
  			auto R = ParseExpression();

  			return ParseBinaryOperator(std::make_unique<AST::Add>(std::move(L), std::move(R)));
  		}
  		else if(Lexer::CurrentToken == '-')
  		{
  			Lexer::GetNextToken();
  			auto R = ParseExpression();

  			return ParseBinaryOperator(std::make_unique<AST::Sub>(std::move(L), std::move(R)));
  		}
  		else
  			return L;
  	}

	static std::unique_ptr<AST::Expression> ParseParenthesis() 
	{
  		Lexer::GetNextToken(); // eat (.
  		auto V = ParseExpression();
  		if (!V)
  			return nullptr;
		
  		if (Lexer::CurrentToken != ')')
  			return AST::ExprError("Expected ')'");
	
  		Lexer::GetNextToken(); // eat ).

  		return V;
	}

	static std::unique_ptr<AST::Expression> ParseIdentifier() 
	{
  		std::string IdName = Lexer::IdentifierStr;
		
  		Lexer::GetNextToken();  // eat identifier.

  		//std::cout << "Parsed Identifier: " << IdName << "\n";
		
  		return std::make_unique<AST::Variable>(nullptr, IdName);
  	}

  	static std::unique_ptr<AST::Expression> ParsePrimary() 
  	{
		if(Lexer::CurrentToken == Token::Identifier) 
			return ParseIdentifier();

		else if(Lexer::CurrentToken == Token::Number) 
			return ParseNumber();

		else if(Lexer::CurrentToken == '(') 
			return ParseParenthesis();

		else if(Lexer::CurrentToken == Token::Return) 
			return ParseReturn();

		else if(Lexer::CurrentToken == Token::Alloca) 
			return ParseAlloca();

		else if(Lexer::CurrentToken == Token::Store) 
			return ParseStore();

		else if(Lexer::CurrentToken == Token::Load) 
			return ParseLoad();

		else if(Lexer::CurrentToken == Token::Add)
			return ParseAdd();

		else if(Lexer::CurrentToken == Token::Sub)
			return ParseSub();

		else if(Lexer::CurrentToken == Token::Link)
			return ParseLink();

		else 
			return AST::ExprError("unknown token when expecting an expression");
	}

	static std::unique_ptr<AST::Expression> ParseLink()
	{
		Lexer::GetNextToken();

		if(Lexer::CurrentToken != '(')
			return AST::ExprError("Expected '(' to initialize Add.");

		Lexer::GetNextToken();

		auto I = ParseIdentifier();

		if(Lexer::CurrentToken != ',')
		{
			if(Lexer::CurrentToken == ')')
			{
				Lexer::GetNextToken();

				return std::make_unique<AST::Link>(std::move(I), nullptr);
			}
			else
				return AST::ExprError("Expected ',' to separate Add arguments.");
		}

		Lexer::GetNextToken();

		auto E = ParseIdentifier();

		if(Lexer::CurrentToken != ')')
			return AST::ExprError("Expected ')' to close Add.");

		Lexer::GetNextToken();

		return std::make_unique<AST::Link>(std::move(I), std::move(E));
	}

	static std::unique_ptr<AST::Expression> ParseAdd()
	{
		Lexer::GetNextToken();

		if(Lexer::CurrentToken != '(')
			return AST::ExprError("Expected '(' to initialize Add.");

		Lexer::GetNextToken();

		auto I = ParseIdentifier();

		if(Lexer::CurrentToken != ',')
			return AST::ExprError("Expected ',' to separate Add arguments.");

		Lexer::GetNextToken();

		auto E = ParseExpression();

		if(Lexer::CurrentToken != ')')
			return AST::ExprError("Expected ')' to close Add.");

		Lexer::GetNextToken();

		return std::make_unique<AST::Add>(std::move(I), std::move(E));
	}

	static std::unique_ptr<AST::Expression> ParseSub()
	{
		Lexer::GetNextToken();

		if(Lexer::CurrentToken != '(')
			return AST::ExprError("Expected '(' to initialize Add.");

		Lexer::GetNextToken();

		auto I = ParseIdentifier();

		if(Lexer::CurrentToken != ',')
			return AST::ExprError("Expected ',' to separate Add arguments.");

		Lexer::GetNextToken();

		auto E = ParseExpression();

		if(Lexer::CurrentToken != ')')
			return AST::ExprError("Expected ')' to close Add.");

		Lexer::GetNextToken();

		return std::make_unique<AST::Sub>(std::move(I), std::move(E));
	}

	static std::unique_ptr<AST::Expression> ParseLoad()
	{
		Lexer::GetNextToken();

		std::string Name = Lexer::IdentifierStr;

		Lexer::GetNextToken();

		if(Lexer::CurrentToken != ':')
			return AST::ExprError("Expected ':' to set Load type.");

		Lexer::GetNextToken();

		auto T = ParseType();

		Lexer::GetNextToken();

		if(Lexer::CurrentToken != '=')
			return AST::ExprError("Expected ':' to set Load value.");

		Lexer::GetNextToken();

		auto Value = ParseIdentifier();

		return std::make_unique<AST::Load>(Name, std::move(T), std::move(Value));
	}

	static std::unique_ptr<AST::Expression> ParseStore()
	{
		Lexer::GetNextToken();

		if(Lexer::CurrentToken != '(')
			return AST::ExprError("Expected '('.");

		Lexer::GetNextToken();

		auto Target = ParseIdentifier();

		if(Lexer::CurrentToken != ',')
			return AST::ExprError("Expected ','.");

		Lexer::GetNextToken();

		auto Value = ParseExpression();

		if(Lexer::CurrentToken != ')')
			return AST::ExprError("Expected ')'.");

		Lexer::GetNextToken();

		return std::make_unique<AST::Store>(std::move(Target), std::move(Value));
	}

	static std::unique_ptr<AST::Expression> ParseAlloca()
	{
		Lexer::GetNextToken();

		std::string Name = Lexer::IdentifierStr;

		Lexer::GetNextToken();

		if(Lexer::CurrentToken != ':')
			return AST::ExprError("Expected ':' to set Alloca type.");

		Lexer::GetNextToken();

		auto T = ParseType();

		Lexer::GetNextToken();

		return std::make_unique<AST::Alloca>(std::move(T), Name);
	}

	static std::unique_ptr<AST::Expression> ParseReturn()
	{
		Lexer::GetNextToken();

		if(Lexer::CurrentToken == Token::Load)
		{
			Lexer::GetNextToken();

			auto Ref = ParseIdentifier();

			if(Lexer::CurrentToken != ';')
  				return AST::ExprError("Expected ';'");

			return std::make_unique<AST::Return>(std::move(Ref));
		}

		auto Expr = ParseExpression();

		if(Lexer::CurrentToken != ';')
  			return AST::ExprError("Expected ';'");

  		auto LoadExpr = std::make_unique<AST::Load>("autoLoad", nullptr, std::move(Expr));

		return std::make_unique<AST::Return>(std::move(LoadExpr));
	}

	static std::unique_ptr<AST::Type> ParseType()
	{
		if(Lexer::IdentifierStr == "i32") { return std::make_unique<AST::i32>(); }

		return nullptr;
	}

	static std::unique_ptr<AST::Prototype> ParsePrototype() 
	{
		auto PType = ParseType();

		if(!PType)
			return AST::Prototype::Error("Expected type in prototype");

		Lexer::GetNextToken();

		if (Lexer::CurrentToken != Token::Identifier)
	    	return AST::Prototype::Error("Expected function name in prototype");
	
		std::string FnName = Lexer::IdentifierStr;
		Lexer::GetNextToken();

		if (Lexer::CurrentToken != '(')
			return AST::Prototype::Error("Expected '(' in prototype");

		Lexer::GetNextToken();

	 // Read the list of argument names.
		std::vector<std::unique_ptr<AST::Variable>> ArgNames;
		while (Lexer::CurrentToken == Token::Identifier || Lexer::CurrentToken == ',')
		{
			auto t = ParseType();

			if(t == nullptr)
				return AST::Prototype::Error("Unknown type found in arguments.");

			Lexer::GetNextToken();

			auto A = std::make_unique<AST::Variable>(std::move(t), Lexer::IdentifierStr);

			ArgNames.push_back(std::move(A));
			Lexer::GetNextToken();

			if(Lexer::CurrentToken != ',')
				return AST::Prototype::Error("Expected ',' inside prototype");

			Lexer::GetNextToken();
		}

		if (Lexer::CurrentToken != ')')
			return AST::Prototype::Error("Expected ')' in prototype");

		// success.
		Lexer::GetNextToken();  // eat ')'.

		return std::make_unique<AST::Prototype>(std::move(PType), FnName, std::move(ArgNames));
	}

	static std::unique_ptr<AST::Function> ParseFunction() 
	{
  		Lexer::GetNextToken();  // eat def.
  		auto Proto = ParsePrototype();
  		if (!Proto) { return nullptr; }

  		if(Lexer::CurrentToken != '{')
  			AST::ExprError("Expected '{' in function");

  		Lexer::GetNextToken();

  		std::vector<std::unique_ptr<AST::Expression>> Body;
  		while(Lexer::CurrentToken != '}')
		{
  			if (auto E = ParseExpression())
				Body.push_back(std::move(E));

			if(Lexer::CurrentToken != ';')
  				AST::ExprError("Expected ';' inside function.");

  			Lexer::GetNextToken();
		}

  		return std::make_unique<AST::Function>(std::move(Proto), std::move(Body));
	}

	static void HandleFunction()
	{
		//std::cout << "Parsed a Function!\n";
		auto Func = ParseFunction();

		llvm::Function* getF = Func->codegen();

		getF->print(llvm::outs(), nullptr);
	}

	static void MainLoop()
	{
		while(Lexer::CurrentToken != Token::EndOfFile)
		{
			Lexer::GetNextToken();

			if(Lexer::CurrentToken == Token::EndOfFile)
				break;

			if(Lexer::CurrentToken == Token::Function)
			{
				HandleFunction();
				Lexer::GetNextToken();
			}
		}
	}
};

#endif
#ifndef PARSER_HPP
#define PARSER_HPP

#include "Lexer.hpp"
#include "AST.hpp"
#include "ErrorHandler.hpp"
#include "StaticAnalyzer.hpp"
#include <unordered_map>

struct Parser
{
	static std::string last_identifier;
	static std::string last_target;
	static bool grab_target;
	static std::unordered_map<std::string, std::string> all_variables;

	static void add_to_variables_list(std::string name, std::string type)
	{
		all_variables[name] = type;
	}

	static std::string get_type_from_variable(std::string name)
	{
		if (all_variables.find(name) == all_variables.end()) return "";

		return all_variables[name];
	}

	static std::unique_ptr<AST::Expression> ParseNumber() 
	{
		std::string type_result = get_type_from_variable(Parser::last_target);

		if (type_result == "i1" || type_result == "bool")
			return AST::ExprError("Cannot assign a number value to a boolean type variable.");

		auto Result = std::make_unique<AST::Number>(Lexer::NumValString);
	
		Lexer::GetNextToken();
	
		return std::move(Result);
	}

	static std::unique_ptr<AST::Expression> ParseExpression() 
	{
		auto LHS = ParsePrimary();
		if (!LHS) return nullptr;

		return ParseBinaryOperator(std::move(LHS));
	}

	static std::unique_ptr<AST::Expression> ParseBoolValue(bool is_true)
	{
		std::unique_ptr<AST::Number> Result = nullptr;

		std::string type_result = get_type_from_variable(Parser::last_target);

		if (type_result != "i1" || type_result != "bool")
			return AST::ExprError("Cannot assign a boolean value to a number type variable.");

		if (is_true) Result = std::make_unique<AST::Number>("1");
		else Result = std::make_unique<AST::Number>("0");

		Result->bit = 1;

		Lexer::GetNextToken();

		return Result;
	}

	static std::unique_ptr<AST::Expression> CheckForVerify(std::unique_ptr<AST::Expression> V)
	{
		if (dynamic_cast<AST::Link*>(V.get()))
		{
			std::cout << "Link Instruction Found!\n";
			AST::Link* L = (AST::Link*)V.get();

			if (L->Target == nullptr) return std::make_unique<AST::VerifyOne>(std::move(L->Value));
		}

		return V;
	}

	static std::unique_ptr<AST::Expression> ParseBinaryOperator(std::unique_ptr<AST::Expression> L)
	{
		if (Lexer::CurrentToken == '=')
		{
			Lexer::GetNextToken();
			auto R = ParseExpression();

			if (dynamic_cast<AST::Number*>(R.get()) == nullptr)
			{
				auto RLoad = std::make_unique<AST::Load>("autoLoad", std::make_unique<AST::i32>(), std::move(R));
				return ParseBinaryOperator(std::make_unique<AST::Store>(std::move(L), std::move(RLoad)));
			}
			else
			{
				// auto get_r = dynamic_cast<AST::Number*>(R.get());
				// int32_t r_i32;
				// if (get_r->isInt) r_i32 = get_r->return_i32();
				// StaticAnalyzer::new_static_i32(Parser::last_target, r_i32);

				return ParseBinaryOperator(std::make_unique<AST::Store>(std::move(L), std::move(R)));
			}
		}
		else if (Lexer::CurrentToken == '+')
		{
			Lexer::GetNextToken();

			if (Lexer::CurrentToken == '=') Lexer::GetNextToken();

			auto R = ParseExpression();

			// if (dynamic_cast<AST::Number*>(R.get()))
			// {
			// 	auto get_r = dynamic_cast<AST::Number*>(R.get());

			// 	int32_t r_i32;
			// 	if (get_r->isInt) r_i32 = get_r->return_i32();

			// 	StaticAnalyzer::add_static_i32(Parser::last_target, r_i32);
			// }
			
			return ParseBinaryOperator(std::make_unique<AST::Add>(std::move(L), std::move(R)));
		}
		else if (Lexer::CurrentToken == '-')
		{
			Lexer::GetNextToken();

			if (Lexer::CurrentToken == '=') Lexer::GetNextToken();

			auto R = ParseExpression();

			// if (dynamic_cast<AST::Number*>(R.get()))
			// {
			// 	auto get_r = dynamic_cast<AST::Number*>(R.get());

			// 	int32_t r_i32;
			// 	if (get_r->isInt) r_i32 = get_r->return_i32();

			// 	StaticAnalyzer::sub_static_i32(Parser::last_target, r_i32);
			// }

			return ParseBinaryOperator(std::make_unique<AST::Sub>(std::move(L), std::move(R)));
		}
		else if (Lexer::CurrentToken != ';')
		{
			return AST::ExprError("Expected an operator (=, +, -, * or /).");
		}
		else
		{
			return L;
		}
	}

	static std::unique_ptr<AST::Expression> ParseParenthesis() 
	{
		Lexer::GetNextToken(); // eat (.
		auto V = ParseExpression();
		if (!V) return nullptr;

		if (Lexer::CurrentToken != ')') return AST::ExprError("Expected ')'");

		Lexer::GetNextToken(); // eat ).

		return V;
	}

	static void SetIdentToMainTarget(std::string ident)
	{
		if (Parser::grab_target) Parser::last_target = std::string(ident);

		Parser::grab_target = false;
	}

	static std::unique_ptr<AST::Expression> ParseIdentifier() 
	{
		std::string IdName = Lexer::IdentifierStr;

		Parser::last_identifier = IdName;

		SetIdentToMainTarget(Parser::last_identifier);

		Lexer::check_if_identifier_follows_format(0, 0);

		Lexer::GetNextToken();  // eat identifier.

		return std::make_unique<AST::Variable>(nullptr, IdName);
	}

	static std::unique_ptr<AST::Expression> ParsePrimary() 
	{
		if (Lexer::CurrentToken == Token::Identifier) return ParseIdentifier();
		else if (Lexer::CurrentToken == Token::Number)  return ParseNumber();
		else if (Lexer::CurrentToken == '(') return ParseParenthesis();
		else if (Lexer::CurrentToken == Token::Return)  return ParseReturn();
		else if (Lexer::CurrentToken == Token::Alloca) return ParseAlloca();
		else if (Lexer::CurrentToken == Token::Store) return ParseStore();
		else if (Lexer::CurrentToken == Token::Load) return ParseLoad();
		else if (Lexer::CurrentToken == Token::Add) return ParseAdd();
		else if (Lexer::CurrentToken == Token::Sub) return ParseSub();
		else if (Lexer::CurrentToken == Token::Link) return ParseLink();
		else if (Lexer::CurrentToken == Token::Verify) return ParseVerify();
		else if (Lexer::CurrentToken == Token::True) return ParseBoolValue(true);
		else if (Lexer::CurrentToken == Token::False) return ParseBoolValue(false);
		else  return AST::ExprError("unknown token when expecting an expression");
	}

	static std::unique_ptr<AST::Expression> ParseVerify()
	{
		Lexer::GetNextToken();

		if (Lexer::CurrentToken == ';')
		{
			if (Parser::last_target != "")
			{
				auto I = std::make_unique<AST::Variable>(nullptr, Parser::last_target);
				return std::make_unique<AST::Link>(std::move(I), nullptr);
			}
			else
			{
				AST::ExprError("Last Target is not found!");
			}
		}

		auto I = ParseIdentifier();

		if (I == nullptr) return AST::ExprError("Identifier not found!");

		return std::make_unique<AST::Link>(std::move(I), nullptr);
	}

	static std::unique_ptr<AST::Expression> ParseLink()
	{
		Lexer::GetNextToken();

		if (Lexer::CurrentToken != '(') return AST::ExprError("Expected '(' to initialize Add.");

		Lexer::GetNextToken();

		auto I = ParseIdentifier();

		if (Lexer::CurrentToken != ',') return AST::ExprError("Expected ',' to separate Add arguments.");

		Lexer::GetNextToken();

		auto E = ParseIdentifier();

		if (Lexer::CurrentToken != ')') return AST::ExprError("Expected ')' to close Add.");

		Lexer::GetNextToken();

		return std::make_unique<AST::Link>(std::move(I), std::move(E));
	}

	static std::unique_ptr<AST::Expression> ParseAdd()
	{
		Lexer::GetNextToken();

		if (Lexer::CurrentToken != '(') return AST::ExprError("Expected '(' to initialize Add.");

		Lexer::GetNextToken();

		auto I = ParseIdentifier();

		if (Lexer::CurrentToken != ',') return AST::ExprError("Expected ',' to separate Add arguments.");

		Lexer::GetNextToken();

		auto E = ParseExpression();

		if (Lexer::CurrentToken != ')') return AST::ExprError("Expected ')' to close Add.");

		Lexer::GetNextToken();

		return std::make_unique<AST::Add>(std::move(I), std::move(E));
	}

	static std::unique_ptr<AST::Expression> ParseSub()
	{
		Lexer::GetNextToken();

		if (Lexer::CurrentToken != '(') return AST::ExprError("Expected '(' to initialize Add.");

		Lexer::GetNextToken();

		auto I = ParseIdentifier();

		if (Lexer::CurrentToken != ',') return AST::ExprError("Expected ',' to separate Add arguments.");

		Lexer::GetNextToken();

		auto E = ParseExpression();

		if (Lexer::CurrentToken != ')') return AST::ExprError("Expected ')' to close Add.");

		Lexer::GetNextToken();

		return std::make_unique<AST::Sub>(std::move(I), std::move(E));
	}

	static std::unique_ptr<AST::Expression> ParseLoad()
	{
		Lexer::GetNextToken();

		std::string Name = Lexer::IdentifierStr;

		Lexer::GetNextToken();

		if (Lexer::CurrentToken != ':') return AST::ExprError("Expected ':' to set Load type.");

		Lexer::GetNextToken();

		auto T = ParseType();

		Lexer::GetNextToken();

		if (Lexer::CurrentToken != '=') return AST::ExprError("Expected ':' to set Load value.");

		Lexer::GetNextToken();

		auto Value = ParseIdentifier();

		return std::make_unique<AST::Load>(Name, std::move(T), std::move(Value));
	}

	static std::unique_ptr<AST::Expression> ParseStore()
	{
		Lexer::GetNextToken();

		if (Lexer::CurrentToken != '(') return AST::ExprError("Expected '('.");

		Lexer::GetNextToken();

		auto Target = ParseIdentifier();

		if (Lexer::CurrentToken != ',') return AST::ExprError("Expected ','.");

		Lexer::GetNextToken();

		auto Value = ParseExpression();

		if (Lexer::CurrentToken != ')') return AST::ExprError("Expected ')'.");

		Lexer::GetNextToken();

		return std::make_unique<AST::Store>(std::move(Target), std::move(Value));
	}

	static std::unique_ptr<AST::Expression> ParseAlloca()
	{
		Lexer::GetNextToken();

		std::string Name = Lexer::IdentifierStr;

		SetIdentToMainTarget(Name);

		Lexer::GetNextToken();

		if (Lexer::CurrentToken != ':') return AST::ExprError("Expected ':' to set Alloca type.");

		Lexer::GetNextToken();

		auto T = ParseType();

		add_to_variables_list(Name, Lexer::IdentifierStr);

		Lexer::GetNextToken();

		return std::make_unique<AST::Alloca>(std::move(T), Name);
	}

	static std::unique_ptr<AST::Expression> ParseReturn()
	{
		Lexer::GetNextToken();

		if (Lexer::CurrentToken == Token::Load)
		{
			Lexer::GetNextToken();

			auto Ref = ParseIdentifier();

			//if (Lexer::CurrentToken != ';')
			//	return AST::ExprError("Expected ';'");

			return std::make_unique<AST::Return>(std::move(Ref));
		}

		auto Expr = ParseExpression();

		// if (Lexer::CurrentToken != ';') return AST::ExprError("Expected ';'");

		if (dynamic_cast<AST::Variable*>(Expr.get()) || dynamic_cast<AST::Link*>(Expr.get()))
		{
			auto LoadExpr = std::make_unique<AST::Load>("autoLoad", nullptr, std::move(Expr));

			return std::make_unique<AST::Return>(std::move(LoadExpr));	
		}

		return std::make_unique<AST::Return>(std::move(Expr));
	}

	static std::unique_ptr<AST::Type> ParseType()
	{
		if (Lexer::IdentifierStr == "i1" || Lexer::IdentifierStr == "bool") { return std::make_unique<AST::i1>(); }
		else if (Lexer::IdentifierStr == "i32") { return std::make_unique<AST::i32>(); }
		return nullptr;
	}

	static std::unique_ptr<AST::Prototype> ParsePrototype() 
	{
		if (Lexer::CurrentToken != Token::Identifier) return AST::Prototype::Error("Expected function name in prototype");
	
		std::string FnName = Lexer::IdentifierStr;

		Lexer::check_if_identifier_follows_format(0, 1);

		Lexer::GetNextToken();

		if (Lexer::CurrentToken != '(') return AST::Prototype::Error("Expected '(' in prototype");

		Lexer::GetNextToken();

		// Read the list of argument names.
		std::vector<std::unique_ptr<AST::Variable>> ArgNames;
		while (Lexer::CurrentToken == Token::Identifier || Lexer::CurrentToken == ',')
		{
			std::string idName = Lexer::IdentifierStr;

			Lexer::GetNextToken();

			if (Lexer::CurrentToken != ':') return AST::Prototype::Error("Expected ':' to split name and type.");

			Lexer::GetNextToken();

			auto t = ParseType();

			if (t == nullptr) return AST::Prototype::Error("Unknown type found in arguments.");

			add_to_variables_list(idName, Lexer::IdentifierStr);

			Lexer::GetNextToken();

			auto A = std::make_unique<AST::Variable>(std::move(t), idName);
			ArgNames.push_back(std::move(A));

			if (Lexer::CurrentToken != ',') break;

			Lexer::GetNextToken();
		}

		if (Lexer::CurrentToken != ')') return AST::Prototype::Error("Expected ')' in prototype");

		Lexer::GetNextToken();

		if (Lexer::CurrentToken != ':') return AST::Prototype::Error("Expected ':' in prototype");

		Lexer::GetNextToken();

		auto PType = ParseType();

		if (!PType) return AST::Prototype::Error("Expected type in prototype");

		Lexer::GetNextToken();

		return std::make_unique<AST::Prototype>(std::move(PType), FnName, std::move(ArgNames));
	}

	static void ResetTarget()
	{
		Parser::grab_target = true;
		// Parser::last_target = std::string("");
	}

	static std::unique_ptr<AST::Function> ParseFunction() 
	{
		all_variables.clear();

		Lexer::GetNextToken();  // eat def.
		auto Proto = ParsePrototype();
		if (!Proto) { return nullptr; }

		if (Lexer::CurrentToken != '{')
		AST::ExprError("Expected '{' in function");

		Lexer::GetNextToken();

		std::vector<std::unique_ptr<AST::Expression>> Body;
		while (Lexer::CurrentToken != '}') {
			if (auto E = ParseExpression()) Body.push_back(std::move(E));

			if (Lexer::CurrentToken != ';') AST::ExprError("Expected ';' inside function.");
			else ResetTarget();
			
			Lexer::GetNextToken();
		}

		return std::make_unique<AST::Function>(std::move(Proto), std::move(Body));
	}

	static void HandleFunction()
	{
		// std::cout << "Parsed a Function!\n";
		auto Func = ParseFunction();

		llvm::Function* getF = Func->codegen();

		// getF->print(llvm::outs(), nullptr);
	}

	static void MainLoop()
	{
		while (Lexer::CurrentToken != Token::EndOfFile)
		{
			Lexer::GetNextToken();

			if (Lexer::CurrentToken == Token::EndOfFile) break;
			if (Lexer::CurrentToken == Token::Function) HandleFunction();
		}
	}
};

#endif
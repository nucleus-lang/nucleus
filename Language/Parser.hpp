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
	static std::string last_function_call;

	static bool grab_target;
	static std::unordered_map<std::string, std::string> all_variables;
	static std::unordered_map<std::string, std::string> all_loads;
	static std::unordered_map<std::string, std::string> all_prototypes;

	static std::unordered_map<std::string, bool> vars_with_nothing;
	static std::unordered_map<std::string, bool> verified_allocs;

	static std::pair<std::string, std::string> current_function_in_scope;

	template<typename TO, typename FROM>
	static std::unique_ptr<TO> static_unique_pointer_cast (std::unique_ptr<FROM>&& old) {
    	return std::unique_ptr<TO>{static_cast<TO*>(old.release())};
    	// conversion: unique_ptr<FROM>->FROM*->TO*->unique_ptr<TO>
	}

	static void add_to_variables_list(std::string name, std::string type)
	{
		all_variables[name] = type;
	}

	static void add_to_loads_list(std::string name, std::string type)
	{
		all_loads[name] = type;
	}

	static std::string get_type_from_variable(std::string name)
	{
		if (all_variables.find(name) != all_variables.end()) 	return all_variables[name];
		else if (all_loads.find(name) != all_loads.end()) 		return all_loads[name];

		return "";
	}

	static std::string get_type_from_prototype(std::string name)
	{
		if (name == "") 										return "";
		if (all_prototypes.find(name) != all_prototypes.end()) 	return all_prototypes[name];

		return "";
	}

	static int type_to_bit(std::string type)
	{
		if(type == "i1" || type == "bool") return 1;
		else if(type == "i8") return 8;
		else if(type == "i16") return 16;
		else if(type == "i32") return 32;
		else if(type == "i64") return 64;
		else if(type == "i128") return 128;

		else if(type == "u8" || type == "char") return -8;
		else if(type == "u16" || type == "wchar") return -16;
		else if(type == "u32" || type == "uchar") return -32;
		else if(type == "u64") return -64;
		else if(type == "u128") return -128;

		AST::ExprError("Cannot assign a number, boolean and/or character to a '" + type + "' variable.");
		return 0;
	}

	static std::unique_ptr<AST::Expression> ParseNumber(bool in_return = false) 
	{
		std::string type_result;

		if(in_return) 			type_result = current_function_in_scope.second;
		else 					type_result = get_type_from_prototype(Parser::last_function_call);

		if(type_result == "") 	type_result = get_type_from_variable(Parser::last_target);

		if (type_result == "i1" || type_result == "bool")
			return AST::ExprError("Cannot assign a number value to a boolean type variable.");

		bool is_unsigned = false;

		int bit = type_to_bit(type_result);

		if(bit < 0)
		{
			bit *= -1;
			is_unsigned = true;
		}

		auto Result = std::make_unique<AST::Number>(Lexer::NumValString, is_unsigned);
		Result->bit = bit;
	
		Lexer::GetNextToken();
	
		return std::move(Result);
	}

	static std::unique_ptr<AST::Expression> ParseExpression(bool is_in_return = false) 
	{
		auto LHS = ParsePrimary(is_in_return);
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

	static std::unique_ptr<AST::Expression> ThrowNothingError()
	{
		return AST::ExprError("'Nothing' can't be used here! 'Nothing' is used only to mark values as uninitialized!");
	}

	static void CheckIfNothing(AST::Expression* l)
	{
		if(dynamic_cast<AST::Nothing*>(l)) ThrowNothingError();
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

	static void check_if_alloc_is_verified(AST::Variable* v)
	{
		if(all_loads.find(v->Name) == all_loads.end() && !verified_allocs[v->Name])
			AST::ExprError("The variable '" + v->Name + "' is not verified.");
	}

	static void verify_alloc(std::string v)
	{
		if(all_loads.find(v) == all_loads.end())
			verified_allocs[v] = true;
	}

	static void unverify_alloc(std::string v)
	{
		if(all_loads.find(v) == all_loads.end())
			verified_allocs[v] = false;
	}

	static void initialize_alloc(std::string v)
	{
		if(all_loads.find(v) == all_loads.end())
			vars_with_nothing[v] = false;
	}

	static void uninitialize_alloc(std::string v)
	{
		if(all_loads.find(v) == all_loads.end())
			vars_with_nothing[v] = true;
	}

	static std::unique_ptr<AST::Expression> ParseBinaryOperator(std::unique_ptr<AST::Expression> L)
	{
		if (Lexer::CurrentToken == '=')
		{
			Lexer::GetNextToken();
			auto R = ParseExpression();

			if(dynamic_cast<AST::Nothing*>(R.get()))
			{
				if(dynamic_cast<AST::Alloca*>(L.get()) == nullptr)
					return ThrowNothingError();

				return L;
			}
			else if (!dynamic_cast<AST::Number*>(R.get()) && !dynamic_cast<AST::Call*>(R.get()))
			{
				AST::Variable* l_as_var = dynamic_cast<AST::Variable*>(L.get());
				AST::Alloca* l_as_alloc = dynamic_cast<AST::Alloca*>(L.get());

				AST::Variable* r_as_var = dynamic_cast<AST::Variable*>(R.get());

				if(r_as_var) check_if_alloc_is_verified(r_as_var);
				if(l_as_var) {
					verify_alloc(l_as_var->Name);
					initialize_alloc(l_as_var->Name);
				}
				else if(l_as_alloc) {
					verify_alloc(l_as_alloc->VarName);
					initialize_alloc(l_as_alloc->VarName);
				}

				auto RLoad = std::make_unique<AST::Load>("autoLoad", std::make_unique<AST::i32>(), std::move(R));
				return ParseBinaryOperator(std::make_unique<AST::Store>(std::move(L), std::move(RLoad)));
			}
			else
			{
				// auto get_r = dynamic_cast<AST::Number*>(R.get());
				// int32_t r_i32;
				// if (get_r->isInt) r_i32 = get_r->return_i32();
				// StaticAnalyzer::new_static_i32(Parser::last_target, r_i32);

				AST::Variable* l_as_var = dynamic_cast<AST::Variable*>(L.get());
				AST::Alloca* l_as_alloc = dynamic_cast<AST::Alloca*>(L.get());

				if(l_as_var) {
					verify_alloc(l_as_var->Name);
					initialize_alloc(l_as_var->Name);
				}
				else if(l_as_alloc) {
					verify_alloc(l_as_alloc->VarName);
					initialize_alloc(l_as_alloc->VarName);
				}
				else
					return AST::ExprError("What");

				return ParseBinaryOperator(std::make_unique<AST::Store>(std::move(L), std::move(R)));
			}

		}
		else if (Lexer::CurrentToken == '+')
		{
			Lexer::GetNextToken();

			if (Lexer::CurrentToken == '=') Lexer::GetNextToken();

			auto R = ParseExpression();

			CheckIfNothing(R.get());

			AST::Variable* l_as_var = dynamic_cast<AST::Variable*>(L.get());
			if(l_as_var) unverify_alloc(l_as_var->Name);

			AST::Variable* r_as_var = dynamic_cast<AST::Variable*>(R.get());
			if(r_as_var) check_if_alloc_is_verified(r_as_var);

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

			CheckIfNothing(R.get());

			AST::Variable* l_as_var = dynamic_cast<AST::Variable*>(L.get());
			if(l_as_var) unverify_alloc(l_as_var->Name);

			AST::Variable* r_as_var = dynamic_cast<AST::Variable*>(R.get());
			if(r_as_var) check_if_alloc_is_verified(r_as_var);

			// if (dynamic_cast<AST::Number*>(R.get()))
			// {
			// 	auto get_r = dynamic_cast<AST::Number*>(R.get());

			// 	int32_t r_i32;
			// 	if (get_r->isInt) r_i32 = get_r->return_i32();

			// 	StaticAnalyzer::sub_static_i32(Parser::last_target, r_i32);
			// }

			return ParseBinaryOperator(std::make_unique<AST::Sub>(std::move(L), std::move(R)));
		}
		//else if (Lexer::CurrentToken != ';')
		//{
		//	return AST::ExprError("Expected an operator (=, +, -, * or /).");
		//}
		else if(dynamic_cast<AST::Alloca*>(L.get()) != nullptr && Lexer::CurrentToken == ';')
		{
			AST::Alloca* l_as_alloc = dynamic_cast<AST::Alloca*>(L.get());
			if(l_as_alloc) uninitialize_alloc(l_as_alloc->VarName);
		}
		
		return L;
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

		Lexer::check_if_identifier_follows_format(0, 0);

		Lexer::GetNextToken();  // eat identifier.

		Parser::last_function_call = IdName;

		if(Lexer::CurrentToken == '(')
		{
			Lexer::GetNextToken();
			ARGUMENT_LIST() currentArgs;

			while(Lexer::CurrentToken != ')')
			{
				auto Expr = ParseExpression();

				if(Lexer::CurrentToken != ',' && Lexer::CurrentToken != ')')
					return AST::ExprError("Expected ',' or ')' after identifier.");

				currentArgs.push_back(std::move(Expr));

				if(Lexer::CurrentToken == ')')
				{
					Lexer::GetNextToken();
					break;
				}
			}

			if(Lexer::CurrentToken == ')') Lexer::GetNextToken();

			Parser::last_function_call = "";

			return std::make_unique<AST::Call>(IdName, std::move(currentArgs));
		}

		SetIdentToMainTarget(IdName);

		return std::make_unique<AST::Variable>(nullptr, IdName);
	}

	static std::unique_ptr<AST::Expression> ParsePrimary(bool is_in_return = false) 
	{
		if (Lexer::CurrentToken == Token::Identifier) return ParseIdentifier();
		else if (Lexer::CurrentToken == Token::Number)  return ParseNumber(is_in_return);
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
		else if (Lexer::CurrentToken == Token::Nothing) return ParseNothing();
		else  return AST::ExprError("unknown token when expecting an expression");
	}

	static std::unique_ptr<AST::Expression> ParseNothing()
	{
		vars_with_nothing[Parser::last_target] = true;

		Lexer::GetNextToken();

		return std::make_unique<AST::Nothing>();
	}

	static std::unique_ptr<AST::Expression> ParseVerify()
	{
		Lexer::GetNextToken();

		if (Lexer::CurrentToken == ';')
		{
			if (Parser::last_target != "")
			{
				auto I = std::make_unique<AST::Variable>(nullptr, Parser::last_target);

				verify_alloc(Parser::last_target);

				return std::make_unique<AST::Link>(std::move(I), nullptr);
			}
			else
			{
				AST::ExprError("Last Target is not found!");
			}
		}

		auto I = ParseIdentifier();

		verify_alloc(Parser::last_identifier);

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

		unverify_alloc(Parser::last_identifier);

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

		unverify_alloc(Parser::last_identifier);

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

		add_to_loads_list(Name, Lexer::IdentifierStr);

		Lexer::GetNextToken();

		if (Lexer::CurrentToken != '=') return AST::ExprError("Expected ':' to set Load value.");

		Lexer::GetNextToken();

		auto Value = ParseIdentifier();

		AST::Variable* value_as_var = dynamic_cast<AST::Variable*>(Value.get());
		if(value_as_var) check_if_variable_is_uninitialized(value_as_var, 1);

		return std::make_unique<AST::Load>(Name, std::move(T), std::move(Value));
	}

	static std::unique_ptr<AST::Expression> ParseStore()
	{
		Lexer::GetNextToken();

		if (Lexer::CurrentToken != '(') return AST::ExprError("Expected '('.");

		Lexer::GetNextToken();

		auto Target = ParseIdentifier();

		initialize_alloc(Parser::last_identifier);
		verify_alloc(Parser::last_identifier);

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

		auto new_alloc = std::make_unique<AST::Alloca>(std::move(T), Name);

		//new_alloc->is_unsigned = T->is_unsigned;

		return new_alloc;
	}

	// uninitialized_type guide:
	// 0 = The variable is being used in the 'return' keyword.
	// 1 = The variable is being passed to a 'load' instruction.
	static void check_if_variable_is_uninitialized(AST::Variable* v, int uninitialized_type = 0)
	{
		if(vars_with_nothing[v->Name] == true)
		{
			if(uninitialized_type == 0)
				AST::ExprError("You can't return uninitialized variables. '" + v->Name + "' is uninitialized.");
			else if(uninitialized_type == 1)
				AST::ExprError("You can't pass uninitialized variables to 'load'. '" + v->Name + "' is uninitialized.");
		}
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

		auto Expr = ParseExpression(true);

		// if (Lexer::CurrentToken != ';') return AST::ExprError("Expected ';'");

		if (dynamic_cast<AST::Variable*>(Expr.get()) || dynamic_cast<AST::Link*>(Expr.get()))
		{
			AST::Variable* expr_as_v = dynamic_cast<AST::Variable*>(Expr.get());
			if(expr_as_v)
			{
				check_if_variable_is_uninitialized(expr_as_v);
				check_if_alloc_is_verified(expr_as_v);
			}

			auto LoadExpr = std::make_unique<AST::Load>("autoLoad", nullptr, std::move(Expr));

			return std::make_unique<AST::Return>(std::move(LoadExpr));	
		}

		return std::make_unique<AST::Return>(std::move(Expr));
	}

	static std::unique_ptr<AST::Type> ParseType()
	{
		std::unique_ptr<AST::Type> unsigned_type = nullptr;

		if (Lexer::IdentifierStr == "i1" || Lexer::IdentifierStr == "bool") { return std::make_unique<AST::i1>(); }
		else if (Lexer::IdentifierStr == "i8") { return std::make_unique<AST::i8>(); }
		else if (Lexer::IdentifierStr == "i16") { return std::make_unique<AST::i16>(); }
		else if (Lexer::IdentifierStr == "i32") { return std::make_unique<AST::i32>(); }
		else if (Lexer::IdentifierStr == "i64") { return std::make_unique<AST::i64>(); }
		else if (Lexer::IdentifierStr == "i128") { return std::make_unique<AST::i128>(); }

		else if(Lexer::IdentifierStr == "u8" || Lexer::IdentifierStr == "char") { unsigned_type = std::make_unique<AST::i8>(); }
		else if(Lexer::IdentifierStr == "u16" || Lexer::IdentifierStr == "wchar") { unsigned_type = std::make_unique<AST::i16>(); }
		else if(Lexer::IdentifierStr == "u32" || Lexer::IdentifierStr == "uchar") { unsigned_type = std::make_unique<AST::i32>(); }
		else if(Lexer::IdentifierStr == "u64") { unsigned_type = std::make_unique<AST::i64>(); }
		else if(Lexer::IdentifierStr == "u128") { unsigned_type = std::make_unique<AST::i128>(); }

		unsigned_type->is_unsigned = true;

		return unsigned_type;
	}

	static std::unique_ptr<AST::Prototype> ParsePrototype(bool set_in_scope_function = false) 
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

		std::string type_as_string = Lexer::IdentifierStr;

		if(set_in_scope_function) set_new_function_in_scope(FnName, Lexer::IdentifierStr);

		if (!PType) return AST::Prototype::Error("Expected type in prototype");

		Lexer::GetNextToken();

		Parser::all_prototypes[FnName] = type_as_string;

		return std::make_unique<AST::Prototype>(std::move(PType), FnName, std::move(ArgNames), type_as_string);
	}

	static void ResetTarget()
	{
		Parser::grab_target = true;
		// Parser::last_target = std::string("");
	}

	static void set_new_function_in_scope(std::string name, std::string type)
	{
		current_function_in_scope = std::make_pair(name, type);
	}

	static std::unique_ptr<AST::Function> ParseFunction() 
	{
		all_variables.clear();

		Lexer::GetNextToken();  // eat def.
		auto Proto = ParsePrototype(true);
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
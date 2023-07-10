#ifndef PARSER_HPP
#define PARSER_HPP

#include "Lexer.hpp"
#include "IncludeCInternals.hpp"
#include "ErrorHandler.hpp"
#include "StaticAnalyzer.hpp"
#include "TodoList.hpp"
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
	static std::unordered_map<std::string, AST::Type*> all_arrays;

	static std::unordered_map<std::string, bool> vars_with_nothing;
	static std::unordered_map<std::string, bool> verified_allocs;

	static bool is_exit_declared;

	static std::pair<std::string, std::string> current_function_in_scope;

	static int random_global_id;

	static bool dont_share_history;

	static bool use_architechture_bit;

	template<typename TO, typename FROM>
	static std::unique_ptr<TO> static_unique_pointer_cast (std::unique_ptr<FROM>&& old) {
    	return std::unique_ptr<TO>{static_cast<TO*>(old.release())};
    	// conversion: unique_ptr<FROM>->FROM*->TO*->unique_ptr<TO>
	}

	static void add_to_array_list(std::string name, AST::Type* t) {

		all_arrays[name] = t;
	}

	static bool check_if_is_array(AST::Type* t) {

		return dynamic_cast<AST::Array*>(t) != nullptr;
	}

	static bool check_if_is_in_array_list(std::string name) {

		return all_arrays.find(name) != all_arrays.end();
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

		//AST::ExprError("Cannot assign a number, boolean and/or character to a '" + type + "' variable.");
		return 32;
	}

	static std::unique_ptr<AST::Expression> ParseNumber(bool in_return = false)
	{
		std::string type_result;

		if(Parser::use_architechture_bit)		type_result = "i32";
		else if(in_return) 						type_result = current_function_in_scope.second;
		else 									type_result = get_type_from_prototype(Parser::last_function_call);

		Parser::use_architechture_bit = false;

		if(type_result == "") {
			type_result = get_type_from_variable(Parser::last_target);

			if (type_result == "i1" || type_result == "bool")
				return AST::ExprError("Cannot assign a number value to a boolean type variable.");
		}

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

	static void RuleCheck_Expressions(bool is_beginning, AST::Expression* ref)
	{
		if(is_beginning && dynamic_cast<AST::Call*>(ref))
			AST::ExprError("Functions can't be implicitly called outside of variables.");
	}

	static std::unique_ptr<AST::Expression> ParseExpression(bool is_in_return = false)
	{
		bool is_beginning = Parser::grab_target;

		auto LHS = ParsePrimary(is_in_return);
		if (!LHS) return nullptr;

		RuleCheck_Expressions(is_beginning, LHS.get());

		return ParseBinaryOperator(std::move(LHS));
	}

	static std::unique_ptr<AST::Expression> ParseBoolValue(bool is_true, bool in_return = false)
	{
		std::unique_ptr<AST::Number> Result = nullptr;

		std::string type_result;

		if(in_return) 			type_result = current_function_in_scope.second;
		else 					type_result = get_type_from_prototype(Parser::last_function_call);

		if(type_result == "") {
			type_result = get_type_from_variable(Parser::last_target);

			if (type_result != "i1" && type_result != "bool")
				return AST::ExprError("Cannot assign a boolean value to a number type variable.");
		}

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

			bool is_compare = false;

			if (Lexer::CurrentToken == '=') {
				is_compare = true;
				Lexer::GetNextToken();
			}

			auto R = ParseExpression();

			if(dynamic_cast<AST::NewArray*>(R.get()) && dynamic_cast<AST::Alloca*>(L.get()))
			{
				auto l_as_alloc = dynamic_cast<AST::Alloca*>(L.get());

				if(!l_as_alloc)
					AST::ExprError("'new_array()' can't be used in this variable type.");

				auto l_alloc_array_type = dynamic_cast<AST::Array*>(l_as_alloc->T.get());

				if(!l_alloc_array_type)
					AST::ExprError("'" + l_as_alloc->VarName + "' is not an Array.");

				auto RPtr = dynamic_cast<AST::NewArray*>(R.get());
				auto RFinal = std::make_unique<AST::NewArray>(std::move(RPtr->items));
				RFinal->target = std::move(L);

				auto newT = std::make_unique<AST::Array>(std::move(l_alloc_array_type->childType), RFinal->items.size());
				l_as_alloc->T = std::move(newT);

				all_arrays[l_as_alloc->VarName] = l_as_alloc->T.get();

				if(l_alloc_array_type->amount == 0)
					RFinal->is_resizable = true;

				return RFinal;
			}

			if(dynamic_cast<AST::GetElement*>(R.get()) && dynamic_cast<AST::Alloca*>(L.get()))
			{
				auto RPtr = dynamic_cast<AST::GetElement*>(R.get());

				auto RFinal = std::make_unique<AST::GetElement>(std::move(RPtr->target), std::move(RPtr->number));

				auto LPtr = dynamic_cast<AST::Alloca*>(L.get());

				RFinal->GetElementName = LPtr->VarName;
				RFinal->set_var = true;

				verify_alloc(LPtr->VarName);
				initialize_alloc(LPtr->VarName);

				return RFinal;
			}

			if(is_compare) {

				auto C = std::make_unique<AST::Compare>(std::move(L), std::move(R), get_compare_type("is_equals"));
				C->dont_share_history = Parser::dont_share_history;

				return ParseBinaryOperator(std::move(C));
			}

			if(dynamic_cast<AST::Nothing*>(R.get()))
			{
				if(dynamic_cast<AST::Alloca*>(L.get()) == nullptr)
					return ThrowNothingError();

				return L;
			}
			else if (!dynamic_cast<AST::Number*>(R.get()) &&
					 !dynamic_cast<AST::Call*>(R.get()) &&
					 !dynamic_cast<AST::Compare*>(R.get()))
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

				if(Parser::all_loads.find(r_as_var->Name) == Parser::all_loads.end())
				{
					auto RLoad = std::make_unique<AST::Load>("autoLoad", std::make_unique<AST::i32>(), std::move(R));
					return ParseBinaryOperator(std::make_unique<AST::Store>(std::move(L), std::move(RLoad)));
				}
				else { return ParseBinaryOperator(std::make_unique<AST::Store>(std::move(L), std::move(R))); }
			}
			else
			{
				if(dynamic_cast<AST::Pure*>(L.get()) && dynamic_cast<AST::Number*>(R.get()))
				{
					auto A = std::make_unique<AST::Add>(std::move(L), std::move(R));
					A->dont_share_history = Parser::dont_share_history;

					return ParseBinaryOperator(std::move(A));
				}


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
					if(dynamic_cast<AST::Call*>(R.get()))
						L->is_initialized_by_call = true;

					verify_alloc(l_as_alloc->VarName);
					initialize_alloc(l_as_alloc->VarName);
				}
				else if(dynamic_cast<AST::GetElement*>(L.get()))
				{
					return AST::ExprError("Due to safety mechanisms, get_element() can't be used to set elements.");
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

			auto A = std::make_unique<AST::Add>(std::move(L), std::move(R));
			A->dont_share_history = Parser::dont_share_history;

			return ParseBinaryOperator(std::move(A));
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

			auto S = std::make_unique<AST::Sub>(std::move(L), std::move(R));
			S->dont_share_history = Parser::dont_share_history;

			return ParseBinaryOperator(std::move(S));
		}
		//else if (Lexer::CurrentToken != ';')
		//{
		//	return AST::ExprError("Expected an operator (=, +, -, * or /).");
		//}
		else if(Lexer::CurrentToken == '>' || Lexer::CurrentToken == '<' || Lexer::CurrentToken == '!')
		{
			std::string cmp_str = "";

			bool not_equals = false;

			if(Lexer::CurrentToken == '>') { cmp_str = "is_more_than"; }
			if(Lexer::CurrentToken == '<') { cmp_str = "is_less_than"; }
			if(Lexer::CurrentToken == '!') { cmp_str = "is_not_equals"; not_equals = true; }

			Lexer::GetNextToken();

			if (Lexer::CurrentToken == '=') {

				if(!not_equals) { cmp_str += "_or_equals"; }
				Lexer::GetNextToken();
			}
			else if(not_equals) { AST::ExprError("Expected '=' to indicate 'not equals' comparison."); }

			auto R = ParseExpression();

			std::unique_ptr<AST::Compare> C;

			C = std::make_unique<AST::Compare>(std::move(L), std::move(R), get_compare_type(cmp_str));
			C->dont_share_history = Parser::dont_share_history;

			return ParseBinaryOperator(std::move(C));
		}
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

		if(Lexer::CurrentToken == '(')
		{
			Parser::last_function_call = IdName;

			Lexer::GetNextToken();
			ARGUMENT_LIST() currentArgs;
			ARGUMENT_LIST() inst_before_arg;

			while(Lexer::CurrentToken != ')')
			{
				auto Expr = ParseExpression();
				Expr->dont_share_history = true;

				// AUTO-PURIFIER MY BELOVED <3

				std::string title = "autoPure" + std::to_string(Parser::random_global_id);
				Parser::random_global_id++;

				std::string get_type = get_type_from_variable(Parser::last_target);

				add_to_loads_list(title, get_type);
				auto T = ParseType(get_type);

				auto VE = dynamic_cast<AST::Variable*>(Expr.get());
				AST::Array* a = nullptr;

				if(VE) { a = dynamic_cast<AST::Array*>(all_arrays[VE->Name]); }

				inst_before_arg.push_back(std::make_unique<AST::Pure>(title, std::move(T), std::move(Expr)));
				currentArgs.push_back(std::make_unique<AST::Variable>(nullptr, title));

				if(VE)
				{
					if(check_if_is_in_array_list(VE->Name))
					{
						if(!a) { std::cout << "Internal Error.\n"; exit(1); }

						inst_before_arg.push_back(std::make_unique<AST::Pure>(IdName + "_length", std::make_unique<AST::i32>(), std::make_unique<AST::Number>(std::to_string(a->amount - 1))));
						currentArgs.push_back(std::make_unique<AST::Variable>(nullptr, IdName + "_length"));
					}
				}

				if(Lexer::CurrentToken != ',' && Lexer::CurrentToken != ')')
					return AST::ExprError("Expected ',' or ')' after identifier.");

				if(Lexer::CurrentToken == ')')
				{
					Lexer::GetNextToken();
					break;
				}

				Lexer::GetNextToken();
			}

			if(Lexer::CurrentToken == ')') Lexer::GetNextToken();

			Parser::last_function_call = "";

			return std::make_unique<AST::Call>(IdName, std::move(currentArgs), std::move(inst_before_arg));
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
		else if (Lexer::CurrentToken == Token::True) return ParseBoolValue(true, is_in_return);
		else if (Lexer::CurrentToken == Token::False) return ParseBoolValue(false, is_in_return);
		else if (Lexer::CurrentToken == Token::Nothing) return ParseNothing();
		else if (Lexer::CurrentToken == Token::Compare) return ParseCompare();
		else if (Lexer::CurrentToken == Token::If) return ParseIf();
		else if (Lexer::CurrentToken == Token::Pure) return ParsePure();
		else if (Lexer::CurrentToken == Token::While) return ParseWhile();
		else if (Lexer::CurrentToken == Token::Todo) return ParseTodo();
		else if (Lexer::CurrentToken == Token::GetElement) return ParseGetElement();
		else if (Lexer::CurrentToken == Token::NewArray) return ParseNewArray();
		else if (Lexer::CurrentToken == Token::String) return ParseNewString();
		else if (Lexer::CurrentToken == Token::Exit) return ParseExit();
		else if (Lexer::CurrentToken == Token::IntCast) return ParseIntCast();
		else return AST::ExprError("Unknown token when expecting an expression.");
	}

	static std::unique_ptr<AST::Expression> ParseIntCast()
	{
		Lexer::GetNextToken();

		if(Lexer::CurrentToken != '(') { AST::ExprError("Expected '(' to add int cast arguments."); }

		Lexer::GetNextToken();

		auto I = ParseExpression();

		if(Lexer::CurrentToken != ',') { AST::ExprError("Expected ',' to separate int cast arguments."); }

		Lexer::GetNextToken();

		auto T = ParseType();

		Lexer::GetNextToken();

		if(Lexer::CurrentToken != ')') { AST::ExprError("Expected ')' to close int cast arguments."); }

		Lexer::GetNextToken();

		return std::make_unique<AST::IntCast>(std::move(I), std::move(T));
	}

	static void DeclareExit()
	{
		auto exit_type = std::make_unique<AST::Void>();

		std::vector<std::unique_ptr<AST::Variable>> args;

		auto exit_param_type = std::make_unique<AST::i32>();
		auto exit_param = std::make_unique<AST::Variable>(std::move(exit_param_type), "condition");

		args.push_back(std::move(exit_param));

		std::string name = "exit";
		std::string type_string = "void";

		auto exit_proto = std::make_unique<AST::Prototype>(std::move(exit_type), name, std::move(args), type_string);

		Parser::is_exit_declared = true;

		AST::exit_declare = exit_proto->codegen();
	}

	static std::unique_ptr<AST::Expression> ParseExit()
	{
		if(!Parser::is_exit_declared)
		{
			DeclareExit();
		}

		Lexer::GetNextToken();

		auto T = ParseNumber();

		return std::make_unique<AST::Exit>(std::move(T));
	}

	static std::unique_ptr<AST::Expression> ParseNewString()
	{
		std::string s = Lexer::StringString;

		ARGUMENT_LIST() args;

		for(auto i : s)
		{
			int c = int(i);
			auto character = std::make_unique<AST::Number>(std::to_string(c));
			character->bit = 8;

			args.push_back(std::move(character));
		}

		auto end = std::make_unique<AST::Number>("0");
		end->bit = 8;
		args.push_back(std::move(end));

		Lexer::GetNextToken();

		return std::make_unique<AST::NewArray>(std::move(args));
	}

	static std::unique_ptr<AST::Expression> ParseNewArray()
	{
		Lexer::GetNextToken();

		if(Lexer::CurrentToken != '(') { AST::ExprError("Expected '(' to add new array arguments."); }

		Lexer::GetNextToken();

		ARGUMENT_LIST() args;

		while(Lexer::CurrentToken != ')')
		{
			auto I = ParseExpression();

			args.push_back(std::move(I));

			if(Lexer::CurrentToken == ',') { Lexer::GetNextToken(); }
			else { break; }
		}

		if(Lexer::CurrentToken != ')') { AST::ExprError("Expected ')' to close new array arguments."); }

		Lexer::GetNextToken();

		return std::make_unique<AST::NewArray>(std::move(args));
	}

	static std::unique_ptr<AST::Expression> ParseGetElement()
	{
		if(!Parser::is_exit_declared)
		{
			DeclareExit();
		}
		
		Lexer::GetNextToken();

		if(Lexer::CurrentToken != '(') { AST::ExprError("Expected '(' to add get_element arguments."); }

		Lexer::GetNextToken();

		auto A = ParseIdentifier();

		if(!check_if_is_in_array_list(Lexer::IdentifierStr)) { AST::ExprError("'" + Lexer::IdentifierStr + "' is not an array or a type that is able to use 'get_element()'."); }

		if(Lexer::CurrentToken != ',') { AST::ExprError("Expected ',' to separate get_element arguments."); }

		Lexer::GetNextToken();

		Parser::use_architechture_bit = true;
		auto N = ParseExpression();

		if(Lexer::CurrentToken != ')') { AST::ExprError("Expected ')' to close get_element arguments."); }

		Lexer::GetNextToken();

		return std::make_unique<AST::GetElement>(std::move(A), std::move(N));
	}

	static std::unique_ptr<AST::Expression> ParseTodo()
	{
		int get_line = Lexer::Line;
		int get_column = Lexer::Column;

		Lexer::GetNextToken();

		if(Lexer::CurrentToken != Token::String) { AST::ExprError("String not found."); }

		std::string s = Lexer::StringString;

		TodoList::add(s, get_line, get_column);

		Lexer::GetNextToken();

		return std::make_unique<AST::Todo>();
	}

	static std::unique_ptr<AST::Expression> ParseWhile()
	{
		Lexer::GetNextToken();

		auto Cond = ParseExpression();

		if(Lexer::CurrentToken != '{') AST::ExprError("Expected '{'.");

		Lexer::GetNextToken();

		ARGUMENT_LIST() Body;

		while (Lexer::CurrentToken != '}') {
			if (auto E = ParseExpression()) Body.push_back(std::move(E));

			if (Lexer::CurrentToken != ';') AST::ExprError("Expected ';' inside of while block.");
			else ResetTarget();

			Lexer::GetNextToken();
		}

		//Lexer::GetNextToken();

		if(Lexer::CurrentToken != '}') AST::ExprError("Expected '}'.");

		Lexer::GetNextToken();

		return std::make_unique<AST::Loop>("while", std::move(Cond), std::move(Body));
	}

	static std::unique_ptr<AST::Expression> ParsePure()
	{
		Lexer::GetNextToken();

		std::string IdName = Lexer::IdentifierStr;

		SetIdentToMainTarget(IdName);

		Lexer::GetNextToken();

		if(Lexer::CurrentToken != ':') AST::ExprError("Expected ':'.");

		Lexer::GetNextToken();

		auto T = ParseType();

		std::string type = Lexer::IdentifierStr;

		Lexer::GetNextToken();

		if(Lexer::CurrentToken != '=') AST::ExprError("Expected '='.");

		Lexer::GetNextToken();

		add_to_loads_list(IdName, type);

		Parser::dont_share_history = true;

		auto Expr = ParseExpression();

		return std::make_unique<AST::Pure>(IdName, std::move(T), std::move(Expr));
	}

	static std::unique_ptr<AST::Expression> ParseIf()
	{
		Lexer::GetNextToken();

		auto Condition = ParseExpression();

		if(Lexer::CurrentToken != '{') AST::ExprError("Expected '{'.");

		Lexer::GetNextToken();

		ARGUMENT_LIST() IfBody;
		ARGUMENT_LIST() ElseBody;

		while (Lexer::CurrentToken != '}') {
			if (auto E = ParseExpression()) IfBody.push_back(std::move(E));

			if (Lexer::CurrentToken != ';') AST::ExprError("Expected ';' inside if block.");
			else ResetTarget();

			Lexer::GetNextToken();
		}

		Lexer::GetNextToken();

		if(Lexer::CurrentToken == Token::Else) {

			Lexer::GetNextToken();

			if(Lexer::CurrentToken == '{')
			{
				Lexer::GetNextToken();

				while (Lexer::CurrentToken != '}') {
					if (auto E = ParseExpression()) ElseBody.push_back(std::move(E));

					if (Lexer::CurrentToken != ';') AST::ExprError("Expected ';' inside else block.");
					else ResetTarget();

					Lexer::GetNextToken();
				}

				Lexer::GetNextToken();
			}
			else if(Lexer::CurrentToken == Token::If)
			{
				if (auto E = ParseIf()) ElseBody.push_back(std::move(E));
			}
			else AST::ExprError("Expected '{'.");
		}

		return std::make_unique<AST::If>(std::move(Condition), std::move(IfBody), std::move(ElseBody));
	}

	static std::unique_ptr<AST::Expression> ParseNothing()
	{
		vars_with_nothing[Parser::last_target] = true;

		Lexer::GetNextToken();

		return std::make_unique<AST::Nothing>();
	}

	static int get_compare_type(std::string id)
	{
		if(id == "is_less_than") return 0;
		else if(id == "is_more_than") return 1;
		else if(id == "is_equals") return 2;
		else if(id == "is_not_equals") return 3;
		else if(id == "is_less_than_or_equals") return 4;
		else if(id == "is_more_than_or_equals") return 5;

		AST::ExprError("Compare type unknown, found '" + id + "'.");
		return -1;
	}

	static std::unique_ptr<AST::Expression> ChangeNumberType(std::unique_ptr<AST::Expression> N, std::string type)
	{
		AST::Number* n_as_number = dynamic_cast<AST::Number*>(N.get());
		if(n_as_number)
		{
			auto final_number = std::make_unique<AST::Number>(n_as_number->valueAsString, n_as_number->is_unsigned);
			final_number->bit = type_to_bit(type);
			return final_number;
		}

		return N;
	}

	static void initialize_all_core_protos() {

		Parser::all_prototypes["compare.is_less_than"] = "i1";
		Parser::all_prototypes["compare.is_more_than"] = "i1";
		Parser::all_prototypes["compare.is_equals"] = "i1";
		Parser::all_prototypes["compare.is_not_equals"] = "i1";
		Parser::all_prototypes["compare.is_less_than_or_equals"] = "i1";
		Parser::all_prototypes["compare.is_more_than_or_equals"] = "i1";
	}

	static std::unique_ptr<AST::Expression> ParseCompare()
	{
		Lexer::GetNextToken();

		if(Lexer::CurrentToken != '.') AST::ExprError("Expected '.'.");

		Lexer::GetNextToken();

		int cmp_type = get_compare_type(Lexer::IdentifierStr);

		Parser::last_function_call = "compare." + Lexer::IdentifierStr;

		Lexer::GetNextToken();

		if(Lexer::CurrentToken != '(') AST::ExprError("Expected '('.");

		Lexer::GetNextToken();

		auto ExprA = ParseExpression();

		std::string left_identifier = Parser::last_identifier;

		AST::Variable* a_as_var = dynamic_cast<AST::Variable*>(ExprA.get());
		if(a_as_var) check_if_alloc_is_verified(a_as_var);

		if(Lexer::CurrentToken != ',') AST::ExprError("Expected ','.");

		Lexer::GetNextToken();

		auto ExprB_Pro = ParseExpression();

		auto ExprB = ChangeNumberType(std::move(ExprB_Pro), get_type_from_variable(left_identifier));

		AST::Variable* b_as_var = dynamic_cast<AST::Variable*>(ExprB.get());
		if(b_as_var) check_if_alloc_is_verified(b_as_var);

		if(Lexer::CurrentToken != ')') AST::ExprError("Expected ')'.");

		Lexer::GetNextToken();

		Parser::last_function_call = "";

		return std::make_unique<AST::Compare>(std::move(ExprA), std::move(ExprB), cmp_type);
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

		auto Value = ParseExpression();

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

		if(check_if_is_array(T.get()))
		{
			add_to_array_list(Name, T.get());
		}

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

	static std::unique_ptr<AST::Type> ParseType(std::string n = "")
	{
		std::string final_name;

		if(n == "") final_name = Lexer::IdentifierStr;
		else final_name = n;

		std::unique_ptr<AST::Type> unsigned_type = nullptr;

		if (final_name == "i1" || final_name == "bool") { return std::make_unique<AST::i1>(); }
		else if (final_name == "i8") { return std::make_unique<AST::i8>(); }
		else if (final_name == "i16") { return std::make_unique<AST::i16>(); }
		else if (final_name == "i32") { return std::make_unique<AST::i32>(); }
		else if (final_name == "i64") { return std::make_unique<AST::i64>(); }
		else if (final_name == "i128") { return std::make_unique<AST::i128>(); }

		else if(final_name == "u8" || final_name == "char") { unsigned_type = std::make_unique<AST::i8>(); }
		else if(final_name == "u16" || final_name == "wchar") { unsigned_type = std::make_unique<AST::i16>(); }
		else if(final_name == "u32" || final_name == "uchar") { unsigned_type = std::make_unique<AST::i32>(); }
		else if(final_name == "u64") { unsigned_type = std::make_unique<AST::i64>(); }
		else if(final_name == "u128") { unsigned_type = std::make_unique<AST::i128>(); }

		else if (final_name == "void") { return std::make_unique<AST::Void>(); }

		else if(final_name == "Array") {

			Lexer::GetNextToken();

			if(Lexer::CurrentToken != '<')
				AST::ExprError("Expected '<' to specify array arguments.");

			Lexer::GetNextToken();

			auto T = ParseType();

			Lexer::GetNextToken();

			int amount = 0;

			if(Lexer::CurrentToken == ',')
			{
				Lexer::GetNextToken();

				amount = std::stoi(Lexer::NumValString);

				Lexer::GetNextToken();
			}
			else if(Lexer::CurrentToken != '>')
				AST::ExprError("Expected '>' to close array arguments.");

			return std::make_unique<AST::Array>(std::move(T), amount);
		}

		if(unsigned_type == nullptr) AST::ExprError("Unknown type '" + Lexer::IdentifierStr + "'.");

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

			t->is_in_prototype = true;

			auto il = std::make_unique<AST::Variable>(std::make_unique<AST::i32>(), idName + "_length");

			auto get_a = dynamic_cast<AST::Array*>(t.get());
			bool is_array = get_a != nullptr;

			if(is_array)
				add_to_array_list(idName, t.get());

			add_to_loads_list(idName, Lexer::IdentifierStr);

			Lexer::GetNextToken();

			std::unique_ptr<AST::Variable> A = std::make_unique<AST::Variable>(std::move(t), idName);

			A->is_argument = true;

			ArgNames.push_back(std::move(A));

			if(is_array)
			{
				ArgNames.push_back(std::move(il));
			}

			if (Lexer::CurrentToken != ',') break;

			Lexer::GetNextToken();
		}

		if (Lexer::CurrentToken != ')') return AST::Prototype::Error("Expected ')' in prototype");

		Lexer::GetNextToken();

		std::unique_ptr<AST::Type> PType;
		std::string type_as_string;

		if (Lexer::CurrentToken != ':') { 

			PType = std::make_unique<AST::Void>();
			PType->is_in_prototype = true;

			type_as_string = "void";

			if(set_in_scope_function) set_new_function_in_scope(FnName, type_as_string);
		}
		else {

			Lexer::GetNextToken();

			PType = ParseType();
			PType->is_in_prototype = true;

			type_as_string = Lexer::IdentifierStr;

			if(set_in_scope_function) set_new_function_in_scope(FnName, type_as_string);

			if (!PType) return AST::Prototype::Error("Expected type in prototype");

			Lexer::GetNextToken();
		}

		std::string calling_convention;

		if(Lexer::IdentifierStr == "cc")
		{
			Lexer::GetNextToken();

			if(Lexer::CurrentToken != '(') return AST::Prototype::Error("Expected '(' to begin calling convention in prototype.");

			Lexer::GetNextToken();

			calling_convention = Lexer::IdentifierStr;

			Lexer::GetNextToken();

			if(Lexer::CurrentToken != ')') return AST::Prototype::Error("Expected ')' to end calling convention in prototype.");

			Lexer::GetNextToken();
		}

		Parser::all_prototypes[FnName] = type_as_string;

		return std::make_unique<AST::Prototype>(std::move(PType), FnName, std::move(ArgNames), type_as_string, calling_convention);
	}

	static void ResetTarget()
	{
		Parser::grab_target = true;
		Parser::dont_share_history = false;
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

		if(dynamic_cast<AST::If*>(Body[Body.size() - 1].get()))
			Body[Body.size() - 1]->uncontinue = true;

		return std::make_unique<AST::Function>(std::move(Proto), std::move(Body));
	}

	static std::unique_ptr<AST::Prototype> ParseExtern()
	{
		Lexer::GetNextToken();
		auto Proto = ParsePrototype();

		if (Lexer::CurrentToken != ';') AST::ExprError("Expected ';' to end extern.");

		return Proto;
	}

	static void HandleExtern()
	{
		if (auto ProtoAST = Parser::ParseExtern()) {
			if (auto *FnIR = ProtoAST->codegen())
				AST::FunctionProtos[ProtoAST->getName()] = std::move(ProtoAST);
		}
	}

	static std::unique_ptr<AST::Atom> ParseAtom()
	{
		Lexer::GetNextToken();

		std::string name = Lexer::IdentifierStr;

		Lexer::GetNextToken();

		if(Lexer::CurrentToken != '(') AST::ExprError("Expected '('.");

		Lexer::GetNextToken();

		ATOM_ARG_LIST() atom_args;

		while(Lexer::CurrentToken == Token::Identifier || Lexer::CurrentToken == ',')
		{
			std::string arg_name = Lexer::IdentifierStr;

			Lexer::GetNextToken();

			if(Lexer::CurrentToken != ':') AST::ExprError("Expected ':'.");

			Lexer::GetNextToken();

			auto T = ParseType();

			atom_args.push_back(std::make_pair(arg_name, std::move(T)));

			add_to_loads_list(arg_name, Lexer::IdentifierStr);

			Lexer::GetNextToken();

			if(Lexer::CurrentToken != ',') break;

			Lexer::GetNextToken();
		}

		if(Lexer::CurrentToken != ')') AST::ExprError("Expected ')'.");

		Lexer::GetNextToken();

		std::unique_ptr<AST::Type> AtomT;

		if(Lexer::CurrentToken != ':') { AtomT = std::make_unique<AST::Void>(); }
		else {

			Lexer::GetNextToken();

			AtomT = ParseType();

			Lexer::GetNextToken();
		}

		if(Lexer::CurrentToken != '{') AST::ExprError("Expected '{'.");

		Lexer::GetNextToken();

		ARGUMENT_LIST() Body;
		while (Lexer::CurrentToken != '}') {
			if (auto E = ParseExpression()) Body.push_back(std::move(E));

			if (Lexer::CurrentToken != ';') AST::ExprError("Expected ';' inside function.");
			else ResetTarget();

			Lexer::GetNextToken();
		}

		if(dynamic_cast<AST::If*>(Body[Body.size() - 1].get()))
			Body[Body.size() - 1]->uncontinue = true;

		return std::make_unique<AST::Atom>(name, std::move(atom_args), std::move(AtomT), std::move(Body));
	}

	static void HandleAtom()
	{
		if(auto AtomV = Parser::ParseAtom()) {

			std::string n = AtomV->Name;
			AST::Atoms[n] = std::move(AtomV);
		}
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
		initialize_all_core_protos();
		IncludeCInternals::start();

		while (Lexer::CurrentToken != Token::EndOfFile)
		{
			Lexer::GetNextToken();

			if (Lexer::CurrentToken == Token::EndOfFile) 	break;
			if (Lexer::CurrentToken == Token::Function) 	HandleFunction();
			if (Lexer::CurrentToken == Token::Extern) 		HandleExtern();
			if (Lexer::CurrentToken == Token::Atom) 		HandleAtom();
			if (Lexer::CurrentToken == Token::Todo) 		ParseTodo();
		}
	}
};

#endif
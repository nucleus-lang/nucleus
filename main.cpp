#include <iostream>
#include "Language/Lexer.hpp"
#include "Language/Parser.hpp"
#include "Language/AST.hpp"
#include "Language/CodeGen.hpp"
#include "Tooling/Project.hpp"
#include <fstream>

std::string Parser::last_identifier;
std::string Parser::last_target;
std::string Parser::last_function_call;
bool Parser::grab_target = true;

std::unordered_map<std::string, std::string> Parser::all_variables;
std::unordered_map<std::string, std::string> Parser::all_loads;
std::unordered_map<std::string, std::string> Parser::all_prototypes;
std::unordered_map<std::string, AST::Type*> Parser::all_arrays;

bool Parser::is_exit_declared = false;
bool Parser::use_architechture_bit = false;

std::unordered_map<std::string, bool> Parser::vars_with_nothing;
std::unordered_map<std::string, bool> Parser::verified_allocs;
std::pair<std::string, std::string> Parser::current_function_in_scope;

bool Parser::dont_share_history = false;

int Parser::random_global_id = 0;

void CompileToLLVMIR()
{
	std::ifstream t("main.nk");
	std::string str((std::istreambuf_iterator<char>(t)),
  	             std::istreambuf_iterator<char>());

	CodeGen::Initialize();

	Lexer::AddContent(str);

	Lexer::Start();

	Parser::MainLoop();
}

int main(int argc, char const *argv[])
{
	if (argc > 1)
	{
		std::string cmd = argv[1];
		if (cmd == "hello")
		{
			std::cout << "Hi! :D\n";
		}
		else if (cmd == "emit")
		{
			if(argc > 2) {
				std::string cmd_two = argv[2];

				if(cmd_two == "release") {
					CodeGen::is_release = true;
				}
			}
			
			CompileToLLVMIR();
			CodeGen::Print();
			TodoList::print();
		}
		else if (cmd == "build")
		{
			if(argc > 2) {
				std::string cmd_two = argv[2];

				if(cmd_two == "release") {
					CodeGen::is_release = true;
				}
			}

			CompileToLLVMIR();
			TodoList::print();
			CodeGen::Build();
		}
		else if (cmd == "run")
		{
			CompileToLLVMIR();
			TodoList::print();
			CodeGen::Run();
		}
		else if (cmd == "create")
		{
			if (argc > 2)
			{
				Project::Create(argv[2]);
			}
			else { std::cout << "Error: Name not provided!\n"; }
		}
	}

	return 0;
}
#include <iostream>
#include "Language/Lexer.hpp"
#include "Language/Parser.hpp"
#include "Language/AST.hpp"
#include "Language/CodeGen.hpp"
#include "Tooling/Project.hpp"
#include <fstream>

std::string Parser::last_identifier;
std::string Parser::last_target;
bool Parser::grab_target = true;
std::unordered_map<std::string, std::string> Parser::all_variables;

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
			CompileToLLVMIR();
			CodeGen::Print();
		}
		else if (cmd == "build")
		{
			CompileToLLVMIR();
			CodeGen::Build();
		}
		else if (cmd == "run")
		{
			CompileToLLVMIR();
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
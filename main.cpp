#include <iostream>
#include "Language/Lexer.hpp"
#include "Language/Parser.hpp"
#include "Language/AST.hpp"
#include "Language/CodeGen.hpp"
#include <fstream>

int main(int argc, char const *argv[])
{
	if(argc > 1)
	{
		std::string cmd = argv[1];
		if(cmd == "hello")
		{
			std::cout << "Hi! :D\n";
		}
	}
	else
	{
		std::ifstream t("main.nk");
		std::string str((std::istreambuf_iterator<char>(t)),
    		             std::istreambuf_iterator<char>());

		CodeGen::Initialize();

		Lexer::AddContent(str);

		Lexer::Start();

		Parser::MainLoop();
	}
	
	return 0;
}
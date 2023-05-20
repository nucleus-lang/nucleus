#include "CodeGen.hpp"
#include "AST.hpp"

#include <filesystem>

#ifdef _WIN32
	#include <Windows.h>
#endif

std::unique_ptr<llvm::LLVMContext> CodeGen::TheContext;
std::unique_ptr<llvm::IRBuilder<>> CodeGen::Builder;
std::unique_ptr<llvm::Module> CodeGen::TheModule;

std::map<std::string, llvm::AllocaInst*> CodeGen::NamedValues;
std::map<std::string, std::pair<llvm::LoadInst*, llvm::Value*>> CodeGen::NamedLoads;
std::map<std::string, llvm::Value*> CodeGen::NamedPures;
std::map<std::string, std::pair<llvm::BasicBlock*, llvm::BasicBlock*>> CodeGen::NamedPHILoads;
std::map<std::string, std::pair<llvm::Argument*, llvm::Value*>> CodeGen::NamedArguments;
std::map<std::string, llvm::Value*> CodeGen::CurrentInst;

std::vector<llvm::BasicBlock*> CodeGen::allBasicBlocks;

void CodeGen::push_block_to_list(llvm::BasicBlock* bb) {

	CodeGen::allBasicBlocks.push_back(bb);
}

llvm::BasicBlock* CodeGen::get_last_bb_used_by(llvm::Value* v) {

	for (int i = CodeGen::allBasicBlocks.size() - 1; i > -1; i--)
	{
		if(CodeGen::allBasicBlocks[i] == nullptr) {
			std::cout << i << " is nullptr\n";
			continue;
		}

		if(v->isUsedInBasicBlock(CodeGen::allBasicBlocks[i]))
			return CodeGen::allBasicBlocks[i];
	}

	return nullptr;
}

void CodeGen::Initialize()
{
	// Open a new context and module.
 	TheContext = std::make_unique<llvm::LLVMContext>();

 	TheModule = std::make_unique<llvm::Module>("Nucleus", *TheContext);
 	//TheModule->setDataLayout(TheJIT->getDataLayout());

 	 // Create a new builder for the module.
 	Builder = std::make_unique<llvm::IRBuilder<>>(*TheContext);

 	//TheFPM = std::make_unique<llvm::legacy::FunctionPassManager>(TheModule.get());
}

void CodeGen::Error(std::string str)
{
	std::cout << "Error: " << str << "\n";
	exit(1);
}

void CodeGen::Build()
{
	std::string clangCmd;

	std::error_code EC;
	llvm::raw_fd_ostream dest("output.ll", EC, llvm::sys::fs::OF_None);

	TheModule->print(dest, nullptr);
	clangCmd = "clang++ output.ll -Wno-override-module -o result";

	std::cout << "Compiling...\n";

	system(clangCmd.c_str());
}

void CodeGen::Print()
{	
	TheModule->print(llvm::outs(), nullptr);
}

void CodeGen::Run()
{
	CodeGen::Build();

	#ifdef _WIN32
		// additional information
		STARTUPINFO si;     
		PROCESS_INFORMATION pi;
		
		// set the size of the structures
		ZeroMemory( &si, sizeof(si) );
		si.cb = sizeof(si);
		ZeroMemory( &pi, sizeof(pi) );

		std::string resultPath = std::string(std::filesystem::current_path().string() + "/result.exe");
		
		// start the program up
		auto res = CreateProcess(
			NULL,   // the path
			(LPSTR)resultPath.c_str(),           // Command line
			NULL,           // Process handle not inheritable
			NULL,           // Thread handle not inheritable
			FALSE,          // Set handle inheritance to FALSE
			0,              // No creation flags
			NULL,           // Use parent's environment block
			NULL,           // Use parent's starting directory 
			&si,            // Pointer to STARTUPINFO structure
			&pi             // Pointer to PROCESS_INFORMATION structure (removed extra parentheses)
		);
		// Close process and thread handles. 
		
		if (!res)
		{
			std::cout << "Error: Program can't be executed.\n";
			exit(1);
		}
		else
		{
			unsigned long exitCode;

			WaitForSingleObject(
				pi.hProcess,  
				INFINITE      // time-out interval in milliseconds  
				);  
			GetExitCodeProcess(pi.hProcess, &exitCode);

			std::cout << "Your Program Returned: " << (int)exitCode << "\n";
		}

		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

	#else
		std::cout << "The 'run' command is not supported in your current OS yet.\n";
	#endif
}

llvm::Function* CodeGen::GetFunction(std::string name)
{
	if (auto* F = TheModule->getFunction(name)) { return F; }

	auto FI = AST::FunctionProtos.find(name);
	if (FI != AST::FunctionProtos.end())
	{
		return FI->second->codegen();
	}

	return nullptr;
}
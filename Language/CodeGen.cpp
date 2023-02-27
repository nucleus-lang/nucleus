#include "CodeGen.hpp"
#include "AST.hpp"

#ifdef _WIN32
	#include <Windows.h>
#endif

std::unique_ptr<llvm::LLVMContext> CodeGen::TheContext;
std::unique_ptr<llvm::IRBuilder<>> CodeGen::Builder;
std::unique_ptr<llvm::Module> CodeGen::TheModule;

std::map<std::string, llvm::AllocaInst*> CodeGen::NamedValues;
std::map<std::string, std::pair<llvm::LoadInst*, llvm::Value*>> CodeGen::NamedLoads;
std::map<std::string, llvm::Value*> CodeGen::CurrentInst;

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
	clangCmd = "clang++ output.ll -o result";

	std::cout << "Compiling...\n";

	system(clangCmd.c_str());
}

void CodeGen::Print()
{
	std::cout << "Printing...\n";
	
	TheModule->print(llvm::outs(), nullptr);
}

llvm::Function* CodeGen::GetFunction(std::string name)
{
	if(auto* F = TheModule->getFunction(name)) { return F; }

	auto FI = AST::FunctionProtos.find(name);
	if(FI != AST::FunctionProtos.end())
	{
		return FI->second->codegen();
	}

	return nullptr;
}
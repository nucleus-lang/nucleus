#ifndef CODEGEN_HPP
#define CODEGEN_HPP

// RELEASE THE KRAKEN.

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Transforms/Vectorize.h"

#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/RegionPass.h"
#include "llvm/Analysis/RegionPrinter.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/CodeGen/Passes.h"

#include "llvm/Transforms/Utils.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/IPO/AlwaysInliner.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/CFG.h"
#include <map>

struct CodeGen
{
	static std::unique_ptr<llvm::LLVMContext> TheContext;
	static std::unique_ptr<llvm::IRBuilder<>> Builder;
	static std::unique_ptr<llvm::Module> TheModule;

	static std::map<std::string, llvm::Value*> NamedValues;
	static std::map<std::string, std::pair<llvm::LoadInst*, llvm::Value*>> NamedLoads;
	static std::map<std::string, llvm::Value*> NamedPures;
	static std::map<std::string, std::pair<llvm::BasicBlock*, llvm::BasicBlock*>> NamedPHILoads;
	static std::map<std::string, std::pair<llvm::Argument*, llvm::Value*>> NamedArguments;

	static std::vector<llvm::BasicBlock*> allBasicBlocks;
	
	static std::map<std::string, llvm::Value*> CurrentInst;

	static void push_block_to_list(llvm::BasicBlock* bb);

	static llvm::BasicBlock* get_last_bb_used_by(llvm::Value* v);

	static void Initialize();

	static void Error(std::string str);

	static void Build();

	static void Print();

	static void Run();

	static llvm::Function* GetFunction(std::string name);
};

#endif
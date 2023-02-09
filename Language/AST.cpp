#include "AST.hpp"
#include "Lexer.hpp"

std::map<std::string, std::unique_ptr<AST::Prototype>> AST::FunctionProtos;
llvm::Value* AST::CurrInst;

llvm::Value* AST::Expression::codegenOnlyLoad()
{
	onlyLoad = true;
	llvm::Value* res = codegen();
	onlyLoad = false;

	return res;
}

void AST::Expression::GetPointer()
{
	_getPointer = true;
}

std::unique_ptr<AST::Expression> AST::ExprError(std::string str)
{
	std::cout << "Error: " << str << "\n";
	std::cout << "========\n";

	if(Lexer::CurrentToken != Token::Identifier)
	{
		std::cout << "Found Token: " << Lexer::CurrentToken << "\n";
	}
	else
	{
		std::cout << "Found Identifier: " << Lexer::IdentifierStr << "\n";
	}
	
	exit(1);
	return nullptr;
}

llvm::Value* AST::Number::codegen()
{
	if(isInt)
	{
		llvm::Type *getType = nullptr;

		if (bit == 1)        getType = llvm::IntegerType::getInt1Ty(*CodeGen::TheContext);
		else if (bit == 8)   getType = llvm::IntegerType::getInt8Ty(*CodeGen::TheContext);
		else if (bit == 16)  getType = llvm::IntegerType::getInt16Ty(*CodeGen::TheContext);
		else if (bit == 32)  getType = llvm::IntegerType::getInt32Ty(*CodeGen::TheContext);
		else if (bit == 64)  getType = llvm::IntegerType::getInt64Ty(*CodeGen::TheContext);
		else if (bit == 128) getType = llvm::IntegerType::getInt128Ty(*CodeGen::TheContext);
		else                 getType = llvm::IntegerType::getInt32Ty(*CodeGen::TheContext);

		return llvm::ConstantInt::get(getType, intValue, false);
	}
	else if(isDouble)
	{
		return llvm::ConstantFP::get(*CodeGen::TheContext, llvm::APFloat(doubleValue));
	}
	else if(isFloat)
	{
		return llvm::ConstantFP::get(*CodeGen::TheContext, llvm::APFloat(floatValue));
	}

	return nullptr;
}

llvm::Value* AST::Return::codegen()
{
	llvm::Value* c = Expr->codegen();
	return CodeGen::Builder->CreateRet(c);
}

llvm::Value* AST::Load::codegen()
{
	llvm::Value* c = Target->codegen();

	llvm::Type* TV = nullptr;

	if(T == nullptr)
	{
		if (llvm::AllocaInst* I = dyn_cast<llvm::AllocaInst>(c))
		{
		     TV = I->getAllocatedType();
		}
	}
	else
	{
		TV = T->codegen();
	}

	if(TV == nullptr)
		CodeGen::Error("Type of Load Instruction was not found.");

	llvm::LoadInst* L = CodeGen::Builder->CreateLoad(TV, c, Name);
	CodeGen::NamedLoads[Name] = L;

	return L;
}

llvm::Type* AST::i32::codegen()
{
	return llvm::Type::getInt32Ty(*CodeGen::TheContext);
}

llvm::Value* AST::Variable::codegen()
{
	llvm::AllocaInst* V = CodeGen::NamedValues[Name];

	bool currentGPState = _getPointer;
	_getPointer = false;

	if (!V) 
	{ 
		llvm::LoadInst* V2 = CodeGen::NamedLoads[Name];

		if(!V2)
			CodeGen::Error("Unknown variable name: " + Name + "\n"); 

		return V2;
	}

	//if(!currentGPState)
	//	return CodeGen::Builder->CreateLoad(V->getAllocatedType(), V, Name.c_str());
	//else
	//	return CodeGen::Builder->CreateLoad(V->getAllocatedType()->getPointerTo(), V, Name.c_str());

	//return nullptr;

	return V;
}

llvm::Value* AST::Alloca::codegen()
{
	std::vector<llvm::AllocaInst*> OldBindings;
	llvm::Function* TheFunction = CodeGen::Builder->GetInsertBlock()->getParent();

	llvm::AllocaInst* Alloca = nullptr;

	Alloca = CodeGen::Builder->CreateAlloca(T->codegen(), 0, VarName.c_str());

	OldBindings.push_back(CodeGen::NamedValues[VarName]);
	CodeGen::NamedValues[VarName] = Alloca;

	//for (unsigned i = 0, e = VarNames.size(); i != e; ++i)
		//CodeGen::NamedValues[VarName] = OldBindings[0];

	return Alloca;
}

llvm::Value* AST::Store::codegen()
{
	Target->GetPointer();
	return CodeGen::Builder->CreateStore(Value->codegen(), Target->codegen());
}

bool IsIntegerType(llvm::Value* V)
{
	if (llvm::AllocaInst* I = dyn_cast<llvm::AllocaInst>(V))
	{
		return I->getAllocatedType()->isIntegerTy();
	}
	else
		return V->getType()->isIntegerTy();
}

llvm::Value* GetInst(AST::Expression* v)
{
	if(AST::CurrInst == nullptr)
	{
		llvm::Value* getV = v->codegen();
		llvm::Type* TV = nullptr;

		if (llvm::LoadInst* I = dyn_cast<llvm::LoadInst>(getV))
		{
			return I;
		}

		if (llvm::AllocaInst* I = dyn_cast<llvm::AllocaInst>(getV))
		{
			TV = I->getAllocatedType();
		}
		else
			TV = getV->getType();

		return CodeGen::Builder->CreateLoad(TV, getV, getV->getName());
	}

	return AST::CurrInst;
}

llvm::Value* AST::Add::codegen()
{
	llvm::Value* L = GetInst(Target.get());
	llvm::Value* R = Value->codegen();

	llvm::Value* Result = nullptr;

	if(IsIntegerType(L) && IsIntegerType(R))
		Result = CodeGen::Builder->CreateAdd(L, R, "addtmp");
	else
		Result = CodeGen::Builder->CreateFAdd(L, R, "addtmp");

	AST::CurrInst = Result;

	return Result;
}

llvm::Value* AST::Sub::codegen()
{
	llvm::Value* L = GetInst(Target.get());
	llvm::Value* R = Value->codegen();

	llvm::Value* Result = nullptr;

	if(IsIntegerType(L) && IsIntegerType(R))
		Result = CodeGen::Builder->CreateSub(L, R, "subtmp");
	else
		Result = CodeGen::Builder->CreateFSub(L, R, "subtmp");

	AST::CurrInst = Result;

	return Result;
}

llvm::Value* AST::Link::codegen()
{
	llvm::Value* L = GetInst(Target.get());

	if(Value == nullptr)
	{
		llvm::Value* Result = CodeGen::Builder->CreateStore(AST::CurrInst, Target->codegen());
		AST::CurrInst = nullptr;

		return Result;
	}

	llvm::Value* R = Value->codegen();

	AST::CurrInst = nullptr;

	return CodeGen::Builder->CreateStore(L, R);
}

llvm::Function* AST::Prototype::codegen()
{
	std::vector<llvm::Type*> llvmArgs;

	for (auto const& i: Args)
	{
		llvmArgs.push_back(i->T->codegen());
	}

	llvm::FunctionType* FT = llvm::FunctionType::get(PType->codegen(), llvmArgs, false);

	llvm::Function* F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, Name, CodeGen::TheModule.get());

	unsigned Idx = 0;
	for (auto &Arg : F->args()) 
	{
		if (Args[Idx] == nullptr)
			CodeGen::Error("One of the Arguments is nullptr.");

  		Arg.setName(Args[Idx++]->Name);
	}

	return F;
}

llvm::Function* AST::Function::codegen()
{
	if (Proto == nullptr)
		CodeGen::Error("Function prototype is nullptr.\n");

	auto &P = *Proto;

	AST::FunctionProtos[Proto->getName()] = std::move(Proto);
	llvm::Function* TheFunction = CodeGen::GetFunction(P.getName());

	llvm::BasicBlock* BB = llvm::BasicBlock::Create(*CodeGen::TheContext, "entry", TheFunction);
	CodeGen::Builder->SetInsertPoint(BB);

	CodeGen::NamedValues.clear();
	CodeGen::NamedLoads.clear();

	for(auto const& i: Body)
	{
		i->codegen();
	}

	return TheFunction;
}
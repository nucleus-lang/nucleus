#include "AST.hpp"
#include "Lexer.hpp"

std::map<std::string, std::unique_ptr<AST::Prototype>> AST::FunctionProtos;
llvm::Value* AST::CurrInst;
std::string AST::CurrentIdentifier;

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

	if (Lexer::CurrentToken != Token::Identifier)
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
	if (isInt)
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
	else if (isDouble)
	{
		return llvm::ConstantFP::get(*CodeGen::TheContext, llvm::APFloat(doubleValue));
	}
	else if (isFloat)
	{
		return llvm::ConstantFP::get(*CodeGen::TheContext, llvm::APFloat(floatValue));
	}

	return nullptr;
}

llvm::Value* AST::Return::codegen()
{
	if (dynamic_cast<AST::Variable*>(Expr.get()))
	{
		auto I = (AST::Load*)Expr.get();
		if (CodeGen::NamedLoads.find(I->Name) == CodeGen::NamedLoads.end())
		{
			llvm::Value* c = Expr->codegen();
			return CodeGen::Builder->CreateRet(c);
		}

		if (CodeGen::NamedLoads[I->Name].second == nullptr)
		{
			llvm::Value* c = Expr->codegen();
			return CodeGen::Builder->CreateRet(c);
		}

		return CodeGen::Builder->CreateRet(CodeGen::NamedLoads[I->Name].second);
	}

	llvm::Value* c = Expr->codegen();
	return CodeGen::Builder->CreateRet(c);
}

llvm::Value* AST::Load::codegen()
{
	llvm::Value* c = Target->codegen();

	llvm::Type* TV = nullptr;

	if (T == nullptr)
	{
		if (llvm::AllocaInst* I = dyn_cast<llvm::AllocaInst>(c))
			 TV = I->getAllocatedType();
		else if (llvm::LoadInst* I = dyn_cast<llvm::LoadInst>(c))
			 TV = I->getPointerOperandType();
		else if (dynamic_cast<AST::Link*>(Target.get()))
		{
			AST::Link* TTarget = (AST::Link*)Target.get();

			if (TTarget->getType != nullptr)
			{
				TV = TTarget->getType;
			}
		}
	}
	else
	{
		TV = T->codegen();
	}

	if (TV == nullptr)
		CodeGen::Error("Type of Load Instruction was not found.");

	if (dynamic_cast<AST::Link*>(Target.get()))
	{
		AST::Link* TTarget = (AST::Link*)Target.get();
		c = TTarget->Target->codegen();
	}

	llvm::LoadInst* L = CodeGen::Builder->CreateLoad(TV, c, Name);

	if (CodeGen::NamedLoads.find(Name) != CodeGen::NamedLoads.end())
	{
		CodeGen::NamedLoads[AST::CurrentIdentifier].first = L;
		CodeGen::NamedLoads[AST::CurrentIdentifier].second = nullptr;
		AST::CurrInst = nullptr;
	}
	else
		CodeGen::NamedLoads[Name] = std::make_pair(L, nullptr);

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
		llvm::LoadInst* V2 = CodeGen::NamedLoads[Name].first;

		if (!V2)
			CodeGen::Error("Unknown variable name: " + Name + "\n"); 

		AST::CurrentIdentifier = Name;

		return V2;
	}

	AST::CurrentIdentifier = Name;

	//if (!currentGPState)
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

llvm::Value* CreateAutoLoad(AST::Expression* v)
{
	llvm::Value* getV = v->codegen();
	llvm::Type* TV = nullptr;

	if (llvm::LoadInst* I = dyn_cast<llvm::LoadInst>(getV))
	{
		//CodeGen::NamedLoads[AST::CurrentIdentifier] = std::make_pair(I, getV);
		return I;
	}

	if (llvm::AllocaInst* I = dyn_cast<llvm::AllocaInst>(getV))
	{
		TV = I->getAllocatedType();
	}
	else
		TV = getV->getType();

	auto L = CodeGen::Builder->CreateLoad(TV, getV, getV->getName());
	CodeGen::NamedLoads[AST::CurrentIdentifier] = std::make_pair(L, nullptr);

	return L;
}

llvm::Value* GetInst(AST::Expression* v)
{
	llvm::Value* r = v->codegen();

	if (r == nullptr)
		CodeGen::Error("r is nullptr");

	if (dynamic_cast<AST::Number*>(v) != nullptr)
		return r;

	if (CodeGen::NamedLoads.find(AST::CurrentIdentifier) != CodeGen::NamedLoads.end())
	{
		if (CodeGen::NamedLoads[AST::CurrentIdentifier].second != nullptr)
		{
			return CodeGen::NamedLoads[AST::CurrentIdentifier].second;
		}
		else
			return CreateAutoLoad(v);
	}
	else
	{
		return CreateAutoLoad(v);
	}

	return nullptr;
}

void AddInst(std::string target_name, llvm::Value* r)
{	
	if (CodeGen::NamedLoads.find(target_name) != CodeGen::NamedLoads.end())
	{
		CodeGen::NamedLoads[target_name].second = r;
	}
	else
	{
		CodeGen::Error(AST::CurrentIdentifier + " not found in AddInst()");
	}
}

llvm::Value* AST::Add::codegen()
{
	llvm::Value* L = GetInst(Target.get());
	std::string target_name = AST::CurrentIdentifier;

	llvm::Value* R = GetInst(Value.get());

	llvm::Value* Result = nullptr;

	if (IsIntegerType(L) && IsIntegerType(R)) Result = CodeGen::Builder->CreateAdd(L, R, "addtmp");
	else Result = CodeGen::Builder->CreateFAdd(L, R, "addtmp");

	AddInst(target_name, Result);
	AST::CurrInst = Result;

	return Result;
}

llvm::Value* AST::Sub::codegen()
{
	llvm::Value* L = GetInst(Target.get());
	std::string target_name = AST::CurrentIdentifier;

	llvm::Value* R = GetInst(Value.get());

	llvm::Value* Result = nullptr;

	if (IsIntegerType(L) && IsIntegerType(R)) Result = CodeGen::Builder->CreateSub(L, R, "subtmp");
	else Result = CodeGen::Builder->CreateFSub(L, R, "subtmp");

	AddInst(target_name, Result);
	AST::CurrInst = Result;

	return Result;
}

std::string GetName(AST::Expression* T)
{
	if (dynamic_cast<AST::Variable*>(T))
	{
		AST::Variable* V = dynamic_cast<AST::Variable*>(T);
		return V->Name;
	}

	return "";
}

llvm::Value* AST::Link::codegen()
{
	llvm::Value* L = GetInst(Target.get());

	std::string name = GetName(Target.get());

	if (name == "") CodeGen::Error("Name of Target not found.");

	if (L == nullptr) CodeGen::Error("L is nullptr.");

	getType = L->getType();

	if (getType == nullptr)
		CodeGen::Error("getType is nullptr.");

	if (Value == nullptr)
	{
		if (CodeGen::NamedLoads.find(name) != CodeGen::NamedLoads.end())
		{
			auto T = Target->codegen();

			if (T == nullptr) CodeGen::Error("T is nullptr.");

			if (CodeGen::NamedLoads[name].second == nullptr) CodeGen::Error("Current Identifier " + name + " is nullptr.");

			llvm::Value* Result = CodeGen::Builder->CreateStore(CodeGen::NamedLoads[name].second, T);
			CodeGen::NamedLoads[name].second = nullptr;

			if (Result == nullptr) CodeGen::Error("Return is nullptr");

			return Result;
		}
	}

	std::cout << "Getting \"R\" value...\n";

	llvm::Value* R = Value->codegen();

	if (R == nullptr) CodeGen::Error("R is nullptr.");

	AST::CurrInst = nullptr;

	return CodeGen::Builder->CreateStore(L, R);
}

llvm::Value* AST::VerifyOne::codegen()
{
	llvm::Value* Link = CodeGen::Builder->CreateStore(AST::CurrInst, Target->codegen());

	if (CodeGen::NamedLoads.find(AST::CurrentIdentifier) != CodeGen::NamedLoads.end())
	{
		if (CodeGen::NamedLoads[AST::CurrentIdentifier].second != nullptr) CodeGen::NamedLoads[AST::CurrentIdentifier].second = nullptr;
	}

	AST::CurrentIdentifier = "";
	AST::CurrInst = nullptr;

	return Target->codegen();
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
		if (Args[Idx] == nullptr) CodeGen::Error("One of the Arguments is nullptr.");

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

	for (auto const& i: Body)
	{
		if (i != nullptr)
			i->codegen();
	}

	return TheFunction;
}
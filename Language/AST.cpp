#include "AST.hpp"
#include "Lexer.hpp"
#include "ErrorHandler.hpp"
#include <limits>

llvm::Value* AST::CurrInst;

std::string AST::CurrentIdentifier;
std::map<std::string, std::unique_ptr<AST::Prototype>> AST::FunctionProtos;
AST::Type* AST::current_proto_type = nullptr;

llvm::Value* CreateAutoLoad(AST::Expression* v, llvm::Value* r);
llvm::Value* GetPHI(std::string name, llvm::Value* l, llvm::Value* s);
llvm::Value* BlockCodegen(AST::Expression* i, llvm::BasicBlock* begin, llvm::BasicBlock* current, llvm::Value* inst);
ARGUMENT_LIST() generate_block_codegen(ARGUMENT_LIST() Body, llvm::BasicBlock* EntryBlock, llvm::BasicBlock* LoopBlock);

std::map<std::string, std::unique_ptr<AST::Atom>> AST::Atoms;
std::vector<AST::Atom*> AST::current_atom_line;
bool AST::is_inside_atom = false;

llvm::Value* GetInst(AST::Expression* v, bool enable_phi = true)
{
	llvm::Value* r = v->codegen();

	if (r == nullptr) CodeGen::Error("r is nullptr");

	if (dynamic_cast<AST::Number*>(v) || dynamic_cast<AST::Call*>(v)) return r;

	if (CodeGen::NamedPures.find(AST::CurrentIdentifier) != CodeGen::NamedPures.end())
		if (CodeGen::NamedPures[AST::CurrentIdentifier] != nullptr) return CodeGen::NamedPures[AST::CurrentIdentifier];

	if (CodeGen::NamedArguments.find(AST::CurrentIdentifier) != CodeGen::NamedArguments.end())
	{
		if (CodeGen::NamedArguments[AST::CurrentIdentifier].second != nullptr)
		{
			return CodeGen::NamedArguments[AST::CurrentIdentifier].second;
		}

		return CodeGen::NamedArguments[AST::CurrentIdentifier].first;
	}

	if (CodeGen::NamedLoads.find(AST::CurrentIdentifier) != CodeGen::NamedLoads.end())
	{
		if (CodeGen::NamedLoads[AST::CurrentIdentifier].second != nullptr)
		{
			return CodeGen::NamedLoads[AST::CurrentIdentifier].second;
		}
	}

	return CreateAutoLoad(v, r);
}

llvm::Value* GetCoreInst(AST::Expression* v)
{
	if (CodeGen::NamedPures.find(AST::CurrentIdentifier) != CodeGen::NamedPures.end())
		if (CodeGen::NamedPures[AST::CurrentIdentifier] != nullptr) return CodeGen::NamedPures[AST::CurrentIdentifier];

	if (CodeGen::NamedArguments.find(AST::CurrentIdentifier) != CodeGen::NamedArguments.end())
		return CodeGen::NamedArguments[AST::CurrentIdentifier].first;

	if (CodeGen::NamedLoads.find(AST::CurrentIdentifier) != CodeGen::NamedLoads.end())
		return CodeGen::NamedLoads[AST::CurrentIdentifier].first;

	return v->codegen();
}

void AddInst(std::string target_name, llvm::Value* r);

void AddPHILoad(std::string t, llvm::BasicBlock* b, llvm::BasicBlock* e)
{
	CodeGen::NamedPHILoads[t] = std::make_pair(b, e);
}

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
	std::string found_what = "";

	if (Lexer::CurrentToken == Token::Number) found_what = "Found Number: " + Lexer::NumValString + "\n";
	else if (Lexer::CurrentToken != Token::Identifier) found_what = "Found Token: " + std::to_string(Lexer::CurrentToken) + "\n";
	else found_what = "Found Identifier: " + Lexer::IdentifierStr + "\n";

	std::string final_error = str + " " + found_what;

	ErrorHandler::print(final_error, Lexer::Line, Lexer::Column, Lexer::line_as_string, 0);
	
	exit(1);
	return nullptr;
}

llvm::Value* AST::Number::codegen()
{
	if (isInt)
	{
		llvm::IntegerType *getType = nullptr;

		if (bit == 1)        getType = llvm::IntegerType::getInt1Ty(*CodeGen::TheContext);
		else if (bit == 8)   getType = llvm::IntegerType::getInt8Ty(*CodeGen::TheContext);
		else if (bit == 16)  getType = llvm::IntegerType::getInt16Ty(*CodeGen::TheContext);
		else if (bit == 32)  getType = llvm::IntegerType::getInt32Ty(*CodeGen::TheContext);
		else if (bit == 64)  getType = llvm::IntegerType::getInt64Ty(*CodeGen::TheContext);
		else if (bit == 128) getType = llvm::IntegerType::getInt128Ty(*CodeGen::TheContext);
		else                 getType = llvm::IntegerType::getInt32Ty(*CodeGen::TheContext);

		if(!is_unsigned)	 return llvm::ConstantInt::get(*CodeGen::TheContext, llvm::APInt(getType->getBitWidth(), intValue, false));
		else 				 return llvm::ConstantInt::get(*CodeGen::TheContext, llvm::APInt(getType->getBitWidth(), uintValue, true));
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

llvm::Type* AST::i1::codegen() { return llvm::Type::getInt1Ty(*CodeGen::TheContext); }
llvm::Type* AST::i8::codegen() { return llvm::Type::getInt8Ty(*CodeGen::TheContext); }
llvm::Type* AST::i16::codegen() { return llvm::Type::getInt16Ty(*CodeGen::TheContext); }
llvm::Type* AST::i32::codegen() { return llvm::Type::getInt32Ty(*CodeGen::TheContext); }
llvm::Type* AST::i64::codegen() { return llvm::Type::getInt64Ty(*CodeGen::TheContext); }
llvm::Type* AST::i128::codegen() { return llvm::Type::getInt128Ty(*CodeGen::TheContext); }

llvm::Value* AST::Nothing::codegen()
{
	CodeGen::Error("What is 'Nothing' doing here in the codegen process?... what? ._.");
	return nullptr;
}

llvm::Value* CreateAtomCall(std::string name, ARGUMENT_LIST() Args)
{
	AST::Atoms[name]->RealArgs = std::move(Args);

	AST::current_atom_line.push_back(AST::Atoms[name].get());

	AST::is_inside_atom = true;

	llvm::Value* ret = nullptr;

	for (auto const& i: AST::Atoms[name]->Body)
	{
		if (i != nullptr)
		{	
			auto iC = i->codegen();

			if(dynamic_cast<AST::Return*>(i.get())) 
				ret = iC;
		}
	}

	AST::current_atom_line.pop_back();

	if(AST::current_atom_line.size() == 0)
		AST::is_inside_atom = false;

	if(ret == nullptr) std::cout << "ret is returning nullptr!\n";
	return ret;
}

llvm::Value* AST::Call::codegen()
{
	for (auto const& i: inst_before_args)
	{
		if (i != nullptr)
			i->codegen();
	}

	llvm::Function* CalleeF = CodeGen::TheModule->getFunction(Callee);
	if (!CalleeF) {

		if(AST::Atoms.find(Callee) == AST::Atoms.end()) CodeGen::Error("Unknown function " + Callee + " referenced.\n");

		return CreateAtomCall(Callee, std::move(Args));
	}

	if (CalleeF->arg_size() != Args.size())
		CodeGen::Error("Incorrect # arguments passed.\n");

	std::vector<llvm::Value*> ArgsV;
	for (unsigned i = 0, e = Args.size(); i != e; ++i) {
		llvm::Value* argCG = GetInst(Args[i].get());

		if(argCG == nullptr)
			argCG = Args[i]->codegen();

		if(!argCG) { CodeGen::Error("One of the Arguments in " + Callee + " is nullptr in the codegen.\n"); }

		ArgsV.push_back(argCG);
		if (!ArgsV.back()) CodeGen::Error("The Argument List in " + Callee + " had an internal error in the codegen.\n");
	}

	auto C = CodeGen::Builder->CreateCall(CalleeF, ArgsV, "calltmp");
	C->setCallingConv(CalleeF->getCallingConv());
	C->setTailCall();
	return C;
}

llvm::Value* AST::Return::codegen()
{
	llvm::Value* c = GetInst(Expr.get());
	std::string target_name = AST::CurrentIdentifier;
	c = GetPHI(target_name, GetCoreInst(Expr.get()), c);

	if(!AST::is_inside_atom)
		return CodeGen::Builder->CreateRet(c);

	return c;
}

llvm::Value* AST::Atom::codegen()
{
	for (auto const& i: RealArgs)
	{
		if (i != nullptr)
			GetInst(i.get());
	}

	for (auto const& i: Body)
	{
		if (i != nullptr)
			i->codegen();
	}

	return nullptr;
}

llvm::Value* AST::Load::codegen()
{
	llvm::Value* c = Target->codegen();

	llvm::Type* TV = nullptr;

	if (T == nullptr)
	{
		if (llvm::AllocaInst* I = dyn_cast<llvm::AllocaInst>(c)) TV = I->getAllocatedType();
		else if (llvm::LoadInst* I = dyn_cast<llvm::LoadInst>(c)) TV = I->getPointerOperandType();
		else if (dynamic_cast<AST::Link*>(Target.get()))
		{
			AST::Link* TTarget = (AST::Link*)Target.get();

			if (TTarget->getType != nullptr) TV = TTarget->getType;
		}
	}
	else
	{
		TV = T->codegen();
	}

	if (TV == nullptr) CodeGen::Error("Type of Load Instruction was not found.");

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
	else CodeGen::NamedLoads[Name] = std::make_pair(L, nullptr);

	return L;
}

llvm::Value* GetPHI(std::string name, llvm::Value* l, llvm::Value* s)
{
	if(CodeGen::NamedPHILoads.find(name) != CodeGen::NamedPHILoads.end()) 
	{
		if(CodeGen::NamedPHILoads[name].second != CodeGen::Builder->GetInsertBlock()) 
		{
			auto P = CodeGen::Builder->CreatePHI(l->getType(), 2, "phi");

			if(s == nullptr)
				CodeGen::Error("Second argument not found!");

			P->addIncoming(l, CodeGen::NamedPHILoads[name].first);
			P->addIncoming(s, CodeGen::NamedPHILoads[name].second);

			CodeGen::NamedPHILoads.erase(name);

			return P;
		}
	}

	return s;
}

llvm::Value* AST::Variable::codegen()
{
	if(current_atom_line.size() != 0)
	{
		int Idx = 0;
		int atomIdx = current_atom_line.size() - 1;

		while(atomIdx > -1)
		{
			for(int id = 0; id < current_atom_line[atomIdx]->Args.size(); id++) {

				if(current_atom_line[atomIdx]->Args[id].first == Name) {
					return current_atom_line[atomIdx]->RealArgs[Idx]->codegen();
				}

				Idx++;
			}

			atomIdx--;
		}
	}

	llvm::AllocaInst* V = CodeGen::NamedValues[Name];

	bool currentGPState = _getPointer;
	_getPointer = false;

	if (!V) 
	{ 
		llvm::LoadInst* V2 = CodeGen::NamedLoads[Name].first;

		if (!V2) { 

			llvm::Argument* V3 = CodeGen::NamedArguments[Name].first;

			if(!V3) {

				llvm::Value* V4 = CodeGen::NamedPures[Name];

				if(!V4) CodeGen::Error("Unknown variable name: " + Name + "\n"); 

				AST::CurrentIdentifier = Name;

				return V4;
			}

			//llvm::Value* R3 = GetPHI(Name, V3, CodeGen::NamedArguments[Name].second);

			AST::CurrentIdentifier = Name;

			return V3;
		}

		//llvm::Value* R2 = GetPHI(Name, V2, CodeGen::NamedLoads[Name].second);

		AST::CurrentIdentifier = Name;

		return V2;
	}

	AST::CurrentIdentifier = Name;

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

	// for (unsigned i = 0, e = VarNames.size(); i != e; ++i)
	// 	CodeGen::NamedValues[VarName] = OldBindings[0];

	return Alloca;
}

llvm::Value* AST::Store::codegen()
{
	Target->GetPointer();
	return CodeGen::Builder->CreateStore(GetInst(Value.get()), Target->codegen());
}

bool IsIntegerType(llvm::Value* V)
{
	if (llvm::AllocaInst* I = dyn_cast<llvm::AllocaInst>(V)) return I->getAllocatedType()->isIntegerTy();
	else return V->getType()->isIntegerTy();
}

llvm::Value* CreateAutoLoad(AST::Expression* v, llvm::Value* r)
{
	llvm::Value* getV = r;

	if(dynamic_cast<AST::Number*>(v) || dynamic_cast<AST::Call*>(v)) return getV;

	llvm::Type* TV = nullptr;

	if 		(llvm::LoadInst* I = dyn_cast<llvm::LoadInst>(getV)) return I;
	else if (llvm::Argument* I = dyn_cast<llvm::Argument>(getV)) return I;

	if (llvm::AllocaInst* I = dyn_cast<llvm::AllocaInst>(getV)) TV = I->getAllocatedType();
	else return getV;

	auto L = CodeGen::Builder->CreateLoad(TV, getV, getV->getName());
	CodeGen::NamedLoads[AST::CurrentIdentifier] = std::make_pair(L, nullptr);

	return L;
}

void AddInst(std::string target_name, llvm::Value* r)
{
	if (CodeGen::NamedPures.find(target_name) != CodeGen::NamedPures.end()) { CodeGen::NamedPures[target_name] = r; return; }
	else if (CodeGen::NamedArguments.find(target_name) != CodeGen::NamedArguments.end()) { CodeGen::NamedArguments[target_name].second = r; return; }
	else if (CodeGen::NamedLoads.find(target_name) != CodeGen::NamedLoads.end()) { CodeGen::NamedLoads[target_name].second = r; return; }
	else { CodeGen::Error(AST::CurrentIdentifier + " not found in AddInst()"); }
}

intmax_t get_add_uint_limit(std::string t)
{
	if(t == "u8") return intmax_t(std::numeric_limits<int8_t>::max());
	else if(t == "u16") return intmax_t(std::numeric_limits<int16_t>::max());
	else if(t == "u32") return intmax_t(std::numeric_limits<int32_t>::max());
	else if(t == "u64") return intmax_t(std::numeric_limits<int64_t>::max());
	else if(t == "u128") return intmax_t(std::numeric_limits<intmax_t>::max());

	CodeGen::Error("Type is not unsigned!");
	return 1;
}

intmax_t get_add_uint_limit(AST::Number* final_type)
{
	if(final_type->bit == 8) return get_add_uint_limit("u8");
	else if(final_type->bit == 16) return get_add_uint_limit("u16");
	else if(final_type->bit == 32) return get_add_uint_limit("u32");
	else if(final_type->bit == 64) return get_add_uint_limit("u64");
	else if(final_type->bit == 128) return get_add_uint_limit("u128");

	CodeGen::Error("Type is not unsigned!");
	return 1;
}

intmax_t get_real_integer(AST::Number* t)
{
	if(t->is_unsigned)
		return t->uintValue;

	return t->intValue;
}

llvm::Value* cast_final_right_inst(AST::Number* r, llvm::Value* c)
{
	if(r->is_unsigned)
	{
		auto new_numb = std::make_unique<AST::Number>(std::to_string(r->uintValue), true);
		new_numb->bit = r->bit * 2;

		return new_numb->codegen();
	}

	return c;
}

llvm::Value* final_unsigned_integer_add(AST::Expression* value, llvm::Value* L, llvm::Value* R)
{
	if(dynamic_cast<AST::Number*>(value))
	{
		auto numb_v = dynamic_cast<AST::Number*>(value);
		int get_uint_limit = get_add_uint_limit(numb_v);
		intmax_t limit = get_real_integer(numb_v);

		bool bypassed = false;

		for(int i = 0; i < limit; i++)
		{
			if(i >= get_uint_limit)
			{
				bypassed = true;
				break;
			}
		}

		if(bypassed)
		{
			auto current_type = L->getType();

			auto sext_inst = CodeGen::Builder->CreateSExt(L, llvm::IntegerType::get(*CodeGen::TheContext, numb_v->bit * 2), "sexttmp");
			auto add_inst = CodeGen::Builder->CreateAdd(sext_inst, cast_final_right_inst(numb_v, R), "addtmp");
			auto trunc_inst = CodeGen::Builder->CreateTrunc(add_inst, current_type, "trunctmp");

			return trunc_inst;
		}
	}

	return CodeGen::Builder->CreateAdd(L, R, "addtmp");
}

llvm::Value* AST::Add::codegen()
{
	llvm::Value* L = GetInst(Target.get());
	std::string target_name = AST::CurrentIdentifier;

	llvm::Value* R = GetInst(Value.get());

	llvm::Value* Result = nullptr;

	L = GetPHI(target_name, Target->codegen(), L);

	if (IsIntegerType(L) && IsIntegerType(R))
	{
		if(!Target->is_unsigned)
			Result = CodeGen::Builder->CreateAdd(L, R, "addtmp");
		else
			Result = final_unsigned_integer_add(Value.get(), L, R);

	}
	else Result = CodeGen::Builder->CreateFAdd(L, R, "addtmp");

	if(!dont_share_history)
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

	L = GetPHI(target_name, Target->codegen(), L);

	if (IsIntegerType(L) && IsIntegerType(R)) 
	{
		// if(!Target->is_unsigned)
			Result = CodeGen::Builder->CreateSub(L, R, "subtmp");
		// else
		//	CodeGen::Error("TODO: Add Unsigned Integer System for Sub-tracting...");
	}
	else Result = CodeGen::Builder->CreateFSub(L, R, "subtmp");

	if(!dont_share_history)
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

llvm::Value* AST::Compare::codegen()
{
	llvm::Value* L = GetInst(A.get());
	llvm::Value* R = GetInst(B.get());

	llvm::Value* Result = nullptr;

	if (IsIntegerType(L) && IsIntegerType(R))
	{
		if     (cmp_type == 0) Result = CodeGen::Builder->CreateICmpUGT(R, L, "cmptmp");
		else if(cmp_type == 1) Result = CodeGen::Builder->CreateICmpUGT(L, R, "cmptmp");
		else if(cmp_type == 2) Result = CodeGen::Builder->CreateICmpEQ(L, R, "cmptmp");
		else if(cmp_type == 3) Result = CodeGen::Builder->CreateICmpNE(L, R, "cmptmp");
		else if(cmp_type == 4) Result = CodeGen::Builder->CreateICmpUGE(R, L, "cmptmp");
		else if(cmp_type == 5) Result = CodeGen::Builder->CreateICmpUGE(L, R, "cmptmp");

		return CodeGen::Builder->CreateIntCast(Result, llvm::Type::getInt1Ty(*CodeGen::TheContext), false, "booltmp");
	}

	CodeGen::Error("A Compare CodeGen returned nullptr.");
	return nullptr;
}

std::map<std::string, llvm::Value*> get_entry_values(std::string m = "") {

	std::map<std::string, llvm::Value*> r;

	MAP_FOREACH(std::string, llvm::Value*, CodeGen::NamedPures, it) {
		if(it->second != nullptr) { r[it->first] = it->second; }
	}

	MAP_FOREACH_PAIR(std::string, llvm::Argument*, llvm::Value*, CodeGen::NamedArguments, it2) {
		if(it2->second.second != nullptr) { r[it2->first] = it2->second.second; }
	}

	MAP_FOREACH_PAIR(std::string, llvm::LoadInst*, llvm::Value*, CodeGen::NamedLoads, it3) {
		if(it3->second.second != nullptr) { r[it3->first] = it3->second.second; }
	}

	//std::cout << "Size NamedPures: " << CodeGen::NamedPures.size() << "\n";
	//std::cout << "Size NamedArguments: " << CodeGen::NamedArguments.size() << "\n";
	//std::cout << "Size NamedLoads: " << CodeGen::NamedLoads.size() << "\n";

	return r;
}

void set_entry_values(std::map<std::string, llvm::Value*> r) {

	MAP_FOREACH(std::string, llvm::Value*, r, it) {
		
		if(CodeGen::NamedPures.find(it->first) != CodeGen::NamedPures.end())
			CodeGen::NamedPures[it->first] = it->second;

		else if(CodeGen::NamedArguments.find(it->first) != CodeGen::NamedArguments.end())
			CodeGen::NamedArguments[it->first].second = it->second;

		else if(CodeGen::NamedLoads.find(it->first) != CodeGen::NamedLoads.end())
			CodeGen::NamedLoads[it->first].second = it->second;
	}
}

llvm::Value* generate_individual_ifelse_phi(
	llvm::Value* core, llvm::Value* l, llvm::Value* r,
	llvm::BasicBlock* FirstBB, llvm::BasicBlock* SecondBB) {

	if(core == r)
		return r;

	auto phi = CodeGen::Builder->CreatePHI(core->getType(), 2, "phi");

	if(!l) std::cout << "l is nullptr\n";
	if(!r) std::cout << "r is nullptr\n";

	llvm::Function *TheFunction = CodeGen::Builder->GetInsertBlock()->getParent();

	std::vector<llvm::BasicBlock*> blockPreds;

	llvm::BasicBlock* B = CodeGen::Builder->GetInsertBlock();
	for (auto it = llvm::pred_begin(B), et = llvm::pred_end(B); it != et; ++it)
	{
		llvm::BasicBlock* predecessor = *it;

		blockPreds.push_back(predecessor);
	}

	llvm::BasicBlock* leftBB = nullptr;
	llvm::BasicBlock* rightBB = nullptr;
	llvm::BasicBlock* coreBB = nullptr;

	if(blockPreds.size() == 2) {
		leftBB = blockPreds[0];
		rightBB = blockPreds[1];
		coreBB = blockPreds[1];
	}
	else 
	{
		if(dyn_cast<llvm::Instruction>(l)) {
			auto e = dyn_cast<llvm::Instruction>(l);
			leftBB = e->getParent();
		}
		else { leftBB = FirstBB; }

		if(dyn_cast<llvm::Instruction>(r)) {
			auto e = dyn_cast<llvm::Instruction>(r);
			rightBB = e->getParent();
		}
		else { rightBB = SecondBB; }

		if(dyn_cast<llvm::Instruction>(core)) {
			auto e = dyn_cast<llvm::Instruction>(core);
			coreBB = e->getParent();
		}
		else { coreBB = SecondBB; }
	}

	phi->addIncoming(l, leftBB);

	if(l != r)
		phi->addIncoming(r, rightBB);
	else
		phi->addIncoming(core, coreBB);

	return phi;
}

void ifelse_set_phis(
	std::map<std::string, llvm::Value*> l,
	std::map<std::string, llvm::Value*> r,
	llvm::BasicBlock* FirstBB, llvm::BasicBlock* SecondBB) {

	MAP_FOREACH(std::string, llvm::Value*, l, it) {

		if(CodeGen::NamedPures.find(it->first) != CodeGen::NamedPures.end()) {
			auto p = generate_individual_ifelse_phi(l[it->first], r[it->first], CodeGen::NamedPures[it->first], FirstBB, SecondBB);
			CodeGen::NamedPures[it->first] = p;
		}

		else if(CodeGen::NamedArguments.find(it->first) != CodeGen::NamedArguments.end()) {
			auto p = generate_individual_ifelse_phi(l[it->first], r[it->first], CodeGen::NamedArguments[it->first].second, FirstBB, SecondBB);
			CodeGen::NamedArguments[it->first].second = p;
		}

		else if(CodeGen::NamedLoads.find(it->first) != CodeGen::NamedLoads.end()) {
			auto p = generate_individual_ifelse_phi(l[it->first], r[it->first], CodeGen::NamedLoads[it->first].second, FirstBB, SecondBB);
			CodeGen::NamedLoads[it->first].second = p;
		}
	}
}

llvm::Value* AST::If::codegen()
{
	//std::cout << "generating if...\n";

	std::map<std::string, llvm::Value*> IfEntryValues = get_entry_values("if");

	llvm::Value* ConditionV = Condition->codegen();
	if(ConditionV == nullptr) CodeGen::Error("Condition caught an internal error in If CodeGen.");

	llvm::Function *TheFunction = CodeGen::Builder->GetInsertBlock()->getParent();

	llvm::BasicBlock* EntryBlock = CodeGen::Builder->GetInsertBlock();

	llvm::BasicBlock* IfBlock = 		llvm::BasicBlock::Create(*CodeGen::TheContext, "if", TheFunction);
	llvm::BasicBlock* ElseBlock = 		nullptr;
	llvm::BasicBlock* ContinueBlock = 	llvm::BasicBlock::Create(*CodeGen::TheContext, "continue");

	//CodeGen::push_block_to_list(IfBlock);
	//CodeGen::push_block_to_list(ContinueBlock);

	bool push_continue_block = true;

	if(ElseBody.size() != 0)
	{
		ElseBlock = llvm::BasicBlock::Create(*CodeGen::TheContext, "else");

		//CodeGen::push_block_to_list(ElseBlock);

		CodeGen::Builder->CreateCondBr(ConditionV, IfBlock, ElseBlock);
		push_continue_block = false;
	}
	else CodeGen::Builder->CreateCondBr(ConditionV, IfBlock, ContinueBlock);

	CodeGen::Builder->SetInsertPoint(IfBlock);

	for(auto const& i: IfBody)
		i->codegen();

	std::map<std::string, llvm::Value*> ElseEntryValues;
	ElseEntryValues = get_entry_values("else");
	
	if(/*!dynamic_cast<AST::Return*>(IfBody[IfBody.size() - 1].get()) &&*/ !uncontinue)
	{
		push_continue_block = true;
		CodeGen::Builder->CreateBr(ContinueBlock);
	}
	else { push_continue_block = false; }

	if(ElseBody.size() != 0)
	{
		set_entry_values(IfEntryValues);

		TheFunction->getBasicBlockList().push_back(ElseBlock);
		CodeGen::Builder->SetInsertPoint(ElseBlock);

		for(auto const& i: ElseBody)
			i->codegen();

		if(/*!dynamic_cast<AST::Return*>(ElseBody[ElseBody.size() - 1].get()) &&*/ !uncontinue)
		{
			push_continue_block = true;
			CodeGen::Builder->CreateBr(ContinueBlock);
		}
		else { push_continue_block = false; }
	}
	
	if(/*push_continue_block &&*/ !uncontinue)
	{
		TheFunction->getBasicBlockList().push_back(ContinueBlock);
		CodeGen::Builder->SetInsertPoint(ContinueBlock);
	}

	llvm::BasicBlock* curr;
	
	if(ElseBlock != nullptr) curr = ElseBlock;
	else curr = EntryBlock;
	
	ifelse_set_phis(IfEntryValues, ElseEntryValues, IfBlock, curr);

	return nullptr;
}

llvm::Value* BlockCodegen(AST::Expression* i, llvm::BasicBlock* begin, llvm::BasicBlock* current, llvm::Value* inst)
{
	if(AST::Add* a = dynamic_cast<AST::Add*>(i))
	{
		//if(!dynamic_cast<AST::Load*>(a->Target.get()))
		//	return i->codegen();

		auto T = GetInst(a->Target.get(), false)->getType();
		std::string target_name = AST::CurrentIdentifier;

		auto phi = CodeGen::Builder->CreatePHI(T, 2, "phi");

		llvm::Value* c = CodeGen::Builder->CreateAdd(phi, GetInst(a->Value.get(), false), "addtmp");

		inst = c;

		if(llvm::Instruction* llvmC = dyn_cast<llvm::Instruction>(c))
			llvmC->eraseFromParent();

		phi->addIncoming(c, current);
		phi->addIncoming(a->Target->codegen(), begin);

		AddInst(target_name, c);
		AST::CurrInst = c;

		AddPHILoad(target_name, begin, current);

		return phi;
	}

	if(AST::Sub* a = dynamic_cast<AST::Sub*>(i))
	{
		//if(!dynamic_cast<AST::Load*>(a->Target.get()))
		//	return i->codegen();

		auto T = GetInst(a->Target.get(), false)->getType();
		std::string target_name = AST::CurrentIdentifier;

		auto phi = CodeGen::Builder->CreatePHI(T, 2, "phi");

		llvm::Value* c = CodeGen::Builder->CreateSub(phi, GetInst(a->Value.get(), false), "subtmp");

		inst = c;

		llvm::Instruction* llvmC = dyn_cast<llvm::Instruction>(c);
		if(llvmC) { llvmC->eraseFromParent(); }

		phi->addIncoming(c, current);
		phi->addIncoming(a->Target->codegen(), begin);

		AddInst(target_name, c);
		AST::CurrInst = c;

		AddPHILoad(target_name, begin, current);

		return phi;
	}

	return i->codegen();
}

std::pair<NucleusPHI, AST::Expression*> GetLoopPHI(AST::Expression* i, llvm::BasicBlock* begin, llvm::BasicBlock* current)
{
	NucleusPHI p;

	AST::Expression* target;

	if(!dynamic_cast<AST::Add*>(i) && !dynamic_cast<AST::Sub*>(i))
		return std::make_pair(p, nullptr);

	std::string target_name;

	llvm::Type* T;

	if(AST::Add* a = dynamic_cast<AST::Add*>(i))
	{
		target = a->Target.get();

		T = GetInst(a->Target.get(), false)->getType();
		target_name = AST::CurrentIdentifier;
	}
	else if(AST::Sub* s = dynamic_cast<AST::Sub*>(i))
	{
		target = s->Target.get();

		T = GetInst(s->Target.get(), false)->getType();
		target_name = AST::CurrentIdentifier;
	}

	auto phi = CodeGen::Builder->CreatePHI(T, 2, "phi");

	AddPHILoad(target_name, begin, current);

	p.phi = phi;
	p.begin = begin;
	p.current = current;
	p.target_name = target_name;
	p.target = target;

	return std::make_pair(p, i);
}

ARGUMENT_LIST() generate_block_codegen(ARGUMENT_LIST() Body, llvm::BasicBlock* EntryBlock, llvm::BasicBlock* LoopBlock) {

	std::map<int, std::pair<NucleusPHI, AST::Expression*>> phiRelated;
	std::map<int, AST::Expression*> instructions;

	int count = 0;
	for(auto const& i: Body)
	{
		auto P = GetLoopPHI(i.get(), EntryBlock, LoopBlock);

		if(P.second != nullptr) phiRelated[count] = P;
		else instructions[count] = i.get();

		count++;
	}

	int total = phiRelated.size() + instructions.size();

	for(int i = 0; i < total; i++)
	{
		if(phiRelated.find(i) != phiRelated.end())
		{
			AddInst(phiRelated[i].first.target_name, phiRelated[i].first.phi);

			auto c = phiRelated[i].second->codegen();

			phiRelated[i].first.phi->addIncoming(c, phiRelated[i].first.current);
			phiRelated[i].first.phi->addIncoming(phiRelated[i].first.target->codegen(), phiRelated[i].first.begin);
		}
		else if(instructions.find(i) != instructions.end())
			instructions[i]->codegen();
	}

	return std::move(Body);
}

llvm::Value* AST::Loop::codegen()
{
	llvm::Value* ConditionV = Condition->codegen();
	if(ConditionV == nullptr) CodeGen::Error("Condition caught an internal error in If CodeGen.");

	llvm::Function *TheFunction = CodeGen::Builder->GetInsertBlock()->getParent();

	llvm::BasicBlock* EntryBlock = 		CodeGen::Builder->GetInsertBlock();
	llvm::BasicBlock* LoopBlock = 		llvm::BasicBlock::Create(*CodeGen::TheContext, LoopName.c_str(), TheFunction);
	llvm::BasicBlock* ContinueBlock = 	llvm::BasicBlock::Create(*CodeGen::TheContext, "continue");

	//CodeGen::push_block_to_list(LoopBlock);
	//CodeGen::push_block_to_list(ContinueBlock);

	CodeGen::Builder->CreateCondBr(ConditionV, LoopBlock, ContinueBlock);

	CodeGen::Builder->SetInsertPoint(LoopBlock);

	Body = generate_block_codegen(std::move(Body), EntryBlock, LoopBlock);

	llvm::Value* ConditionV2 = Condition->codegen();

	CodeGen::Builder->CreateCondBr(ConditionV2, LoopBlock, ContinueBlock);

	TheFunction->getBasicBlockList().push_back(ContinueBlock);
	CodeGen::Builder->SetInsertPoint(ContinueBlock);

	return nullptr;
}

llvm::Value* AST::Pure::codegen()
{
	auto R = Inst->codegen();
	CodeGen::NamedPures[Name] = R;

	return R;
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

		Arg.setName(Args[Idx]->Name);

		CodeGen::NamedArguments[Args[Idx]->Name] = std::make_pair(&Arg, nullptr);

		Idx++;
	}

	return F;
}

llvm::Function* AST::Function::apply_attributes(llvm::Function* f) {

	//if(!attributes.can_return_null) 				{ f->addRetAttr(llvm::Attribute::NonNull); }
	if(!attributes.has_to_free_memory) 				{ f->addFnAttr(llvm::Attribute::NoFree); }
	if(attributes.will_return)						{ f->addFnAttr(llvm::Attribute::WillReturn); }
	if(!attributes.prints_exceptions_at_runtime)	{ f->addFnAttr(llvm::Attribute::NoUnwind); }
	if(attributes.must_progress)					{ f->addFnAttr(llvm::Attribute::MustProgress); }

	if(attributes.is_fast)							{ f->setCallingConv(llvm::CallingConv::Tail); 

													  // TODO: Investigate about the Glasgow Haskell
													  //	   Convention. Possible Potential.
													  //f->setCallingConv(llvm::CallingConv::GHC); 
													}

	return f;
}

llvm::Function* AST::Function::codegen()
{

	CodeGen::NamedValues.clear();
	CodeGen::NamedArguments.clear();
	CodeGen::NamedPHILoads.clear();
	CodeGen::NamedLoads.clear();
	CodeGen::NamedPures.clear();

	CodeGen::allBasicBlocks.clear();

	if (Proto == nullptr)
		CodeGen::Error("Function prototype is nullptr.\n");

	auto &P = *Proto;

	// Save this one for later...
	//AST::current_proto_type = Proto.PType.get();

	AST::FunctionProtos[Proto->getName()] = std::move(Proto);
	llvm::Function* TheFunction = CodeGen::GetFunction(P.getName());

	llvm::BasicBlock* BB = llvm::BasicBlock::Create(*CodeGen::TheContext, "entry", TheFunction);
	//CodeGen::push_block_to_list(BB);

	CodeGen::Builder->SetInsertPoint(BB);

	for (auto const& i: Body)
	{
		if (i != nullptr)
			i->codegen();
	}

	TheFunction = apply_attributes(TheFunction);

	return TheFunction;
}
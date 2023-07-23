#include "AST.hpp"
#include "Lexer.hpp"
#include "ErrorHandler.hpp"
#include <limits>

llvm::Value* AST::CurrInst;

std::string AST::CurrentIdentifier;
std::map<std::string, std::unique_ptr<AST::Prototype>> AST::FunctionProtos;
AST::Type* AST::current_proto_type = nullptr;

llvm::Function* AST::exit_declare;

llvm::Value* CreateAutoLoad(AST::Expression* v, llvm::Value* r);
llvm::Value* GetPHI(std::string name, llvm::Value* l, llvm::Value* s);
ARGUMENT_LIST() generate_block_codegen(ARGUMENT_LIST() Body, llvm::BasicBlock* EntryBlock, llvm::BasicBlock* LoopBlock);

std::map<std::string, std::unique_ptr<AST::Atom>> AST::Atoms;
std::vector<AST::Atom*> AST::current_atom_line;
bool AST::is_inside_atom = false;

std::unordered_map<std::string, AST::Type*> all_array_ptrs;

llvm::Value* GetInst(AST::Expression* v, bool enable_phi = true)
{
	llvm::Value* r = v->codegen();

	if (r == nullptr) CodeGen::Error("r is nullptr");

	if (dynamic_cast<AST::Number*>(v) || dynamic_cast<AST::Call*>(v) || dynamic_cast<AST::Data*>(v)) return r;

	if (CodeGen::NamedPures.find(AST::CurrentIdentifier) != CodeGen::NamedPures.end())
	{
		if (CodeGen::NamedPures[AST::CurrentIdentifier].second != nullptr)
		{
			return CodeGen::NamedPures[AST::CurrentIdentifier].second;
		}
	}

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

llvm::Value* GetCoreInst(AST::Expression* v, std::string name)
{
	if (CodeGen::NamedPures.find(name) != CodeGen::NamedPures.end())
		if (CodeGen::NamedPures[name].first != nullptr) return CodeGen::NamedPures[name].first;

	if (CodeGen::NamedArguments.find(name) != CodeGen::NamedArguments.end())
		return CodeGen::NamedArguments[name].first;

	if (CodeGen::NamedLoads.find(name) != CodeGen::NamedLoads.end())
		return CodeGen::NamedLoads[name].first;

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

llvm::Type* AST::Void::codegen() { return llvm::Type::getVoidTy(*CodeGen::TheContext); }

llvm::Type* AST::Array::codegen() { 

	if(is_in_prototype) {
		if(amount == 0) {
			return llvm::PointerType::getUnqual(childType->codegen());
		}
	}

	if(is_pointer) { return llvm::PointerType::getUnqual(childType->codegen()); }

	return llvm::ArrayType::get(childType->codegen(), amount); 
}

llvm::Value* AST::Nothing::codegen()
{
	CodeGen::Error("What is 'Nothing' doing here in the codegen process?... what? ._.");
	return nullptr;
}

llvm::Value* AST::Todo::codegen()
{
	CodeGen::Error("What is 'todo' doing here in the codegen process?... what? ._.");
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
	{
		CodeGen::Error("Incorrect # arguments passed in " + Callee + ". Call passed " + std::to_string(Args.size()) + ", function requires " + std::to_string(CalleeF->arg_size()) + ".\n");
	}

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
	c = GetPHI(target_name, GetCoreInst(Expr.get(), target_name), c);

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
		else if (llvm::GetElementPtrInst* I = dyn_cast<llvm::GetElementPtrInst>(c)) TV = I->getResultElementType();
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

llvm::PHINode* CreateNkPHI(llvm::Type* t, unsigned int reserved, std::string name)
{
	auto P = CodeGen::Builder->CreatePHI(t, reserved, name);
	auto B = CodeGen::Builder->GetInsertBlock();

	CodeGen::all_internal_phis.push_back(std::make_pair(P, B));

	return CodeGen::all_internal_phis[CodeGen::all_internal_phis.size() - 1].first;
}

llvm::Value* GetPHI(std::string name, llvm::Value* l, llvm::Value* s)
{	
	if(CodeGen::NamedPHILoads.find(name) != CodeGen::NamedPHILoads.end())
	{
		if(CodeGen::NamedPHILoads[name].second != CodeGen::Builder->GetInsertBlock())
		{
			auto P = CreateNkPHI(l->getType(), 2, "phi");

			if(s == nullptr)
				CodeGen::Error("Second argument not found!");

			llvm::Value* final_s = CodeGen::Builder->CreateIntCast(s, l->getType(), true);

			if(!CodeGen::NamedPHILoads[name].first) { CodeGen::Error("'CodeGen::NamedPHILoads[name].first' is nullptr."); }
			if(!CodeGen::NamedPHILoads[name].second) { CodeGen::Error("'CodeGen::NamedPHILoads[name].second' is nullptr."); }

			P->addIncoming(l, CodeGen::NamedPHILoads[name].first);
			P->addIncoming(final_s, CodeGen::NamedPHILoads[name].second);

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

	bool currentGPState = _getPointer;
	_getPointer = false;

	if (CodeGen::NamedValues.find(Name) == CodeGen::NamedValues.end())
	{
		if (CodeGen::NamedLoads.find(Name) == CodeGen::NamedLoads.end()) {

			if(CodeGen::NamedArguments.find(Name) == CodeGen::NamedArguments.end()) {

				if(CodeGen::NamedPures.find(Name) == CodeGen::NamedPures.end()) {
					CodeGen::Error("Unknown variable name: " + Name + "\n");
				}

				llvm::Value* V4 = CodeGen::NamedPures[Name].second;

				AST::CurrentIdentifier = Name;

				return V4;
			}

			llvm::Argument* V3 = CodeGen::NamedArguments[Name].first;

			AST::CurrentIdentifier = Name;

			return V3;
		}

		llvm::LoadInst* V2 = CodeGen::NamedLoads[Name].first;

		AST::CurrentIdentifier = Name;

		return V2;
	}

	llvm::Value* V = CodeGen::NamedValues[Name];

	AST::CurrentIdentifier = Name;

	return V;
}

llvm::Value* AST::Alloca::codegen()
{
	std::vector<llvm::Value*> OldBindings;
	llvm::Function* TheFunction = CodeGen::Builder->GetInsertBlock()->getParent();

	if(dynamic_cast<AST::Array*>(T.get()) && is_initialized_by_call)
	{
		std::cout << "T is Array and initialized by call!\n";
		T->is_pointer = true;
	}

	llvm::Value* Alloca = nullptr;

	Alloca = CodeGen::Builder->CreateAlloca(T->codegen(), 0, VarName.c_str());

	if(dyn_cast<llvm::PointerType>(T->codegen()) && dynamic_cast<AST::Array*>(T.get()))
	{
		auto A = dynamic_cast<AST::Array*>(T.get());
		all_array_ptrs[VarName] = A->childType.get();
	}

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

	if 		(llvm::AllocaInst* I = dyn_cast<llvm::AllocaInst>(getV)) TV = I->getAllocatedType();
	else if (llvm::GetElementPtrInst* I = dyn_cast<llvm::GetElementPtrInst>(getV)) TV = I->getResultElementType();
	else return getV;

	auto L = CodeGen::Builder->CreateLoad(TV, getV, getV->getName());
	CodeGen::NamedLoads[AST::CurrentIdentifier] = std::make_pair(L, nullptr);

	return L;
}

void AddInst(std::string target_name, llvm::Value* r)
{
	if (CodeGen::NamedPures.find(target_name) != CodeGen::NamedPures.end()) { CodeGen::NamedPures[target_name].second = r; return; }
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

	L = GetPHI(target_name, GetCoreInst(Target.get(), target_name), L);

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

	L = GetPHI(target_name, GetCoreInst(Target.get(), target_name), L);

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

	MAP_FOREACH_PAIR(std::string, llvm::Value*, llvm::Value*, CodeGen::NamedPures, it) {
		if(it->second.second != nullptr) { r[it->first] = it->second.second; }
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
			CodeGen::NamedPures[it->first].second = it->second;

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

	auto phi = CreateNkPHI(core->getType(), 2, "phi");

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
			auto p = generate_individual_ifelse_phi(l[it->first], r[it->first], CodeGen::NamedPures[it->first].second, FirstBB, SecondBB);
			CodeGen::NamedPures[it->first].second = p;
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

void correct_all_internal_phis() {

	//std::cout << "PHI Test: \n";
	for(auto i : CodeGen::all_internal_phis)
	{
		//std::cout << std::string(i.first->getName()) << " >> \n";

		std::vector<llvm::BasicBlock*> blockPreds;

		for (auto it = llvm::pred_begin(i.second), et = llvm::pred_end(i.second); it != et; ++it)
		{
			llvm::BasicBlock* predecessor = *it;
	
			blockPreds.push_back(predecessor);
		}

		if(blockPreds.size() < 2)
			continue;

		i.first->setIncomingBlock(0, blockPreds[0]);
		i.first->setIncomingBlock(1, blockPreds[1]);

		//i.second->print(llvm::outs()); std::cout << "\n";
	}
	//std::cout << "End of PHI Test.................\n";
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

	if(ElseBody.size() != 0)
	{
		ElseBlock = llvm::BasicBlock::Create(*CodeGen::TheContext, "else");
		CodeGen::Builder->CreateCondBr(ConditionV, IfBlock, ElseBlock);
	}
	else CodeGen::Builder->CreateCondBr(ConditionV, IfBlock, ContinueBlock);

	CodeGen::Builder->SetInsertPoint(IfBlock);

	for(auto const& i: IfBody)
		i->codegen();

	CodeGen::Builder->CreateBr(ContinueBlock);

	std::map<std::string, llvm::Value*> ElseEntryValues;
	ElseEntryValues = get_entry_values("else");

	if(ElseBody.size() != 0)
	{
		set_entry_values(IfEntryValues);

		TheFunction->getBasicBlockList().push_back(ElseBlock);
		CodeGen::Builder->SetInsertPoint(ElseBlock);

		for(auto const& i: ElseBody)
			i->codegen();

		CodeGen::Builder->CreateBr(ContinueBlock);
	}

	TheFunction->getBasicBlockList().push_back(ContinueBlock);
	CodeGen::Builder->SetInsertPoint(ContinueBlock);

	llvm::BasicBlock* curr;

	if(ElseBlock != nullptr) curr = ElseBlock;
	else curr = EntryBlock;

	ifelse_set_phis(IfEntryValues, ElseEntryValues, IfBlock, curr);

	correct_all_internal_phis();

	return nullptr;
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
	}
	else if(AST::Sub* s = dynamic_cast<AST::Sub*>(i))
	{
		target = s->Target.get();
		T = GetInst(s->Target.get(), false)->getType();
	}

	target_name = AST::CurrentIdentifier;

	auto phi = CreateNkPHI(T, 2, "phi");

	//AddPHILoad(target_name, begin, current);

	p.phi = phi;
	p.begin = begin;
	p.current = current;
	p.target_name = target_name;
	p.target = target;

	return std::make_pair(p, i);
}

ARGUMENT_LIST() generate_block_codegen(ARGUMENT_LIST() Body, llvm::BasicBlock* EntryBlock, llvm::BasicBlock* LoopBlock) {

	std::vector<std::pair<NucleusPHI, AST::Expression*>> phiRelated;
	std::vector<AST::Expression*> instructions;
	std::vector<std::pair<int, int>> linkTo;

	int count = 0;
	for(auto const& i: Body)
	{
		auto P = GetLoopPHI(i.get(), EntryBlock, LoopBlock);

		if(P.second != nullptr) 
		{
			linkTo.push_back(std::make_pair(instructions.size(), phiRelated.size()));
			phiRelated.push_back(P);
		}

		instructions.push_back(i.get());

		count++;
	}

	for(int i = 0; i < phiRelated.size(); i++)
	{
		auto final_t = GetInst(phiRelated[i].first.target); VERIFY(final_t)

		std::cout << "Added " << std::string(phiRelated[i].first.phi->getName()) << " to " << phiRelated[i].first.target_name << "\n";

		if(phiRelated[i].first.current == nullptr) { CodeGen::Error("'phiRelated[i].first.current' is nullptr."); }
		if(phiRelated[i].first.begin == nullptr) { CodeGen::Error("'phiRelated[i].first.begin' is nullptr."); }

		phiRelated[i].first.phi->addIncoming(final_t, phiRelated[i].first.current);
		phiRelated[i].first.phi->addIncoming(final_t, phiRelated[i].first.begin);

		AddInst(phiRelated[i].first.target_name, phiRelated[i].first.phi);
	}

	for(int i = 0; i < instructions.size(); i++)
	{
		bool is_linked = false;
		for(auto l : linkTo)
		{
			if(l.first == i)
			{
				auto c = phiRelated[l.second].second->codegen(); VERIFY(c)
				phiRelated[l.second].first.phi->setIncomingValue(0, c);
				is_linked = true;
			}
		}

		if(!is_linked) {
			instructions[i]->codegen();
		}
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

	correct_all_internal_phis();

	//initialize_all_after_phis();

	return nullptr;
}

llvm::Value* AST::Pure::codegen()
{
	auto R = Inst->codegen();
	//AST::CurrentIdentifier = Name;
	
	CodeGen::NamedPures[Name] = std::make_pair(R, R);

	return R;
}

std::unique_ptr<AST::Expression> apply_get_element_safety_checks(std::unique_ptr<AST::Expression> number, std::unique_ptr<AST::Expression> limit, std::string GetElementName)
{
	auto compare_v = std::make_unique<AST::Compare>(std::move(number), std::move(limit), 1);

	llvm::Value* ConditionV = compare_v->codegen();

	std::string bool_result;
	llvm::raw_string_ostream rslt(bool_result);
	ConditionV->print(rslt);

	if(bool_result == "i1 true")
	{
		std::cout << GetElementName << "'s length is higher than the limit.\n";
		std::cout << "TODO: Link Parser Error System with the CodeGen.\n";
		exit(1);
	}

	ARGUMENT_LIST() IfBody;
	ARGUMENT_LIST() ElseBody;

	auto exit_v = std::make_unique<AST::Number>("1");
	exit_v->bit = 32;

	auto exit_call = std::make_unique<AST::Exit>(std::move(exit_v));

	IfBody.push_back(std::move(exit_call));

	auto if_b = std::make_unique<AST::If>(std::move(compare_v), std::move(IfBody), std::move(ElseBody));

	if_b->codegen();

	number = std::move(if_b->Condition);

	return std::move(number);
}

llvm::Value* AST::GetElement::codegen()
{
	std::cout << "GetElement Current Identifier: " << AST::CurrentIdentifier << "\n";

	llvm::Value* number_codegen = GetInst(number.get());

	std::cout << "number_codegen: ";
	number_codegen->print(llvm::outs());
	std::cout << "\n";

	llvm::Value* target_codegen = GetInst(target.get());

	llvm::Value* final_number_result = nullptr;

	if(dynamic_cast<AST::Number*>(number.get())) {
		auto T = dynamic_cast<AST::Number*>(number.get());

		auto newT = std::make_unique<AST::Number>(std::to_string(T->intValue));
		newT->bit = 32;
		final_number_result = newT->codegen();
	}
	else { final_number_result = number_codegen; }

	llvm::Value* indexList[2] = {llvm::ConstantInt::get(final_number_result->getType(), 0), final_number_result};

	llvm::ArrayType* t = dyn_cast<llvm::ArrayType>(target_codegen->getType());
	llvm::PointerType* pT = dyn_cast<llvm::PointerType>(target_codegen->getType());

	if(t)
	{
		uint64_t number_of_elements = t->getNumElements() - 1;

		auto limit_v = std::make_unique<Number>(std::to_string(number_of_elements));
		limit_v->bit = 32;

		number = apply_get_element_safety_checks(std::move(number), std::move(limit_v), GetElementName);
	}
	else if(pT)
	{
		if(!dynamic_cast<AST::Variable*>(target.get()))
			CodeGen::Error("Invalid 'get_element()' target.");

		auto V = dynamic_cast<AST::Variable*>(target.get());

		auto find_il = std::make_unique<AST::Variable>(std::make_unique<AST::i32>(), V->Name + "_length");

		std::string getName = V->Name;

		AST::Type* T = nullptr;

		if(all_array_ptrs.find(getName) == all_array_ptrs.end())
			CodeGen::Error("'T' is not found or is nullptr.");

		T = all_array_ptrs[getName];

		number = apply_get_element_safety_checks(std::move(number), std::move(find_il), GetElementName);

		llvm::Value* indexListPtr[1] = {final_number_result};

		llvm::Value* RPtr = CodeGen::Builder->CreateInBoundsGEP(T->codegen(), target->codegen(), llvm::ArrayRef<llvm::Value*>(indexListPtr, 1), GetElementName);
		if(set_var) { CodeGen::NamedValues[GetElementName] = RPtr; }
		return RPtr;
	}

	llvm::Value* R = CodeGen::Builder->CreateGEP(target_codegen->getType(), target->codegen(), llvm::ArrayRef<llvm::Value*>(indexList, 2), GetElementName);

	if(set_var) { CodeGen::NamedValues[GetElementName] = R; }

	return R;
}

llvm::Value* AST::NewArray::codegen()
{
	if(!is_data)
	{
		if(target == nullptr) { CodeGen::Error("NewArray Target not found."); }

		llvm::Type* t_type = nullptr;

		auto TPtr = dynamic_cast<AST::Alloca*>(target.get()); VERIFY(TPtr)

		std::string save_name = TPtr->VarName;

		AST::CurrentIdentifier = save_name;
	
		if(is_resizable)
		{
			auto TPtrType = dynamic_cast<AST::Array*>(TPtr->T.get()); VERIFY(TPtrType)

			auto final_array_type = std::make_unique<AST::Array>(std::move(TPtrType->childType), items.size());

			t_type = final_array_type->codegen(); VERIFY(t_type)
			
			auto new_target = std::make_unique<AST::Alloca>(std::move(final_array_type), save_name);

			target = std::move(new_target);
		}
	
		llvm::Value* target_cg = GetInst(target.get()); VERIFY(target_cg)

		if(t_type == nullptr)
			t_type = target_cg->getType(); VERIFY(t_type)
	
		if(target_cg == nullptr) {
			std::cout << "TODO: Link Parser Error System with the CodeGen.\n";
			CodeGen::Error("NewArray Target's leaf not found.");
		}
	
		llvm::ArrayType* target_type = dyn_cast<llvm::ArrayType>(t_type);
	
		if(target_type == nullptr) {
			std::cout << "TODO: Link Parser Error System with the CodeGen.\n";
			CodeGen::Error("NewArray Target's Type is not an array.");
		}
	
		uint64_t num_elements = target_type->getNumElements();
	
		if(num_elements < items.size())
		{
			std::cout << "TODO: Link Parser Error System with the CodeGen.\n";
			CodeGen::Error("'new_array()' size is higher than the Array.");
		}

		llvm::Value* pointer_operand;

		llvm::LoadInst* load_inst = dyn_cast<llvm::LoadInst>(target_cg); 

		if (load_inst) { pointer_operand = load_inst->getPointerOperand(); }
		//else { pointer_operand = target_cg; }

		int count;
		for(auto const& i: items)
		{
			llvm::Value* i_cg = GetInst(i.get());

			if(i_cg == nullptr)
			{
				CodeGen::Error("i_cg is nullptr.");
			}

			if(i_cg->getType() != target_type->getArrayElementType())
			{
				std::cout << "TODO: Link Parser Error System with the CodeGen.\n";
				CodeGen::Error("Item Type is not the same as Array Type.");
			}

			auto int_type = llvm::Type::getInt32Ty(*CodeGen::TheContext);
			llvm::Value* indexList[2] = {llvm::ConstantInt::get(int_type, 0), llvm::ConstantInt::get(int_type, count)};

			llvm::Value* R = CodeGen::Builder->CreateGEP(target_type, pointer_operand, llvm::ArrayRef<llvm::Value*>(indexList, 2), "getelement");
			llvm::Value* S = CodeGen::Builder->CreateStore(i_cg, R, "arrayinit");

			count++;
		}

		return nullptr;
	}

	std::vector<llvm::Constant*> values;

	llvm::ArrayType* get_t;

	for(auto const& i: items)
	{
		auto c = GetInst(i.get());
		if(!dyn_cast<llvm::Constant>(c))
		{
			CodeGen::Error("One of the items is not constant");
		}

		if(!get_t)
			get_t = llvm::ArrayType::get(c->getType(), items.size());

		values.push_back(dyn_cast<llvm::Constant>(c));
	}

	if(!get_t) { CodeGen::Error("Constant Array Type not initialized."); }

	return llvm::ConstantArray::get(get_t, values);
}

llvm::Value* AST::Exit::codegen()
{
	std::vector<llvm::Value*> ArgsV;
	ArgsV.push_back(numb->codegen());

	return CodeGen::Builder->CreateCall(AST::exit_declare, ArgsV);
}

llvm::Value* AST::IntCast::codegen()
{
	llvm::Value* target_cg = GetInst(target.get());

	return CodeGen::Builder->CreateIntCast(target_cg, convert_to_type->codegen(), !convert_to_type->is_unsigned);
}

llvm::Value* AST::Data::codegen()
{
	initializer->is_data = true;
	auto init = initializer->codegen();

	llvm::Constant* init_c = dyn_cast<llvm::Constant>(init);

	if(!init_c) { CodeGen::Error("Data Initializer is not Constant.\n"); }

	auto globalVariable = new llvm::GlobalVariable(*CodeGen::TheModule, init->getType(), false, llvm::GlobalVariable::ExternalLinkage, init_c, "");

	return globalVariable;
}

llvm::Function* AST::Prototype::codegen()
{
	std::vector<llvm::Type*> llvmArgs;

	for (auto const& i: Args)
	{
		if(dyn_cast<llvm::PointerType>(i->T->codegen()) && dynamic_cast<AST::Array*>(i->T.get()))
		{
			i->T->is_pointer = true;
			auto A = dynamic_cast<AST::Array*>(i->T.get());
			all_array_ptrs[i->Name] = A->childType.get();
		}

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

	if(attributes.calling_convention == "GLASGOW_HASKELL") 		{ f->setCallingConv(llvm::CallingConv::GHC); }
	else if(attributes.calling_convention == "TAIL") 			{ f->setCallingConv(llvm::CallingConv::Tail); }

	return f;
}

bool can_generate_codegen(AST::Expression* i)
{
	return dynamic_cast<AST::Todo*>(i) == nullptr;
}

llvm::Function* AST::Function::codegen()
{

	CodeGen::NamedValues.clear();
	CodeGen::NamedArguments.clear();
	CodeGen::NamedPHILoads.clear();
	CodeGen::NamedLoads.clear();
	CodeGen::NamedPures.clear();

	CodeGen::all_internal_phis.clear();

	CodeGen::allBasicBlocks.clear();

	all_array_ptrs.clear();

	if (Proto == nullptr)
		CodeGen::Error("Function prototype is nullptr.\n");

	auto &P = *Proto;

	// Save this one for later...
	//AST::current_proto_type = Proto.PType.get();

	attributes.calling_convention = P.calling_convention;

	AST::FunctionProtos[Proto->getName()] = std::move(Proto);
	llvm::Function* TheFunction = CodeGen::GetFunction(P.getName());

	llvm::BasicBlock* BB = llvm::BasicBlock::Create(*CodeGen::TheContext, "entry", TheFunction);
	//CodeGen::push_block_to_list(BB);

	CodeGen::Builder->SetInsertPoint(BB);

	int return_count = 0;

	for (auto const& i: Body)
	{
		if (i != nullptr)
		{
			if(can_generate_codegen(i.get()))
				i->codegen();

			if(dynamic_cast<AST::Return*>(i.get()))
				return_count += 1;
		}
	}

	if(return_count == 0)
	{
		if(dynamic_cast<AST::Void*>(P.PType.get()))
			CodeGen::Builder->CreateRetVoid();
		else
			CodeGen::Error("This function doesn't have returns.");
	}

	TheFunction = apply_attributes(TheFunction);

	return TheFunction;
}
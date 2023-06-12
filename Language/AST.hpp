#ifndef AST_HPP
#define AST_HPP

#include <iostream>
#include <memory>
#include <vector>
#include "CodeGen.hpp"

#define NEW_TYPE(x) struct x : public Type { llvm::Type* codegen() override; }

#define MAP_FOREACH(x, y, z, w) for(std::map<x, y>::iterator w = z.begin(); w != z.end(); ++w)
#define MAP_FOREACH_PAIR(x, y1, y2, z, w) for(std::map<x, std::pair<y1, y2>>::iterator w = z.begin(); w != z.end(); ++w)

#define ATOM_ARG_LIST() std::vector<std::pair<std::string, std::unique_ptr<AST::Type>>>
#define ARGUMENT_LIST() std::vector<std::unique_ptr<AST::Expression>>

struct AST
{
	static llvm::Value* CurrInst;
	static std::string CurrentIdentifier;

	struct Type
	{
		virtual ~Type() = default;

		bool is_unsigned = false;

		virtual llvm::Type* codegen() = 0;
	};

	static AST::Type* current_proto_type;

	struct Expression
	{
		virtual ~Expression() = default;

		bool _getPointer = false;

		void GetPointer();

		bool onlyLoad = false;

		bool is_unsigned = true;

		bool uncontinue = false;

		bool dont_share_history = false;

		llvm::Value* codegenOnlyLoad();

		llvm::Value* CurrentInstruction();

		virtual llvm::Value* codegen() = 0;
	};

	NEW_TYPE(i1);
	NEW_TYPE(i8);
	NEW_TYPE(i16);
	NEW_TYPE(i32);
	NEW_TYPE(i64);
	NEW_TYPE(i128);

	struct Array : public Type {

		std::unique_ptr<Type> childType;
		int amount;

		Array(std::unique_ptr<Type> childType, int amount) : childType(std::move(childType)), amount(amount) {}

		llvm::Type* codegen() override;
	};

	static std::unique_ptr<Expression> ExprError(std::string str);

	struct Number : public Expression
	{
		bool isDouble = false, isInt = false, isFloat = false;
		int64_t intValue = 0;
		uint64_t uintValue = 0;
		double doubleValue = 0;
		float floatValue = 0;
		unsigned bit = 32;
		std::string valueAsString;

		Number(std::string val, bool is_uns = false) 
		{
			is_unsigned = is_uns;
			//std::cout << "Number Input: " << val << "\n";
			if (val.find(".") != std::string::npos) {
				isFloat = val.back() == 'f';
				isDouble = !isFloat;

				if (isFloat) floatValue = std::stof(val);
				else doubleValue = std::stod(val);
			} else {
				isInt = true;

				if(!is_unsigned) intValue = std::stoi(val);
				else uintValue = std::stoul(val);
			}

			valueAsString = val;
		}

		int32_t return_i32()
		{
			return intValue;
		}

		llvm::Value* codegen() override;
	};

	struct Call : public Expression
	{
		std::string Callee;
		ARGUMENT_LIST() Args;
		ARGUMENT_LIST() inst_before_args;

		Call(const std::string &Callee,
			  ARGUMENT_LIST() Args, ARGUMENT_LIST() inst_before_args)
		: Callee(Callee), Args(std::move(Args)), inst_before_args(std::move(inst_before_args)) {}

		llvm::Value* codegen() override;
	};

	struct Variable : public Expression
	{
		std::string Name;

		std::unique_ptr<Type> T;

		bool is_argument = false;

		Variable(std::unique_ptr<Type> T, const std::string& Name) : T(std::move(T)), Name(Name) {}

		llvm::Value* codegen() override;
	};

	struct Return : public Expression
	{
		std::unique_ptr<Expression> Expr;

		bool is_atom_return = false;

		Return(std::unique_ptr<Expression> Expr) : Expr(std::move(Expr)) {}

		llvm::Value* codegen() override;
	};

	struct Alloca : public Expression
	{
		std::unique_ptr<Type> T;
		std::string VarName;

		bool noLoad = false;

		Alloca(std::unique_ptr<Type> T, std::string VarName) : T(std::move(T)), VarName(VarName) {}

		llvm::Value* codegen() override;
	};

	struct Store : public Expression
	{
		std::unique_ptr<Expression> Target;
		std::unique_ptr<Expression> Value;

		Store(std::unique_ptr<Expression> Target, std::unique_ptr<Expression> Value) : Target(std::move(Target)), Value(std::move(Value)) {}

		llvm::Value* codegen() override;
	};

	struct Load : public Expression
	{
		std::string Name;
		std::unique_ptr<Type> T;
		std::unique_ptr<Expression> Target;

		Load(std::string Name, std::unique_ptr<Type> T, std::unique_ptr<Expression> Target) : Name(Name), T(std::move(T)), Target(std::move(Target)) {}

		llvm::Value* codegen() override;
	};

	struct Add : public Expression
	{
		std::unique_ptr<Expression> Target;
		std::unique_ptr<Expression> Value;

		Add(std::unique_ptr<Expression> Target, std::unique_ptr<Expression> Value) : Target(std::move(Target)), Value(std::move(Value)) {}

		llvm::Value* codegen() override;
	};

	struct Sub : public Expression
	{
		std::unique_ptr<Expression> Target;
		std::unique_ptr<Expression> Value;

		Sub(std::unique_ptr<Expression> Target, std::unique_ptr<Expression> Value) : Target(std::move(Target)), Value(std::move(Value)) {}

		llvm::Value* codegen() override;
	};

	struct Link : public Expression
	{
		llvm::Type* getType = nullptr;
		std::unique_ptr<Expression> Target;
		std::unique_ptr<Expression> Value;

		Link(std::unique_ptr<Expression> Target, std::unique_ptr<Expression> Value) : Target(std::move(Target)), Value(std::move(Value)) {}

		llvm::Value* codegen() override;
	};

	struct Pure : public Expression
	{
		std::string Name;
		std::unique_ptr<Type> T;
		std::unique_ptr<Expression> Inst;

		Pure(std::string Name, std::unique_ptr<Type> T, std::unique_ptr<Expression> Inst) :
			Name(Name), T(std::move(T)), Inst(std::move(Inst)) {}

		llvm::Value* codegen() override;
	};

	struct VerifyOne : public Expression
	{
		std::unique_ptr<Expression> Target;

		VerifyOne(std::unique_ptr<Expression> Target) : Target(std::move(Target)) {}

		llvm::Value* codegen() override;
	};

	struct Nothing : public Expression
	{
		llvm::Value* codegen() override;
	};

	struct Compare : public Expression
	{
		std::unique_ptr<Expression> A;
		std::unique_ptr<Expression> B;
		int cmp_type;

		Compare(std::unique_ptr<Expression> A, std::unique_ptr<Expression> B, int cmp_type) :
		A(std::move(A)), B(std::move(B)), cmp_type(cmp_type) {}

		llvm::Value* codegen() override;
	};

	struct If : public Expression
	{
		std::unique_ptr<Expression> Condition;
		std::vector<std::unique_ptr<Expression>> IfBody;
		std::vector<std::unique_ptr<Expression>> ElseBody;

		If(std::unique_ptr<Expression> Condition, std::vector<std::unique_ptr<Expression>> IfBody, std::vector<std::unique_ptr<Expression>> ElseBody) :
			Condition(std::move(Condition)), IfBody(std::move(IfBody)), ElseBody(std::move(ElseBody)) {}

		llvm::Value * codegen() override;
	};

	struct Loop : public Expression
	{
		std::string LoopName;
		std::unique_ptr<Expression> Condition;
		ARGUMENT_LIST() Body;

		Loop(std::string LoopName, std::unique_ptr<Expression> Condition, ARGUMENT_LIST() Body) : 
			LoopName(LoopName), Condition(std::move(Condition)), Body(std::move(Body)) {}

		llvm::Value* codegen() override;
	};

	struct Todo : public Expression
	{
		llvm::Value* codegen() override;
	};

	struct GetElement : public Expression
	{
		std::unique_ptr<Expression> target;
		std::unique_ptr<Expression> number;

		std::string GetElementName = "getelement";
		bool set_var = false;

		GetElement(std::unique_ptr<Expression> target, std::unique_ptr<Expression> number) :
			target(std::move(target)), number(std::move(number)) {}

		llvm::Value* codegen() override;
	};

	struct Prototype 
	{
		std::unique_ptr<AST::Type> PType;
		std::string Name;
		std::string TypeAsString;
		std::string calling_convention;
		std::vector<std::unique_ptr<AST::Variable>> Args;
		
		public:
			Prototype(std::unique_ptr<AST::Type> PType, const std::string &Name, std::vector<std::unique_ptr<AST::Variable>> Args, const std::string &type_as_string, std::string calling_convention = "")
		 	 : PType(std::move(PType)), Name(Name), Args(std::move(Args)), TypeAsString(type_as_string), calling_convention(calling_convention) {}
		
		const std::string &getName() const { return Name; }

		static std::unique_ptr<Prototype> Error(std::string str)
		{
			ExprError(str);
			return nullptr;
		}

		llvm::Function* codegen();
	};

	static std::map<std::string, std::unique_ptr<AST::Prototype>> FunctionProtos;

	struct Atom : public Expression
	{
		std::string Name;
		std::unique_ptr<AST::Type> AType;
		ATOM_ARG_LIST() Args;
		ARGUMENT_LIST() Body;

		ARGUMENT_LIST() RealArgs;

		Atom(std::string Name,
			 ATOM_ARG_LIST() Args,
			 std::unique_ptr<AST::Type> AType,
			 ARGUMENT_LIST() Body)
		: Name(Name), Args(std::move(Args)), AType(std::move(AType)), Body(std::move(Body)) {}

		llvm::Value* codegen() override;
	};

	static std::map<std::string, std::unique_ptr<Atom>> Atoms;
	static std::vector<Atom*> current_atom_line;
	static bool is_inside_atom;

	struct FunctionAttributes {

		bool can_return_null = false;
		bool has_to_free_memory = false;
		bool will_return = true;
		bool prints_exceptions_at_runtime = false;
		bool must_progress = true;
		bool is_fast = true;

		std::string calling_convention = "";
	};

	struct Function
	{
		std::unique_ptr<Prototype> Proto;
		std::vector<std::unique_ptr<Expression>> Body;

		FunctionAttributes attributes;

		Function(std::unique_ptr<Prototype> Proto,
			std::vector<std::unique_ptr<Expression>> Body)
		: Proto(std::move(Proto)), Body(std::move(Body)) {}

		llvm::Function* codegen();

		llvm::Function* apply_attributes(llvm::Function* f);
	};
};

struct NucleusPHI
{
	llvm::PHINode* phi;
	llvm::BasicBlock* begin;
	llvm::BasicBlock* current;
	std::string target_name;
	AST::Expression* target;
};

#endif
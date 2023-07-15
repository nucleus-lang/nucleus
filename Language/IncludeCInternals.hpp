#ifndef INCLUDE_C_INTERNALS_HPP
#define INCLUDE_C_INTERNALS_HPP

#include "AST.hpp"

struct IncludeCInternals
{
	static void say()
	{
		std::vector<std::unique_ptr<AST::Variable>> ArgNames;

		ArgNames.push_back(
			std::make_unique<AST::Variable>(
				std::make_unique<AST::Array>(std::make_unique<AST::i8>(), 0),
				"string"
				)
			);

		ArgNames.push_back(
			std::make_unique<AST::Variable>(
				std::make_unique<AST::i32>(),
				"length"
				)
			);

		auto P = std::make_unique<AST::Prototype>(std::make_unique<AST::i32>(), "say", std::move(ArgNames), "i32");

		P->codegen();
		AST::FunctionProtos["say"] = std::move(P);
	}

	static void i8_to_string()
	{
		std::vector<std::unique_ptr<AST::Variable>> ArgNames;

		ArgNames.push_back(
			std::make_unique<AST::Variable>(
				std::make_unique<AST::i8>(),
				"number"
				)
			);

		auto P = std::make_unique<AST::Prototype>(std::make_unique<AST::Array>(std::make_unique<AST::i8>(), 4), "i8_to_string", std::move(ArgNames), "Array<i8, 4>");

		P->codegen();
		AST::FunctionProtos["i8_to_string"] = std::move(P);
	}

	static void say_i8()
	{
		std::vector<std::unique_ptr<AST::Variable>> ArgNames;

		ArgNames.push_back(
			std::make_unique<AST::Variable>(
				std::make_unique<AST::i8>(),
				"number"
				)
			);

		auto P = std::make_unique<AST::Prototype>(std::make_unique<AST::i32>(), "say_i8", std::move(ArgNames), "i32");

		P->codegen();
		AST::FunctionProtos["say_i8"] = std::move(P);
	}

	static void say_i32()
	{
		std::vector<std::unique_ptr<AST::Variable>> ArgNames;

		ArgNames.push_back(
			std::make_unique<AST::Variable>(
				std::make_unique<AST::i32>(),
				"number"
				)
			);

		auto P = std::make_unique<AST::Prototype>(std::make_unique<AST::i32>(), "say_i32", std::move(ArgNames), "i32");

		P->codegen();
		AST::FunctionProtos["say_i32"] = std::move(P);
	}

	static void input()
	{
		std::vector<std::unique_ptr<AST::Variable>> ArgNames;

		ArgNames.push_back(
			std::make_unique<AST::Variable>(
				std::make_unique<AST::Array>(std::make_unique<AST::i8>(), 0),
				"string"
				)
			);

		ArgNames.push_back(
			std::make_unique<AST::Variable>(
				std::make_unique<AST::i32>(),
				"length"
				)
			);

		auto P = std::make_unique<AST::Prototype>(std::make_unique<AST::i32>(), "input", std::move(ArgNames), "i32");

		P->codegen();
		AST::FunctionProtos["input"] = std::move(P);
	}

	static void random_between()
	{
		std::vector<std::unique_ptr<AST::Variable>> ArgNames;

		ArgNames.push_back(
			std::make_unique<AST::Variable>(
				std::make_unique<AST::i32>(),
				"min"
				)
			);

		ArgNames.push_back(
			std::make_unique<AST::Variable>(
				std::make_unique<AST::i32>(),
				"max"
				)
			);

		auto P = std::make_unique<AST::Prototype>(std::make_unique<AST::i32>(), "random_between", std::move(ArgNames), "i32");

		P->codegen();
		AST::FunctionProtos["random_between"] = std::move(P);
	}

	static void start()
	{
		// Say.c
		say();
		i8_to_string();
		//i32_to_string();
		say_i8();
		say_i32();
		input();

		// Random.c
		random_between();
	}
};

#endif
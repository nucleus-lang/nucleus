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

	static void say_i8_number()
	{
		std::vector<std::unique_ptr<AST::Variable>> ArgNames;

		ArgNames.push_back(
			std::make_unique<AST::Variable>(
				std::make_unique<AST::i8>(),
				"number"
				)
			);

		auto P = std::make_unique<AST::Prototype>(std::make_unique<AST::i32>(), "say_i8_number", std::move(ArgNames), "i32");

		P->codegen();
		AST::FunctionProtos["say_i8_number"] = std::move(P);
	}

	static void start()
	{
		say();
		say_i8_number();
	}
};

#endif
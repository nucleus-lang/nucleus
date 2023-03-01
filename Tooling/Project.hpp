#ifndef PROJECT_HPP
#define PROJECT_HPP

#include <string>

struct Project
{
	static void Create(std::string name);
	static void CreateTOML(std::string name);
	static void CreateMainNk(std::string name);
};

#endif
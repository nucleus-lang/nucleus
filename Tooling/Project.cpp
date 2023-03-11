#include "Project.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>

void Project::CreateTOML(std::string name)
{
	std::string path = name + "/Nucleus.toml";

	std::string content = "[project]\n";
	content += "name = \"" + name + "\"\n";
	content += "version = \"1.0.0\"\n\n";

	content += "[dependencies]\n\n";

	//content += "[imports]\n";
	//content += "folders = [\n";
	//content += "\t\"Nucleus.Std\"\n";
	//content += "]\n";

	std::ofstream o(path.c_str());

  	o << content << "\n";
}

void Project::CreateMainNk(std::string name)
{
	std::string path = name + "/main.nk";

	std::string content = "fn main(): i32 {\n";
	content += "\treturn 0;\n";
	content += "}\n";

	std::ofstream o(path.c_str());

  	o << content;
}

void Project::Create(std::string name)
{
	for(auto c : name)
	{
		if(c < 32)
			continue;

		if(!isalpha(c))
		{
			std::cout << "Error: Sorry, but the name of the project can't have special characters, only letters and numbers are allowed!\n";
			exit(1);
		}
	}

	if(std::filesystem::exists(name))
	{
		std::cout << "Error: Sorry, but the folder '" << name << "' already exists...\n";
		exit(1);
	}

	std::cout << "Creating \"" << name << "\"...\n";

	std::filesystem::create_directory(name);

	CreateTOML(name);

	CreateMainNk(name);

	std::cout << "Project \"" << name << "\" successfully created!\n";
}
#ifndef TODOLIST_HPP
#define TODOLIST_HPP

#include <iostream>
#include <string>
#include <vector>

struct TodoList_Item
{
	std::string text;
	unsigned int line;
	unsigned int column;
};

struct TodoList
{
	static std::vector<TodoList_Item> all_items;

	static void add(std::string text, unsigned int line, unsigned int column);

	static void print_item(TodoList_Item t);

	static void print();
};

#endif
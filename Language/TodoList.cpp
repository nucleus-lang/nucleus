#include "TodoList.hpp"

std::vector<TodoList_Item> TodoList::all_items;

void TodoList::add(std::string text, unsigned int line, unsigned int column) {

	TodoList_Item t;
	t.text = text;
	t.line = line;
	t.column = column;

	all_items.push_back(t);
}

void TodoList::print_item(TodoList_Item t) {

	std::cerr << " - " << t.text << " (" << t.line << ":" << t.column << ")\n";
}

void TodoList::print() {

	if(all_items.size() == 0)
		return;

	std::cerr << "\n\nTODO LIST:\n";
	std::cerr << "===============\n\n";

	for(auto i : all_items)
	{
		print_item(i);
	}

	std::cerr << "\n===============\n\n";

}
#include <plib/trie.hpp>

#include <iostream>

#include <string>

void print(std::vector<std::string> const& v) {
	for (auto const& s : v) {
		std::cout << s << " ";
	}
}

int main() {
	plib::trie<std::string, int> mytrie{ plib::trie<std::string, int>::alphabet{.min = 'a', .max = 'e'}};
	mytrie.insert("abc", 5);
	mytrie.insert("bec", 8);
	mytrie.insert("abd", 3);
	mytrie.insert("acc", 3);
	mytrie.insert("a", 0);

	std::cout << std::boolalpha;

	print(mytrie.collect_all_keys());
	std::cout << std::endl;
	print(mytrie.collect_with_prefix("ab"));
	std::cout << std::endl;
	print(mytrie.collect_with_prefix("abc"));
	std::cout << std::endl;

	std::cout << *mytrie.get("abc") << std::endl;
	std::cout << *mytrie.get("acc") << std::endl;

	std::cout << (mytrie.get("aaa") == std::nullopt) << std::endl;
}
#pragma once

#include <concepts>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <vector>


namespace plib {

/**
 * @brief Implementation for a string trie. Useful for autocompletion or storing similar strings. 
 *		  The used representation is a hybrid ternary search trie, where the root node is a list of R ternary search tries.
 * @tparam S String type to be stored. The value_type of the string must be convertible to an index.
 * @tparam V Value type accociated with each stored string.
*/
template<typename S, typename V> requires std::convertible_to<typename S::value_type, std::size_t>
class trie {
public:
	/**
	 * @brief Character type of nodes in the trie.
	*/
	using character_type = typename S::value_type;

	/**
	 * @brief Key type of the S -> V mapping
	*/
	using key_type = S;

	/**
	 * @brief Value tyoe of the S -> V mapping
	*/
	using value_type = V;

	/**
	 * @brief Represents an alphabet of the trie. Any values outside of this range are forbidden and cannot be stored in the trie.
	*/
	struct alphabet {
		/**
		 * @brief Character with the smallest value in the alphabet.
		*/
		character_type min = std::numeric_limits<character_type>::min();

		/**
		 * @brief Character with the largest value in the alphabet.
		*/
		character_type max = std::numeric_limits<character_type>::max();
	};

	/**
	 * @brief Construct a trie with a given alphabet.
	 * @param alpha Alphabet for the trie. This can help the implementation choose a more efficient representation.
	 *		  The default alphabet includes the whole character range.
	*/
	trie(alphabet alpha = {}) : alpha(alpha) {
		alphabet_size = static_cast<std::int64_t>(alpha.max) - static_cast<std::int64_t>(alpha.min) + 1;

		root_node.resize(alphabet_size);
		for (std::uint32_t i = 0; i < alphabet_size; ++i) {
			root_node[i] = std::make_unique<ternary_node>();
			root_node[i]->key = static_cast<character_type>(static_cast<std::uint32_t>(alpha.min) + i);
		}
	}

	/**
	 * @brief Get the used alphabet.
	 * @return The alphabet that represents the values that can be stored in this trie.
	*/
	alphabet get_alphabet() const {
		return alpha;
	}

	/**
	 * @brief Get the amount of characters in the alphabet.
	 * @return alpha.max - alpha.min + 1
	*/
	std::uint32_t alpha_size() const {
		return alphabet_size;
	}

	/**
	 * @brief Insert a new value into the trie.
	 * @param str The string to insert. An empty string will be ignored.
	*/
	void insert(S const& str, V&& value) {
		if (str.size() == 0) return;

		// Get the first character so we can index into our TST.
		character_type first = str[0];
		auto& root = root_node[char_index(first)];
		// Note that we always insert in middle from the root node, since the character matches.
		// We also make sure to only call this insert when the length of the string is > 1.
		if (str.size() > 1) {
			root->middle = tst_insert(root->middle, str, std::move(value), 1);
		}
		else {
			// This key only has a single character, mark the key as an entry.
			root->value = std::move(value);
		}
	}


	/**
	 * @brief Get the value associated with a given string.
	 * @param str The string key to search for.
	 * @return An optional containing the value, or std::nullopt if the key was not found.
	*/
	std::optional<value_type> get(S const& str) const {
		if (str.size() == 0) return std::nullopt;

		character_type first = str[0];
		auto const& root = root_node[char_index(first)];
		if (str.size() > 1) {
			auto const& node = tst_get(root->middle, str, 1);
			if (node) return node->value;
			else return std::nullopt;
		}
		else {
			return root->value;
		}
	}

	/**
	 * @brief Query whether the trie contains a given string.
	 * @param str The string to search for.
	 * @return True if the trie contains it, false if not.
	*/
	bool contains(S const& str) const {
		return get(str) != std::nullopt;
	}


	std::vector<S> collect_with_prefix(S const& prefix) const {
		std::vector<S> result;
		if (prefix.size() == 0) {
			// No prefix, instead we will collect with every character as a singular prefix.
			// This is necessary because of how our hybrid TST is implemented.
			for (character_type c = alpha.min; c <= alpha.max; ++c) {
				S new_prefix;
				new_prefix.push_back(c);

				auto const& root = root_node[char_index(c)];
				tst_collect(root, "", new_prefix, result);
			}
		}
		else if (prefix.size() == 1) {
			character_type c = prefix[0];
			S new_prefix;
			new_prefix.push_back(c);

			auto const& root = root_node[char_index(c)];
			tst_collect(root, "", new_prefix, result);
		}
		else {
			character_type c = prefix[0];
			auto const& root = root_node[char_index(c)];
			auto const& node = tst_get(root, prefix, 0);
			if (node->middle) {
				tst_collect(node->middle, prefix, prefix + node->middle->key, result);
			}
			// This node could also be a value, add it.
			if (node->value) {
				result.push_back(prefix);
			}
		}

		return result;
	}

	/**
	 * @brief Collect all strings in the trie.
	 * @return A vector containing every string in the trie
	*/
	std::vector<S> collect_all_keys() const {
		return collect_with_prefix(S{});
	}

private:
	/**
	 * @brief Node in the TST.
	*/
	struct ternary_node {
		/**
		 * @brief Character key of this node.
		*/
		character_type key{};

		/**
		 * @brief Value of this node, or std::nullopt if it has no value
		*/
		std::optional<value_type> value = std::nullopt;

		/**
		 * @brief TST with key < this.key
		*/
		std::unique_ptr<ternary_node> left;
		/**
		 * @brief TST with key == this.key
		*/
		std::unique_ptr<ternary_node> middle;
		/**
		 * @brief TST with key > this.key
		*/
		std::unique_ptr<ternary_node> right;
	};

	alphabet alpha;
	std::uint32_t alphabet_size = 0;

	std::vector<std::unique_ptr<ternary_node>> root_node{};

	std::uint32_t char_index(character_type c) const {
		return static_cast<std::uint32_t>(c) - static_cast<std::uint32_t>(alpha.min);
	}

	std::unique_ptr<ternary_node> tst_insert(std::unique_ptr<ternary_node>& node, S const& str, value_type&& value, std::size_t index) {
		character_type c = str[index];

		// Node wasn't created yet.
		if (node == nullptr) {
			node = std::make_unique<ternary_node>();
			node->key = c;
		}

		// Insert in the correct sub-trie
		if (c < node->key) node->left = tst_insert(node->left, str, std::forward<value_type>(value), index);
		else if (c > node->key) node->right = tst_insert(node->right, str, std::forward<value_type>(value), index);
		else if (index < str.size() - 1) node->middle = tst_insert(node->middle, str, std::forward<value_type>(value), index + 1);
		else node->value = std::move(value);

		return std::move(node);
	}

	std::unique_ptr<ternary_node> const& tst_get(std::unique_ptr<ternary_node> const& node, S const& str, std::size_t index) const {
		static std::unique_ptr<ternary_node> el_rubio;
		if (node == nullptr) return el_rubio;

		character_type c = str[index];
		if (c < node->key) return tst_get(node->left, str, index);
		else if (c > node->key) return tst_get(node->right, str, index);
		else if (index < str.size() - 1) return tst_get(node->middle, str, index + 1);
		else return node;
	}

	void tst_collect(std::unique_ptr<ternary_node> const& node, S const& prev_prefix, S const& prefix, std::vector<S>& result) const {
		if (node == nullptr) return;

		if (node->value) {
			result.push_back(prefix);
		}

		if (node->left) tst_collect(node->left, prefix, prev_prefix + node->left->key, result);
		if (node->middle) tst_collect(node->middle, prefix, prefix + node->middle->key, result);
		if (node->right) tst_collect(node->right, prefix, prev_prefix + node->right->key, result);
	}
};

}
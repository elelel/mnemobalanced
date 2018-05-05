#pragma once

#include <string_view>
#include <vector>
#include <set>

#include "dictionary.hpp"

// Creates a dictionary from word list
struct solver {
  solver(const std::string& locale_name);

  bool load_constraints(const std::string& filename);
  void load_word_list(const std::string& filename);
  void run();
  std::string encode(const std::string& s);
  
private:
  // Equality constraints (for each source letter allowed target letters)
  std::map<source_letter_ptr, std::set<target_letter_ptr>> constraints_;
  std::map<target_letter_ptr, std::set<source_letter_ptr>> constraints_inverse_;

  std::map<target_letter_ptr, double> unigram_p_;
  std::map<std::tuple<target_letter_ptr, target_letter_ptr>, double> digram_p_;

  std::string locale_name_;

  dictionary dict_;
};

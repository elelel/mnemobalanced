#include "dictionary.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <codecvt>
#include <locale>

matching_node::matching_node(const std::wstring& s) :
  s_(s) {
}

std::string matching_node::to_string() const {
 std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> convert;
 std::string utf8String = convert.to_bytes(s_);
 return utf8String;
}

void dictionary::pp_source_to_target() {
  std::cout << "Dictionary source to target " << std::endl;
  for (const auto& p: source_to_target) {
    std::cout << p.first->to_string() << " = [";
    bool need_comma = false;
    for (const auto& t : p.second) {
      if (need_comma) std::cout << ", ";
      std::cout << t->to_string();
      need_comma = true;
    }
    std::cout << "]" << std::endl;
  }
}

dictionary& operator<<(dictionary& d, const std::string_view& s) {
}

std::stringstream& operator<<(std::stringstream& ss, const dictionary& d) {
}


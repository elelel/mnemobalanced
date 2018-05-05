#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>
#include <sstream>

// Base class for bipartite matching graph
struct matching_node {
  matching_node(const std::wstring& s);

  std::string to_string() const;
private:
  std::wstring s_;
};

// Color 1 node for bipartite matching (target dictionary letter)
struct target_letter : matching_node {
  using matching_node::matching_node;
};

// Color 2 node for bipartite matching (source digit)
struct source_letter : matching_node {
  using matching_node::matching_node;
};

using source_letter_ptr = std::shared_ptr<source_letter>;
using target_letter_ptr = std::shared_ptr<target_letter>;

template <typename T>
struct nodes_collection {
  nodes_collection() {};

  nodes_collection(const nodes_collection& other) :
    map_(other.map_) {
  }
  
  T find(const std::wstring& s) {
    auto found = map_.find(s);
    if (found != map_.end()) return found->second;
    return T{};
  }
  
  T add(const std::wstring& s) {
    auto found = find(s);
    if (found == nullptr) {
      auto n = std::make_shared<typename T::element_type>(s);
      map_.insert({s, n});
      return n;
    } else {
      return found;
    }
  }
private:
  std::map<std::wstring, T> map_;
};

using source_to_target_map = std::map<source_letter_ptr, std::set<target_letter_ptr>>;

struct dictionary {
  nodes_collection<source_letter_ptr> source;
  nodes_collection<target_letter_ptr> target;

  void pp_source_to_target();

  source_to_target_map source_to_target;
};

// Serialize/deserialize dictionary
dictionary& operator<<(dictionary& d, const std::string_view& s);
std::stringstream& operator<<(std::stringstream& ss, const dictionary& d);


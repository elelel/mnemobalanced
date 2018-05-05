#include "solver.hpp"

#include <algorithm>
#include <sstream>
#include <fstream>
#include <codecvt>
#include <locale>
#include <iostream>
#include <numeric>
#include <cmath>

solver::solver(const std::string& locale_name) :
  locale_name_(locale_name) {
}

bool solver::load_constraints(const std::string& filename) {
  std::wifstream wif(filename);
  wif.imbue(std::locale(std::locale(locale_name_), new std::codecvt_utf8<wchar_t>));
  std::wstringstream wss;
  wss << wif.rdbuf();
  std::wstring s;
  bool source_line{true};
  std::wstring::value_type src;
  std::size_t count{0};
  while (wss >> s) {
    if (source_line) {
      src = s[0];
      source_line = false;
    } else {
      for (const auto& trg : s) {
        constraints_[dict_.source.add(std::wstring(1, src))].insert(dict_.target.add(std::wstring(1, trg)));
        constraints_inverse_[dict_.target.add(std::wstring(1, trg))].insert(dict_.source.add(std::wstring(1, src)));
        ++count;
      }
      source_line = true;
    }
  }
  dict_.source_to_target = constraints_;
  std::cout << "Loaded " << count << " equality constraints" << std::endl;
  return true;
}

void solver::load_word_list(const std::string& filename) {
  std::wifstream wif(filename);
  wif.imbue(std::locale(std::locale(locale_name_), new std::codecvt_utf8<wchar_t>));
  std::wstringstream wss;
  wss << wif.rdbuf();
  std::wstring s;
  std::size_t count{0};
  double unigram_z;
  double digram_z;
  while (wss >> s) {
    std::optional<target_letter_ptr> prev;
    for (const auto& c : s) {
      target_letter_ptr c_ptr = dict_.target.find(std::wstring(1, c));
      if (c_ptr) {
        // This is an info-symbol
        ++unigram_p_[c_ptr];
        ++unigram_z;
        if (prev) {
          target_letter_ptr prev_c = prev.value();
          auto digram = std::tuple<target_letter_ptr, target_letter_ptr>(prev_c, c_ptr);
          ++digram_p_[digram];
          ++digram_z;
        } else {
          prev = c_ptr;
        }
      } else {
        // This is a slack-symbol
      }
    }
  }
  for (auto& u : unigram_p_) u.second = u.second / unigram_z;
  for (auto& d : digram_p_) d.second = d.second / digram_z;
  std::cout << "Processed "
            << unigram_z << " unigrams, "
            << digram_z << " digrams occurences from word list '"
            << filename << "'" << std::endl;;
}

// Get source letter probabilities if we apply current source_to_target map
std::map<source_letter_ptr, double> source_probabilities(const source_to_target_map& source_to_target,
                                                         const std::map<target_letter_ptr, double>& target_probabilities) {
  // Calculate probabilities of source letters
  std::map<source_letter_ptr, double> source_p;
  for (auto& st : source_to_target) {
    auto& s = st.first;
    auto found = source_p.find(s);
    if (found == source_p.end()) {
      source_p.insert({s, 0.0});
      found = source_p.find(s);
    }
    for (auto& t : st.second) {
      auto p = target_probabilities.find(t);
      if (p != target_probabilities.end()) {
        found->second = found->second + p->second;
      }
    }
  }
  // Renormalize
  double z{0.0};
  for (const auto& sp : source_p) {
    z += sp.second;
  }
  for (auto& sp : source_p) {
    sp.second /= z;
  }
  return source_p;
}

// Optimization target to maximize
double fitness_measure(const source_to_target_map& source_to_target,
                       const std::map<target_letter_ptr, double>& target_probabilities, const size_t additional = 0) {
  auto source_p = source_probabilities(source_to_target, target_probabilities);
  // Multiplier for stability
  double w{10.0};
  // Construct ideal vector to compare to (equal probabilities)
  std::vector<double> ideal(source_p.size(), (double(1.0) * w) / double(source_p.size()));
  // Construct actual source vector to compare
  std::vector<double> actual;
  for (const auto& sp : source_p) {
    actual.push_back(sp.second * w);
  }
  double sum = 0;
  for (auto& s : ideal) sum +=s;
  //  std::cout << "Ideal sum " << sum << std::endl;
  sum = 0;
  for (auto& s : actual) sum +=s;
  //  std::cout << "Actual sum " << sum << std::endl;

  for (auto i = 0; i < additional; ++i) {
    ideal.push_back(1 * w);
    actual.push_back(1 * w);
  }

  if (actual.size() != ideal.size()) {
    std::cout << "Error in vector length" << std::endl;
    exit(0);
  }
  
  // Cosine distance
  double dp_ia = std::inner_product(ideal.begin(), ideal.end(), actual.begin(), 0);
  double sqrt_dp_ii = std::sqrt(std::inner_product(ideal.begin(), ideal.end(), ideal.begin(), 0));
  double sqrt_dp_aa = std::sqrt(std::inner_product(actual.begin(), actual.end(), actual.begin(), 0));
  double dist = dp_ia / (sqrt_dp_ii * sqrt_dp_aa);
  //  std::cout << " vector size " << source_p.size() << " fitness " << dist << std::endl;
  return dist;
}

std::vector<std::pair<target_letter_ptr, size_t>> enumerate_conflicts(const source_to_target_map& source_to_target) {
  std::set<target_letter_ptr> target_letters;
  for (const auto& p : source_to_target) {
    target_letters.insert(p.second.begin(), p.second.end());
  }
  std::set<target_letter_ptr> target_used;
  std::map<target_letter_ptr, size_t> target_conf;
  for (const auto& p : source_to_target) {
    for (const auto& t : p.second) {
      if (target_used.find(t) == target_used.end()) {
        target_used.insert(t);
      } else {
        target_conf[t] = target_conf[t] + 1;
      }
    }
  }
  std::vector<std::pair<target_letter_ptr, size_t>> rslt(target_conf.begin(), target_conf.end());
  std::sort(rslt.begin(), rslt.end(), [] (const auto& a, const auto& b) {
      return a.second > b.second;
    });
  return rslt;
}

std::string solver::encode(const std::string& s) {
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> convert;
  std::wstring ws = convert.from_bytes(s);
  std::vector<source_letter_ptr> source;
  for (const auto& wc : ws) {
    source.push_back(dict_.source.add(std::wstring(1, wc)));
  }
}

void solver::run() {
  size_t i{0};
  decltype(enumerate_conflicts(dict_.source_to_target)) conflicts;
  while ((conflicts = enumerate_conflicts(dict_.source_to_target)).size() != 0) {
    bool updated{false};
    auto current_fitness = fitness_measure(dict_.source_to_target,
                                           unigram_p_);

    std::cout << "Iteration " << i << ": " << conflicts.size() << " conflicts, fitness " << current_fitness << std::endl;
    for (auto& c: conflicts) {
      //      std::cout << "    Conflicts for '" << c.first->to_string() << "' " << c.second << std::endl;
    }

    std::map<target_letter_ptr, std::set<source_letter_ptr>> target_to_source;
    for (const auto& p: dict_.source_to_target) {
      for (const auto& t: p.second) {
        target_to_source[t].insert(p.first);
      }
    }
    std::vector<std::pair<source_to_target_map, double>> successors;
    for (const auto& p : conflicts) {
      auto& t = p.first;
      for (const auto& s : target_to_source[t]) {
        auto proposed_map = dict_.source_to_target;
        if (proposed_map[s].size() > 1) {
          proposed_map[s].erase(t);
          auto new_fitness = fitness_measure(proposed_map, unigram_p_, 1);
          std::cout << "  Removing target '" << t->to_string() << "' from source '" << s->to_string() << "' will give fitness " << new_fitness << std::endl;
          if (new_fitness > current_fitness) successors.push_back({proposed_map, new_fitness});
        }
      }
    }
    if (successors.size() > 0) {
      std::sort(successors.begin(), successors.end(), [] (const auto& a, const auto& b) {
          return a.second > b.second;
        });
      dict_.source_to_target = successors.begin()->first;
      updated = true;
    }

    if (!updated) {
      // Fitness heuristics failed, just remove a violating constraint
      std::cout << "TODO Fitness heuristics failed, implement: remove a violating constraint" << std::endl;
      break;
    }
    ++i;
    dict_.pp_source_to_target();
  }
  dict_.pp_source_to_target();
  auto probabs = source_probabilities(dict_.source_to_target, unigram_p_);
  std::cout << "Resulting balance in word frequency:" << std::endl;
  for (const auto& p : probabs) {
    std::cout << p.first->to_string() << " = " << p.second << std::endl;
  }

  std::string pi =
    "3141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117067982148086513282306647093844609550";
  std::cout << "Encoded: " << encode(pi) << std::endl;
}


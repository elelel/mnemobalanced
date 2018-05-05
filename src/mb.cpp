#include "solver.hpp"

#include <iostream>

int main(int argc, char **argv) {
  bool show_help = true;
  if (argc > 2) {
    if (std::string(argv[1]) == "generate") {
      if (argc == 4) {
        auto constraints_fn = std::string(argv[2]);
        auto word_list_fn = std::string(argv[3]);
        solver s("ru_RU.UTF-8");
        s.load_constraints(constraints_fn);
        s.load_word_list(word_list_fn);
        s.run();
        show_help = false;
      }
    }
  }

  if (show_help) {
    std::cout << "Usage: mb generate constraints_file.txt word_list.txt" << std::endl;
  }
  return 0;
}


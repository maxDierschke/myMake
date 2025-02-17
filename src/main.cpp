#include "rule_executor.h"

int main(int argc, char* argv[]) {

  auto executor{executor::rule_executor::executor_from_make_file()};
  for (int i{1}; i < argc; ++i) {
    executor.execute_rule(argv[i]);
  }
}

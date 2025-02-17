#pragma once

#include <string>
#include <unordered_map>

#include "rule_types.h"

namespace executor {

class rule_executor {
 public:
  static rule_executor executor_from_make_file(
      std::string file_path = "my_make_file");
  void execute_rule(std::string rul_name);

 private:
  explicit rule_executor(rules::dependency_graph dependency_graph)
      : dependency_graph_{std::move(dependency_graph)} {};

  rules::dependency_graph dependency_graph_;
};

}  // namespace executor

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace rules {

struct rule {
  std::string name;
  std::vector<std::string> dependencies;
  std::string command;
};

rule from_string(std::string str);

using dependency_graph = std::unordered_map<std::string, rule>;
}  // namespace rules

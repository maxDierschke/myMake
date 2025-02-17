#include "rule_types.h"

namespace rules {

rule from_string(std::string str) {
  auto split_string{[](std::string_view input, char split_char) {
    std::vector<std::string> arr{};
    int start, end = -1;
    do {
      start = end + 1;
      end = input.find(split_char, start);
      auto substring{input.substr(start, end - start)};
      if (substring != "") {
        arr.push_back(std::string{substring});
      }
    } while (end != -1);
    return arr;
  }};

  auto values{split_string(str, ':')};
  auto dependencies{split_string(values[1], ' ')};
  auto name{values[0]};
  name.erase(std::remove(name.begin(), name.end(), ' '), name.end());
  return rule{name, std::move(dependencies), values[2]};
}

}  // namespace rules

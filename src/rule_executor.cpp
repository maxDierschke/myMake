#include "rule_executor.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include "rule_types.h"

namespace executor {

namespace {

using file_clock = std::chrono::time_point<std::chrono::file_clock>;

enum class status { UP_TO_DATE, EXECUTED };

std::optional<file_clock> last_change_of_file_in_current_path(
    const std::string& relative_file_path) {
  auto file_path{std::filesystem::current_path() / relative_file_path};
  return std::filesystem::is_regular_file(file_path)
             ? std::make_optional(std::filesystem::last_write_time(file_path))
             : std::nullopt;
}

status was_dependency_updated_after_change_time(
    std::optional<file_clock> last_change_time, const std::string& leaf_name) {
  if (!last_change_time.has_value()) {
    return status::EXECUTED;
  }
  auto leaf_change_time{last_change_of_file_in_current_path(leaf_name)};
  auto child_was_executed{leaf_change_time.has_value() &&
                          leaf_change_time.value() > last_change_time.value()};
  return child_was_executed ? status::EXECUTED : status::UP_TO_DATE;
}

bool was_file_updated_after_starting_time(
    std::optional<file_clock> maybe_last_file_change_time,
    file_clock start_time) {
  return maybe_last_file_change_time.has_value() &&
         *maybe_last_file_change_time > start_time;
}

status recursive_execute_on_dependency_change(
    const rules::rule& rule, const rules::dependency_graph& phase_lookup,
    const std::optional<file_clock> maybe_last_parent_execution = std::nullopt,
    const file_clock exec_start_time = std::chrono::file_clock::now()) {

  auto maybe_last_change_time_of_current_file{
      last_change_of_file_in_current_path(rule.name)};

  if (was_file_updated_after_starting_time(
          maybe_last_change_time_of_current_file, exec_start_time)) {
    return status::EXECUTED;
  }

  auto file_path{std::filesystem::current_path() / rule.name};
  bool any_child_updated{false};
  for (const auto& dependency_name : rule.dependencies) {
    auto dependency_was_executed{
        phase_lookup.contains(dependency_name)
            ? recursive_execute_on_dependency_change(
                  phase_lookup.at(dependency_name), phase_lookup,
                  maybe_last_change_time_of_current_file, exec_start_time)
            : was_dependency_updated_after_change_time(
                  maybe_last_change_time_of_current_file, dependency_name)};

    if (dependency_was_executed == status::EXECUTED) {
      any_child_updated = true;
    }
  }
  bool file_exists{std::filesystem::is_regular_file(file_path)};
  bool execute{any_child_updated || !file_exists};
  if (execute) {
    std::cout << "Executing " << rule.name << std::endl;
    system(rule.command.c_str());
  }
  auto dependency_was_updated_after_parent{
      [](const auto& maybe_file_change_time,
         const auto& maybe_parent_change_time) {
        return maybe_parent_change_time.has_value() &&
               maybe_file_change_time.has_value() &&
               *maybe_file_change_time > *maybe_parent_change_time;
      }};
  bool executed{execute || dependency_was_updated_after_parent(
                               maybe_last_change_time_of_current_file,
                               maybe_last_parent_execution)};
  return executed ? status::EXECUTED : status::UP_TO_DATE;
}
}  // namespace

void rule_executor::execute_rule(const std::string rule_name) {
  if (dependency_graph_.contains(rule_name)) {
    std::cout << "Starting " << rule_name << std::endl;
    auto execution_status{recursive_execute_on_dependency_change(
        dependency_graph_.at(rule_name), dependency_graph_)};
    if (execution_status == status::EXECUTED) {
      std::cout << "Executed " << rule_name << std::endl;
    } else {
      std::cout << rule_name << " is already up to date" << std::endl;
    }
  } else {
    std::cout << "Rule with name \"" << rule_name
              << "\" not defined in make file" << std::endl;
  }
}

rule_executor rule_executor::executor_from_make_file(std::string file_path) {
  rules::dependency_graph dependency_graph{};

  std::ifstream config_file{file_path};
  if (!config_file.is_open()) {
    throw std::exception();  // todo: throw better error
  }

  std::string current_line{};
  while (getline(config_file, current_line)) {
    auto rule{rules::from_string(current_line)};
    const auto name{rule.name};
    dependency_graph.try_emplace(name, std::move(rule));
  }

  return rule_executor{std::move(dependency_graph)};
}

}  // namespace executor

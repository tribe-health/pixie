#pragma once
#include <string>
#include <vector>

#include <pypa/ast/ast.hh>
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"

#include "src/carnot/compiler/ast/ast_visitor.h"
#include "src/carnot/compiler/objects/qlobject.h"

namespace pl {
namespace carnot {
namespace compiler {

/**
 * @brief Data structure that contains parsed arguments. This separates the concerns of parsing
 * arguments from the actual implementation by storing evaluated arguments into a map.
 */
class ParsedArgs {
 public:
  void AddArg(const std::string& arg_name, IRNode* node) {
    DCHECK(!HasArgOrKwarg(arg_name));
    args_[arg_name] = node;
  }

  bool HasArgOrKwarg(std::string_view arg_name) { return HasArg(arg_name) || HasKwarg(arg_name); }

  IRNode* GetArg(std::string_view arg_name) const {
    DCHECK(args_.contains(arg_name)) << absl::Substitute("arg '$0' not found", arg_name);
    return args_.find(arg_name)->second;
  }

  void AddKwarg(const std::string& arg_name, IRNode* node) {
    DCHECK(!HasArgOrKwarg(arg_name));
    kwargs_.emplace_back(arg_name, node);
  }

  const std::vector<NameToNode>& kwargs() const { return kwargs_; }
  const absl::flat_hash_map<std::string, IRNode*>& args() const { return args_; }

 private:
  bool HasArg(std::string_view arg_name) { return args_.contains(arg_name); }
  bool HasKwarg(std::string_view kwarg) {
    for (const auto& kw : kwargs_) {
      if (kw.name == kwarg) {
        return true;
      }
    }
    return false;
  }

  // The mapping of specified arguments to the ir representation.
  absl::flat_hash_map<std::string, IRNode*> args_;
  // Holder for extra kw args if the function has a **kwargs argument.
  std::vector<NameToNode> kwargs_;
};

using FunctionType = std::function<StatusOr<QLObjectPtr>(const pypa::AstPtr&, const ParsedArgs&)>;

class FuncObject : public QLObject {
 public:
  static constexpr TypeDescriptor FuncType = {
      /* name */ "Function",
      /* type */ QLObjectType::kFunction,
  };
  // The default type. I haven't completely decided this API so using an alias for now.
  using DefaultType = std::string;
  /**
   * @brief Construct a new Python Function.
   *
   * @param name the name of the function.
   * @param arguments the list of all argument names.
   * @param defaults the list of all defaults. Each key must be a part of arguments, otherwise will
   * fail.
   * @param has_variable_len_kwargs whether or not this supports generic keyword arguments.
   * @param impl the implementation of the function.
   */
  FuncObject(const std::string_view name, const std::vector<std::string>& arguments,
             const absl::flat_hash_map<std::string, DefaultType>& defaults,
             bool has_variable_len_kwargs, FunctionType impl);

  /**
   * @brief Call this function with the args.
   *
   * @param args the args to pass into the function.
   * @param ast the ast object where this function is called. Used for reporting errors accurately.
   * @return The return type of the object or an error if something goes wrong during function
   * processing.
   */
  StatusOr<QLObjectPtr> Call(const ArgMap& args, const pypa::AstPtr& ast, ASTVisitor* ast_visitor);
  const std::string& name() const { return name_; }

  const std::vector<std::string>& arguments() const { return arguments_; }

 private:
  StatusOr<ParsedArgs> PrepareArgs(const ArgMap& args, const pypa::AstPtr& ast,
                                   ASTVisitor* ast_visitor);

  StatusOr<IRNode*> GetDefault(std::string_view arg, ASTVisitor* ast_visitor);
  bool HasDefault(const std::string& arg);

  std::string FormatArguments(const absl::flat_hash_set<std::string> args);

  int64_t NumArgs() const { return arguments_.size(); }
  int64_t NumPositionalArgs() const { return NumArgs() - defaults_.size(); }

  std::string name_;
  std::vector<std::string> arguments_;
  absl::flat_hash_map<std::string, DefaultType> defaults_;
  FunctionType impl_;
  bool has_variable_len_kwargs_;
};

}  // namespace compiler
}  // namespace carnot
}  // namespace pl

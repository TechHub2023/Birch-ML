/**
 * @file
 */
#include "src/common/Scope.hpp"

#include "src/expression/Parameter.hpp"
#include "src/expression/Generic.hpp"
#include "src/statement/GlobalVariable.hpp"
#include "src/statement/MemberVariable.hpp"
#include "src/statement/LocalVariable.hpp"
#include "src/statement/Function.hpp"
#include "src/statement/Program.hpp"
#include "src/statement/MemberFunction.hpp"
#include "src/statement/BinaryOperator.hpp"
#include "src/statement/UnaryOperator.hpp"
#include "src/statement/Basic.hpp"
#include "src/statement/Class.hpp"
#include "src/exception/all.hpp"

birch::Scope::Scope(const ScopeCategory category) :
    base(nullptr),
    category(category) {
  //
}

void birch::Scope::lookup(NamedExpression* o) const {
  auto name = o->name->str();

  auto parameter = parameters.find(name);
  auto localVariable = localVariables.find(name);
  auto memberVariable = memberVariables.find(name);
  auto globalVariable = globalVariables.find(name);
  auto memberFunction = memberFunctions.find(name);
  auto function = functions.find(name);
  auto binaryOperator = binaryOperators.find(name);
  auto unaryOperator = unaryOperators.find(name);
  auto program = programs.find(name);

  if (parameter != parameters.end()) {
    o->category = PARAMETER;
    o->number = parameter->second->number;
    o->type = parameter->second->type;
    o->uses = parameter->second;
    o->rank = parameter->second->useCount++;
  } else if (localVariable != localVariables.end()) {
    o->category = LOCAL_VARIABLE;
    o->number = localVariable->second->number;
    o->type = localVariable->second->type;
    o->uses = localVariable->second;
    o->rank = localVariable->second->useCount++;
  } else if (memberVariable != memberVariables.end()) {
    o->category = MEMBER_VARIABLE;
    o->number = memberVariable->second->number;
    o->type = memberVariable->second->type;
    o->uses = memberVariable->second;
    o->rank = memberVariable->second->useCount++;
  } else if (globalVariable != globalVariables.end()) {
    o->category = GLOBAL_VARIABLE;
    o->number = globalVariable->second->number;
    o->type = globalVariable->second->type;
    o->uses = globalVariable->second;
    o->rank = globalVariable->second->useCount++;
  } else if (memberFunction != memberFunctions.end()) {
    o->category = MEMBER_FUNCTION;
    o->number = memberFunction->second->number;
  } else if (function != functions.end()) {
    o->category = GLOBAL_FUNCTION;
    o->number = function->second->number;
  } else if (binaryOperator != binaryOperators.end()) {
    o->category = BINARY_OPERATOR;
    o->number = binaryOperator->second->number;
  } else if (unaryOperator != unaryOperators.end()) {
    o->category = UNARY_OPERATOR;
    o->number = unaryOperator->second->number;
  }
  if (base && !o->category) {
    base->lookup(o);
  }
}

void birch::Scope::lookup(NamedType* o) const {
  auto name = o->name->str();

  auto basicType = basicTypes.find(name);
  auto classType = classTypes.find(name);
  auto genericType = genericTypes.find(name);

  if (basicType != basicTypes.end()) {
    o->category = BASIC_TYPE;
    o->number = basicType->second->number;
  } else if (classType != classTypes.end()) {
    if (classType->second->has(STRUCT)) {
      o->category = STRUCT_TYPE;
    } else {
      o->category = CLASS_TYPE;
    }
    o->number = classType->second->number;
  } else if (genericType != genericTypes.end()) {
    o->category = GENERIC_TYPE;
    o->number = genericType->second->number;
  }
  //if (base && !o->category) {
  //  base->lookup(o);
  //}
}

void birch::Scope::add(Parameter* o) {
  auto param = parameters.find(o->name->str());
  if (param != parameters.end()) {
    throw RedefinedException(o, param->second);
  }
  auto local = localVariables.find(o->name->str());
  if (local != localVariables.end()) {
    throw RedefinedException(o, local->second);
  }
  parameters.insert(std::make_pair(o->name->str(), o));
}

void birch::Scope::add(LocalVariable* o) {
  auto param = parameters.find(o->name->str());
  if (param != parameters.end()) {
    throw RedefinedException(o, param->second);
  }
  auto local = localVariables.find(o->name->str());
  if (local != localVariables.end()) {
    throw RedefinedException(o, local->second);
  }
  localVariables.insert(std::make_pair(o->name->str(), o));
}

void birch::Scope::add(MemberVariable* o) {
  memberVariables.insert(std::make_pair(o->name->str(), o));
}

void birch::Scope::add(GlobalVariable* o) {
  globalVariables.insert(std::make_pair(o->name->str(), o));
}

void birch::Scope::add(MemberFunction* o) {
  memberFunctions.insert(std::make_pair(o->name->str(), o));
}

void birch::Scope::add(Function* o) {
  functions.insert(std::make_pair(o->name->str(), o));
}

void birch::Scope::add(Program* o) {
  programs.insert(std::make_pair(o->name->str(), o));
}

void birch::Scope::add(BinaryOperator* o) {
  binaryOperators.insert(std::make_pair(o->name->str(), o));
}

void birch::Scope::add(UnaryOperator* o) {
  unaryOperators.insert(std::make_pair(o->name->str(), o));
}

void birch::Scope::add(Basic* o) {
  basicTypes.insert(std::make_pair(o->name->str(), o));
}

void birch::Scope::add(Class* o) {
  classTypes.insert(std::make_pair(o->name->str(), o));
}

void birch::Scope::add(Generic* o) {
  genericTypes.insert(std::make_pair(o->name->str(), o));
}

void birch::Scope::inherit(Class* o) const {
  auto base = dynamic_cast<NamedType*>(o->base);
  if (base) {
    auto name = base->name->str();
    auto iter = classTypes.find(name);
    if (iter != classTypes.end()) {
      o->scope->base = iter->second->scope;

      /* check for loops in inheritance */
      auto scope = o->scope;
      do {
        if (scope->base == o->scope) {
          throw InheritanceLoopException(o);
        }
        scope = scope->base;
      } while (scope);
    }
  }
}

bool birch::Scope::overrides(const std::string& name) const {
  return base && (base->memberFunctions.find(name) != base->memberFunctions.end() ||
      base->overrides(name));
}

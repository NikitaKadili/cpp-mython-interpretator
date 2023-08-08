#include "statement.h"

#include <iostream>
#include <sstream>

using namespace std;

namespace ast {

using runtime::Closure;
using runtime::Context;
using runtime::ObjectHolder;

namespace {
const string ADD_METHOD = "__add__"s;
const string INIT_METHOD = "__init__"s;
}  // namespace

ObjectHolder Assignment::Execute(Closure& closure, Context& context) {
    closure[var_] = rv_->Execute(closure, context);
    return closure.at(var_);
}

Assignment::Assignment(std::string var, std::unique_ptr<Statement> rv)
    : var_(std::move(var))
    , rv_(std::move(rv))
{ /* do nothing */ }

VariableValue::VariableValue(const std::string& var_name)
    : dotted_ids_(1, std::move(var_name))
{ /* do nothing */ }

VariableValue::VariableValue(std::vector<std::string> dotted_ids)
    : dotted_ids_(std::move(dotted_ids))
{ /* do nothing */ }

ObjectHolder VariableValue::Execute(Closure& closure, Context& /*context*/) {
    // Заводим указатель на таблицу символов, будем обновлять при необходимости
    Closure* closure_ptr = &closure;
    ObjectHolder result;

    // Итерируемся по цепочке, присваиваем result очередной объект.
    // Если объект является классом - обновляем указатель на таблицу символов.
    // Если в таблице такой переменной нет - выбрасываем исключение
    for (const std::string& id : dotted_ids_) {
        if (closure_ptr->find(id) == closure_ptr->end()) {
            throw runtime_error("Wrong var name: "s + id);
        }

        result = closure_ptr->at(id);
        if (result.TryAs<runtime::ClassInstance>()) {
            closure_ptr = &result.TryAs<runtime::ClassInstance>()->Fields();
        }
    }

    return result;
}

unique_ptr<Print> Print::Variable(const std::string& name) {
    return make_unique<Print>(make_unique<VariableValue>(VariableValue(name)));
}

Print::Print(unique_ptr<Statement> argument) {
    args_.push_back(std::move(argument));
}

Print::Print(vector<unique_ptr<Statement>> args)
    : args_(std::move(args))
{ /* do nothing */ }

ObjectHolder Print::Execute(Closure& closure, Context& context) {
    auto& output = context.GetOutputStream();

    bool first = true;
    for (const unique_ptr<Statement>& var_ptr : args_) {
        ObjectHolder var = var_ptr->Execute(closure, context);

        if (!first) {
            output << ' ';
        }
        first = false;

        if (var) {
            var->Print(output, context);
        }
        else {
            output << "None"s;
        }
    }

    output << '\n';
    return {};
}

MethodCall::MethodCall(std::unique_ptr<Statement> object, std::string method,
                       std::vector<std::unique_ptr<Statement>> args)
    : method_(std::move(method))
    , object_(std::move(object))
    , args_(std::move(args))
{ /* do nothing */ }

ObjectHolder MethodCall::Execute(Closure& closure, Context& context) {
    auto class_ptr = object_->Execute(closure, context).TryAs<runtime::ClassInstance>();

    if (!class_ptr || !class_ptr->HasMethod(method_, args_.size())) {
        return {};
    }

    vector<ObjectHolder> actual_args;
    for (const unique_ptr<Statement>& arg_ptr : args_) {
        actual_args.push_back(arg_ptr->Execute(closure, context));
    }

    return class_ptr->Call(method_, actual_args, context);
}

ObjectHolder Stringify::Execute(Closure& closure, Context& context) {
    ObjectHolder var = argument_->Execute(closure, context);

    if (!var) {
        return ObjectHolder::Own(runtime::String("None"s));
    }

    stringstream ss;
    var->Print(ss, context);
    return ObjectHolder::Own(runtime::String(ss.str()));
}

ObjectHolder Add::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs_obj = lhs_->Execute(closure, context);
    ObjectHolder rhs_obj = rhs_->Execute(closure, context);

    if (!(lhs_obj && rhs_obj)) {
        throw runtime_error("Attemp to Add wrong object types"s);
    }

    if (lhs_obj.TryAs<runtime::Number>() && rhs_obj.TryAs<runtime::Number>()) {
        return ObjectHolder::Own(runtime::Number(lhs_obj.TryAs<runtime::Number>()->GetValue()
                                 + rhs_obj.TryAs<runtime::Number>()->GetValue()));
    }
    if (lhs_obj.TryAs<runtime::String>() && rhs_obj.TryAs<runtime::String>()) {
        return ObjectHolder::Own(runtime::String(lhs_obj.TryAs<runtime::String>()->GetValue()
                                 + rhs_obj.TryAs<runtime::String>()->GetValue()));
    }
    if (lhs_obj.TryAs<runtime::ClassInstance>()
            && lhs_obj.TryAs<runtime::ClassInstance>()->HasMethod(ADD_METHOD, 1)) {
        return lhs_obj.TryAs<runtime::ClassInstance>()->Call(ADD_METHOD, { rhs_obj }, context);
    }

    throw runtime_error("Attemp to Add wrong object types"s);
}

ObjectHolder Sub::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs_obj = lhs_->Execute(closure, context);
    ObjectHolder rhs_obj = rhs_->Execute(closure, context);

    if (!(lhs_obj && rhs_obj)) {
        throw runtime_error("Attemp to Substract wrong object types"s);
    }

    if (lhs_obj.TryAs<runtime::Number>() && rhs_obj.TryAs<runtime::Number>()) {
        return ObjectHolder::Own(runtime::Number(lhs_obj.TryAs<runtime::Number>()->GetValue()
                                 - rhs_obj.TryAs<runtime::Number>()->GetValue()));
    }

    throw runtime_error("Attemp to Substract wrong object types"s);
}

ObjectHolder Mult::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs_obj = lhs_->Execute(closure, context);
    ObjectHolder rhs_obj = rhs_->Execute(closure, context);

    if (!(lhs_obj && rhs_obj)) {
        throw runtime_error("Attemp to Multiply wrong object types"s);
    }

    if (lhs_obj.TryAs<runtime::Number>() && rhs_obj.TryAs<runtime::Number>()) {
        return ObjectHolder::Own(runtime::Number(lhs_obj.TryAs<runtime::Number>()->GetValue()
                                 * rhs_obj.TryAs<runtime::Number>()->GetValue()));
    }

    throw runtime_error("Attemp to Multiply wrong object types"s);
}

ObjectHolder Div::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs_obj = lhs_->Execute(closure, context);
    ObjectHolder rhs_obj = rhs_->Execute(closure, context);

    if (!(lhs_obj && rhs_obj)) {
        throw runtime_error("Attemp to Divide wrong object types"s);
    }

    if (lhs_obj.TryAs<runtime::Number>() && rhs_obj.TryAs<runtime::Number>()) {
        return ObjectHolder::Own(runtime::Number(lhs_obj.TryAs<runtime::Number>()->GetValue()
                                 / rhs_obj.TryAs<runtime::Number>()->GetValue()));
    }

    throw runtime_error("Attemp to Divide wrong object types"s);
}

ObjectHolder Compound::Execute(Closure& closure, Context& context) {
    for (const std::unique_ptr<Statement>& statemant : statements_) {
        statemant->Execute(closure, context);
    }

    return ObjectHolder::None();
}

ObjectHolder Return::Execute(Closure& closure, Context& context) {
    throw ReturnException(statement_->Execute(closure, context));
}

ClassDefinition::ClassDefinition(ObjectHolder cls)
    : cls_(cls)
{ /* do nothing */ }

ObjectHolder ClassDefinition::Execute(Closure& closure, Context& /*context*/) {
    closure[cls_.TryAs<runtime::Class>()->GetName()] = cls_;
    return cls_;
}

FieldAssignment::FieldAssignment(VariableValue object, std::string field_name,
                                 std::unique_ptr<Statement> rv)
    : object_(std::move(object))
    , field_name_(std::move(field_name))
    , rv_(std::move(rv))
{ /* do nothing */ }

ObjectHolder FieldAssignment::Execute(Closure& closure, Context& context) {
    Closure* closure_ptr = &closure;

    for (const string& id : object_.GetIds()) {
        closure_ptr = &closure_ptr->at(id).TryAs<runtime::ClassInstance>()->Fields();
    }

    (*closure_ptr)[field_name_] = rv_->Execute(closure, context);
    return closure_ptr->at(field_name_);
}

IfElse::IfElse(std::unique_ptr<Statement> condition, std::unique_ptr<Statement> if_body,
               std::unique_ptr<Statement> else_body)
    : condition_(std::move(condition))
    , if_body_(std::move(if_body))
    , else_body_(std::move(else_body))
{ /* do nothing */ }

ObjectHolder IfElse::Execute(Closure& closure, Context& context) {
    ObjectHolder condition_holder = condition_->Execute(closure, context);

    if (IsTrue(condition_holder)) {
        return if_body_->Execute(closure, context);
    }
    if (else_body_ != nullptr) {
        return else_body_->Execute(closure, context);
    }

    return ObjectHolder::None();
}

ObjectHolder Or::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs_obj = lhs_->Execute(closure, context);
    ObjectHolder rhs_obj = rhs_->Execute(closure, context);

    if (lhs_obj && rhs_obj) {
        return ObjectHolder::Own(runtime::Bool{ IsTrue(lhs_obj) || IsTrue(rhs_obj) });
    }

    throw runtime_error("Attemp to call operator Or for wrong object types"s);
}

ObjectHolder And::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs_obj = lhs_->Execute(closure, context);
    ObjectHolder rhs_obj = rhs_->Execute(closure, context);

    if (lhs_obj && rhs_obj) {
        return ObjectHolder::Own(runtime::Bool{ IsTrue(lhs_obj) && IsTrue(rhs_obj) });
    }

    throw runtime_error("Attemp to call operator And for wrong object types"s);
}

ObjectHolder Not::Execute(Closure& closure, Context& context) {
    ObjectHolder obj = argument_->Execute(closure, context);

    if (obj) {
        return ObjectHolder::Own(runtime::Bool{ !IsTrue(obj) });
    }

    throw runtime_error("Wrong argument parsed to Not"s);
}

Comparison::Comparison(Comparator cmp, unique_ptr<Statement> lhs, unique_ptr<Statement> rhs)
    : BinaryOperation(std::move(lhs), std::move(rhs))
    , cmp_(std::move(cmp))
{ /* do nothing */ }

ObjectHolder Comparison::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs_obj = lhs_->Execute(closure, context);
    ObjectHolder rhs_obj = rhs_->Execute(closure, context);
    return ObjectHolder::Own(runtime::Bool{ cmp_(lhs_obj, rhs_obj, context) });
}

NewInstance::NewInstance(const runtime::Class& class_, std::vector<std::unique_ptr<Statement>> args)
    : class_(class_)
    , args_(std::move(args))
{ /* do nothing */ }
NewInstance::NewInstance(const runtime::Class& class_)
    : class_(class_)
{ /* do nothing */ }

ObjectHolder NewInstance::Execute(Closure& closure, Context& context) {
    string class_name = class_.GetName();
    closure[class_.GetName()] = ObjectHolder::Own(runtime::ClassInstance(class_));

    runtime::ClassInstance* class_ptr = const_cast<runtime::ClassInstance*>(
                closure.at(class_name).TryAs<runtime::ClassInstance>()
                );

    if (class_ptr->HasMethod(INIT_METHOD, args_.size())) {
        vector<ObjectHolder> actual_args;
        for (const std::unique_ptr<Statement>& arg : args_) {
            actual_args.push_back(arg->Execute(closure, context));
        }
        class_ptr->Call(INIT_METHOD, actual_args, context);
    }

    return closure.at(class_name);
}

MethodBody::MethodBody(std::unique_ptr<Statement>&& body)
    : body_(std::move(body))
{ /* do nothing */ }

ObjectHolder MethodBody::Execute(Closure& closure, Context& context) {
    try {
        return body_->Execute(closure, context);
    }  catch (const ReturnException& ex) {
        return ex.GetResult();
    }
}

}  // namespace ast

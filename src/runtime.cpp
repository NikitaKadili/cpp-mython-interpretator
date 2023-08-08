#include "runtime.h"

#include <cassert>
#include <optional>
#include <sstream>

using namespace std;

namespace runtime {

namespace {
const string STR_METHOD = "__str__"s;
const string EQ_METHOD = "__eq__"s;
const string LT_METHOD = "__lt__"s;
} // namespace

ObjectHolder::ObjectHolder(std::shared_ptr<Object> data)
    : data_(std::move(data)) {
}

void ObjectHolder::AssertIsValid() const {
    assert(data_ != nullptr);
}

ObjectHolder ObjectHolder::Share(Object& object) {
    // Возвращаем невладеющий shared_ptr (его deleter ничего не делает)
    return ObjectHolder(std::shared_ptr<Object>(&object, [](auto* /*p*/) { /* do nothing */ }));
}

ObjectHolder ObjectHolder::None() {
    return ObjectHolder();
}

Object& ObjectHolder::operator*() const {
    AssertIsValid();
    return *Get();
}

Object* ObjectHolder::operator->() const {
    AssertIsValid();
    return Get();
}

Object* ObjectHolder::Get() const {
    return data_.get();
}

ObjectHolder::operator bool() const {
    return Get() != nullptr;
}

bool IsTrue(const ObjectHolder& object) {
    if (!object) {
        return false;
    }

    if(object.TryAs<Bool>() != nullptr) {
        return object.TryAs<Bool>()->GetValue();
    }
    if (object.TryAs<Number>() != nullptr) {
        return object.TryAs<Number>()->GetValue() != 0;
    }
    if (object.TryAs<String>() != nullptr) {
        return !object.TryAs<String>()->GetValue().empty();
    }

    return false;
}

void ClassInstance::Print(std::ostream& os, Context& context) {
    if (HasMethod(STR_METHOD, 0)) {
        Call(STR_METHOD, {}, context)->Print(os, context);
    }
    else {
        os << this;
    }
}

bool ClassInstance::HasMethod(const std::string& method, size_t argument_count) const {
    const Method* mth = cls_.GetMethod(method);
    return mth != nullptr && mth->formal_params.size() == argument_count;
}

Closure& ClassInstance::Fields() {
    return closure_;
}

const Closure& ClassInstance::Fields() const {
    return closure_;
}

ClassInstance::ClassInstance(const Class& cls)
    : cls_(cls)
{ /* do nothing */ }

ObjectHolder ClassInstance::Call(const std::string& method,
                                 const std::vector<ObjectHolder>& actual_args,
                                 Context& context)
{
    if (!HasMethod(method, actual_args.size())) {
        throw std::runtime_error("Method "s + method + " wasn't found in class "s + cls_.GetName());
    }

    const Method* method_ptr = cls_.GetMethod(method);
    Closure method_vars;
    method_vars["self"s] = ObjectHolder::Share(*this);
    for (size_t i = 0; i < method_ptr->formal_params.size(); ++i) {
        method_vars[method_ptr->formal_params.at(i)] = actual_args[i];
    }

    return method_ptr->body->Execute(method_vars, context);
}

Class::Class(std::string name, std::vector<Method> methods, const Class* parent)
    : name_(name)
    , parent_(parent)
{
    for (Method& method : methods) {
        names_to_methods_[method.name] = std::move(method);
    }
}

const Method* Class::GetMethod(const std::string& name) const {
    if (names_to_methods_.find(name) != names_to_methods_.end()) {
        return &names_to_methods_.at(name);
    }
    else if (parent_ != nullptr) {
        return parent_->GetMethod(name);
    }

    return nullptr;
}

[[nodiscard]] /*inline*/ const std::string& Class::GetName() const {
    return name_;
}

void Class::Print(ostream& os, Context& /*context*/) {
    os << "Class "sv << name_;
}

void Bool::Print(std::ostream& os, [[maybe_unused]] Context& context) {
    os << (GetValue() ? "True"sv : "False"sv);
}

bool Equal(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    if (lhs.TryAs<Bool>() && rhs.TryAs<Bool>()) {
        return lhs.TryAs<Bool>()->GetValue() == rhs.TryAs<Bool>()->GetValue();
    }
    if (lhs.TryAs<Number>() && rhs.TryAs<Number>()) {
        return lhs.TryAs<Number>()->GetValue() == rhs.TryAs<Number>()->GetValue();
    }
    if (lhs.TryAs<String>() && rhs.TryAs<String>()) {
        return lhs.TryAs<String>()->GetValue() == rhs.TryAs<String>()->GetValue();
    }
    if (lhs.TryAs<ClassInstance>() && rhs.TryAs<ClassInstance>()) {
        return lhs.TryAs<ClassInstance>()->Call(EQ_METHOD, { rhs }, context).TryAs<Bool>()->GetValue();
    }
    if (!lhs && !rhs) {
        return true;
    }

    throw std::runtime_error("Trying to compare wrong object types"s);
}

bool Less(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    if (lhs.TryAs<Bool>() && rhs.TryAs<Bool>()) {
        return lhs.TryAs<Bool>()->GetValue() < rhs.TryAs<Bool>()->GetValue();
    }
    if (lhs.TryAs<Number>() && rhs.TryAs<Number>()) {
        return lhs.TryAs<Number>()->GetValue() < rhs.TryAs<Number>()->GetValue();
    }
    if (lhs.TryAs<String>() && rhs.TryAs<String>()) {
        return lhs.TryAs<String>()->GetValue() < rhs.TryAs<String>()->GetValue();
    }
    if (lhs.TryAs<ClassInstance>() && lhs.TryAs<ClassInstance>()->HasMethod(LT_METHOD, 1)) {
        return lhs.TryAs<ClassInstance>()->Call(LT_METHOD, { rhs }, context).TryAs<Bool>()->GetValue();
    }

    throw std::runtime_error("Trying to compare wrong object types"s);
}

bool NotEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !Equal(lhs, rhs, context);
}

bool Greater(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !Less(lhs, rhs, context) && NotEqual(lhs, rhs, context);
}

bool LessOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !Greater(lhs, rhs, context);
}

bool GreaterOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !Less(lhs, rhs, context);
}

}  // namespace runtime

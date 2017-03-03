#pragma once

namespace LuaCpp {

class TypeError : public std::exception {
public:
    explicit TypeError(std::string expected) : _message(std::move(expected) + " expected, got no object.") {}
    explicit TypeError(std::string expected, const std::string &actual) : _message(std::move(expected) + " expected, got " + actual + '.') {}
    
    const char *what() const noexcept override {
        return _message.c_str();
    }

private:
    std::string _message;
};

class CopyUnregisteredType : public std::exception {
public:
    using TypeID = std::reference_wrapper<const std::type_info>;

    explicit CopyUnregisteredType(TypeID type) : _type(type) {}

    TypeID getType() const { return _type; }

    const char *what() const noexcept override {
        return "Tried to copy an object of an unregistered type. "
               "Please register classes before passing instances by value.";
    }

private:
    TypeID _type;
};

}

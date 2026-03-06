#include "numgeom/outcome.h"

#include <cassert>
#include <iostream>

Outcome Outcome::Success() {
    return Outcome(rtSuccess, "");
}

Outcome Outcome::Warning(const std::string& message) {
  return Outcome(rtWarning, message);
}

Outcome Outcome::Error(const std::string& message) {
  return Outcome(rtError, message);
}

Outcome Outcome::UserBreak() {
  return Outcome(rtUserBreak, "");
}

Outcome::Outcome() {
  ret_code_ = rtUndefined;
}

bool Outcome::operator!() const {
  assert(ret_code_ != rtUndefined);
  return ret_code_ != rtSuccess;
}

const std::string& Outcome::message() const {
  assert(ret_code_ != rtUndefined);
  return message_;
}

bool Outcome::IsUserBreak() const {
  return ret_code_ == rtUserBreak;
}

Outcome::Outcome(RetCodeType rc, const std::string& message)
    : ret_code_(rc), message_(message) {}

Outcome::Outcome(const char* errorMessage)
    : ret_code_(rtError), message_(errorMessage) {}

Outcome::Outcome(const std::string& errorMessage)
    : ret_code_(rtError), message_(errorMessage) {}

std::ostream& operator<<(std::ostream& s, const Outcome& rc) {
  if (!rc)
    s << rc.message() << '.' << std::endl;
  else
    s << "Sucess." << std::endl;
  return s;
}

bool Outcome::operator==(const Outcome& other) const {
  assert(ret_code_ != rtUndefined);
  assert(other.ret_code_ != rtUndefined);
  return ret_code_ == other.ret_code_;
}

bool Outcome::operator!=(const Outcome& other) const {
  return !((*this) == other);
}

Outcome& Outcome::operator<<(const char* message) {
  if (ret_code_ == rtUndefined)
    ret_code_ = rtError;
  message_ += message;
  return (*this);
}

Outcome& Outcome::operator<<(const std::string& message) {
  if (ret_code_ == rtUndefined)
    ret_code_ = rtError;
  message_ += message;
  return (*this);
}

Outcome& Outcome::operator<<(const Outcome& other) {
  if (ret_code_ == rtUndefined)
    ret_code_ = rtError;
  message_ += other.message();
  return (*this);
}

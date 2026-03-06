#ifndef numgeom_core_outcome_h
#define numgeom_core_outcome_h

#include <iosfwd>
#include <string>

#include "numgeom/numgeomcore_export.h"

class Outcome {
 public:
  static Outcome Success();
  static Outcome Warning(const std::string& = std::string());
  static Outcome Error(const std::string& = std::string());
  static Outcome UserBreak();

 public:
  Outcome();

  Outcome(const char* errorMessage);

  Outcome(const std::string& errorMessage);

  bool operator!() const;

  const std::string& message() const;

  bool IsUserBreak() const;

  bool operator==(const Outcome&) const;
  bool operator!=(const Outcome&) const;

  Outcome& operator<<(const char*);
  Outcome& operator<<(const std::string&);
  Outcome& operator<<(const Outcome&);

 private:
  enum RetCodeType { rtSuccess, rtWarning, rtError, rtUserBreak, rtUndefined };

  Outcome(RetCodeType success, const std::string& message);

 private:
  RetCodeType ret_code_;
  std::string message_;
};

std::ostream& operator<<(std::ostream&, const Outcome&);

#endif  // !numgeom_core_outcome_h

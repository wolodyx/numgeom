#ifndef numgeom_core_outcome_h
#define numgeom_core_outcome_h

#include <string>
#include <iosfwd>

#include "numgeom/numgeomcore_export.h"


class Outcome
{
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
    enum RetType
    {
        rtSuccess,
        rtWarning,
        rtError,
        rtUserBreak,
        rtUndefined
    };

    Outcome(RetType success, const std::string& message);

private:
    RetType m_rc;
    std::string m_message;
};

std::ostream& operator<<(std::ostream&, const Outcome&);

#endif // !numgeom_core_outcome_h

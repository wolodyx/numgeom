#include "numgeom/outcome.h"


#include <cassert>
#include <iostream>


Outcome Outcome::Success()
{
    return Outcome(rtSuccess, "");
}


Outcome Outcome::Warning(const std::string& message)
{
    return Outcome(rtWarning, message);
}


Outcome Outcome::Error(const std::string& message)
{
    return Outcome(rtError, message);
}


Outcome Outcome::UserBreak()
{
    return Outcome(rtUserBreak, "");
}


Outcome::Outcome()
{
    m_rc = rtUndefined;
}


bool Outcome::operator!() const
{
    assert(m_rc != rtUndefined);
    return m_rc != rtSuccess;
}


const std::string& Outcome::message() const
{
    assert(m_rc != rtUndefined);
    return m_message;
}


bool Outcome::IsUserBreak() const
{
    return m_rc == rtUserBreak;
}


Outcome::Outcome(RetType rc, const std::string& message)
: m_rc(rc), m_message(message)
{
}


Outcome::Outcome(const char* errorMessage)
: m_rc(rtError), m_message(errorMessage)
{
}


Outcome::Outcome(const std::string& errorMessage)
: m_rc(rtError), m_message(errorMessage)
{
}


std::ostream& operator<<(std::ostream& s, const Outcome& rc)
{
    if(!rc)
        s << rc.message() << '.' << std::endl;
    else
        s << "Sucess." << std::endl;
    return s;
}


bool Outcome::operator==(const Outcome& other) const
{
    assert(m_rc != rtUndefined);
    assert(other.m_rc != rtUndefined);
    return m_rc == other.m_rc;
}


bool Outcome::operator!=(const Outcome& other) const
{
    return !((*this) == other);
}


Outcome& Outcome::operator<<(const char* message)
{
    if(m_rc == rtUndefined)
        m_rc = rtError;
    m_message += message;
    return (*this);
}


Outcome& Outcome::operator<<(const std::string& message)
{
    if(m_rc == rtUndefined)
        m_rc = rtError;
    m_message += message;
    return (*this);
}


Outcome& Outcome::operator<<(const Outcome& other)
{
    if(m_rc == rtUndefined)
        m_rc = rtError;
    m_message += other.message();
    return (*this);
}

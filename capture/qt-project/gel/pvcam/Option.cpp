#include "Option.h"

/* System */
#include <sstream>

/* Local */
#include "Log.h"

const std::string pm::Option::ArgValueSeparator = "=";
const char pm::Option::ValuesSeparator = ',';
const char pm::Option::ValueGroupsSeparator = ';';

pm::Option::Option(const std::vector<std::string>& names,
            const std::vector<std::string>& argsDescs,
            const std::vector<std::string>& defVals,
            const std::string& desc, const Handler& handler)
    : m_names(names),
    m_argsDescs(argsDescs),
    m_defVals(defVals),
    m_desc(desc),
    m_handler(handler)
{
}

bool pm::Option::IsMatching(const std::string& nameWithValue) const
{
    const std::string::size_type argValSepPos =
        nameWithValue.find(ArgValueSeparator);
    for (const std::string& name : m_names)
        // Whole name (without value) has to match
        if (name == nameWithValue.substr(0, argValSepPos))
            // ArgValueSeparator has to follow the name if option expects at
            // least one value
            return (m_argsDescs.empty() || argValSepPos != std::string::npos);
    return false;
}

bool pm::Option::RunHandler(const std::string& nameWithValue) const
{
    if (!m_handler)
        return false;

    const std::string::size_type argValSepPos =
        nameWithValue.find(ArgValueSeparator);
    const std::string name = nameWithValue.substr(0, argValSepPos);
    const std::string value = (argValSepPos == std::string::npos)
        ? "" : nameWithValue.substr(argValSepPos + 1);

    const bool ok = m_handler(value);
    std::ostringstream ss;
    ss << "Handler for option " << name << " was called with value '" << value
        << "' - ";
    if (ok)
    {
        ss << "OK";
        Log::LogI(ss.str());
    }
    else
    {
        ss << "ERROR";
        Log::LogE(ss.str());
    }

    return ok;
}

bool operator==(const pm::Option& lhs, const pm::Option& rhs)
{
    if (lhs.GetNames().size() != rhs.GetNames().size())
        return false;
    for (size_t n = 0; n < lhs.GetNames().size(); n++)
        if (lhs.GetNames()[n] != rhs.GetNames()[n])
            return false;
    return true;
}

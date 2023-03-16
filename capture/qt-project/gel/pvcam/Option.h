#pragma once
#ifndef _OPTION_H
#define _OPTION_H

/* System */
#include <functional>
#include <vector>
#include <string>

namespace pm {

class Option
{
public:
    using HandlerProto = bool(const std::string&);
    using Handler = std::function<HandlerProto>;

public:
    static const std::string ArgValueSeparator;
    static const char ValuesSeparator;
    static const char ValueGroupsSeparator;

public:
    // Size of argsDescs and defVals have to be equal
    Option(const std::vector<std::string>& names,
            const std::vector<std::string>& argsDescs,
            const std::vector<std::string>& defVals,
            const std::string& desc, const Handler& handler);
    ~Option()
    {}

    Option() = delete;

    // Used for printing the usage of the option object
    const std::vector<std::string>& GetNames() const
    { return m_names; }

    // Used for printing the usage of the option object
    const std::vector<std::string>& GetArgsDescriptions() const
    { return m_argsDescs; }

    // Used for printing the usage of the option object
    const std::vector<std::string>& GetDefaultValues() const
    { return m_defVals; }

    // Used for printing the usage of the option object
    const std::string& GetDescription() const
    { return m_desc; }

    // Determine if runtime option starts with option name
    bool IsMatching(const std::string& nameWithValue) const;

    // Execute the function pointer associated with the option
    bool RunHandler(const std::string& nameWithValue) const;

private:
    std::vector<std::string> m_names;
    std::vector<std::string> m_argsDescs;
    std::vector<std::string> m_defVals;
    std::string m_desc;
    Handler m_handler;
};

} // namespace pm

// Compared two options, names have to be the same in exact order
bool operator==(const pm::Option& lhs, const pm::Option& rhs);

#endif /* _OPTION_H */

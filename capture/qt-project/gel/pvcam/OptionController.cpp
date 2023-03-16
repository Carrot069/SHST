#include "OptionController.h"

/* Local */
#include "Log.h"

pm::OptionController::OptionController()
    : m_options()
{
}

bool pm::OptionController::AddOption(const Option& option)
{
    if (option.GetNames().empty())
    {
        Log::LogE("Cannot add option, no name given");
        return false;
    }
    if (option.GetArgsDescriptions().size() != option.GetDefaultValues().size())
    {
        Log::LogE("Number of arguments and default values don't match");
        return false;
    }
    m_options.push_back(option);
    std::string conflictingName;
    if (!VerifyOptions(conflictingName))
    {
        m_options.pop_back();
        Log::LogE("Cannot add option, conflict on '%s' detected",
                conflictingName.c_str());
        return false;
    }
    return true;
}

bool pm::OptionController::ProcessOptions(int argc, char* argv[]) const
{
    bool ok = true;

    for (int argvIdx = 1; argvIdx < argc; argvIdx++)
    {
        bool found = false;
        for (const Option& option : m_options)
        {
            if (option.IsMatching(argv[argvIdx]))
            {
                if (!option.RunHandler(argv[argvIdx]))
                    ok = false;
                found = true;
                break;
            }
        }
        if (!found)
        {
            Log::LogE("Unknown option discovered in input: '%s'", argv[argvIdx]);
            ok = false;
            break;
        }
    }

    if (!ok)
    {
        Log::LogE("A parameter was incorrect, please review results");
    }

    return ok;
}

std::string pm::OptionController::GetOptionsDescription() const
{
    std::string usageDesc;
    usageDesc  = "Options\n";
    usageDesc += "-------\n";
    for (const Option& option : m_options)
    {
        usageDesc += "\n";

        // Format option's arguments
        std::string args;
        const std::vector<std::string>& argsDescs = option.GetArgsDescriptions();
        if (!argsDescs.empty())
        {
            args = Option::ArgValueSeparator;
            for (const std::string& argsDesc : argsDescs)
                args += "<" + argsDesc + ">" + Option::ValuesSeparator;
            args.pop_back(); // Removes last separator
        }

        // Adjust indentation for multi-line description
        std::string desc = option.GetDescription();
        size_t pos = (size_t)-1;
        while ((pos = desc.find_first_of("\n", pos + 1)) != std::string::npos)
            desc.insert(pos + 1, "    ");

        // All line with default value
        const std::vector<std::string>& defVals = option.GetDefaultValues();
        if (!defVals.empty())
        {
            desc += "\n    Default value is '";
            for (const std::string& defVal : defVals)
                desc += defVal + Option::ValuesSeparator;
            desc.pop_back(); // Remove last separator
            desc += "'.";
        }

        // Collect complete description
        const std::vector<std::string>& names = option.GetNames();
        for (const std::string& name : names)
            usageDesc += "  " + name + args + "\n";
        usageDesc += "    " + desc + "\n";
    }
    return usageDesc;
}

bool pm::OptionController::VerifyOptions(std::string& conflictingName) const
{
    for (size_t optIdx1 = 0; optIdx1 < m_options.size(); optIdx1++)
    {
        const Option& opt1 = m_options[optIdx1];
        for (size_t optIdx2 = optIdx1 + 1; optIdx2 < m_options.size(); optIdx2++)
        {
            const Option& opt2 = m_options[optIdx2];
            for (const std::string& name1 : opt1.GetNames())
                for (const std::string& name2 : opt2.GetNames())
                    if (name1 == name2)
                    {
                        conflictingName = name1;
                        return false;
                    }
        }
    }
    return true;
}

#pragma once
#ifndef _UTILS_H
#define _UTILS_H

/* System */
#include <limits>
#include <string>
#include <vector>

/* Helper macro that makes multi-line function-like macros more safe
   Usage example:
    #define MULTI_LINE_FUNC_LIKE_MACRO(a,b,c) do {\
        <code>
    } ONCE
*/
#if defined(_WIN32) || defined(_WIN64)
  #define ONCE\
    __pragma(warning(push))\
    __pragma(warning(disable:4127))\
    while (0)\
    __pragma(warning(pop))
#else
  #define ONCE while (0)
#endif

/* Platform independent macro to avoid compiler warnings for unreferenced
   function parameters. It has the same effect as Win32 UNREFERENCED_PARAMETER
   Usage example:
       void do_magic(int count)
       {
           UNUSED(count);
       }
*/
#define UNUSED(expr) do { (void)(expr); } ONCE

namespace pm {

// Converts string to integral number of given type
template<typename T>
bool StrToNumber(const std::string& str,
        typename std::enable_if<
            std::is_integral<T>::value && std::is_signed<T>::value,
            T>::type& number)
{
    try {
        size_t idx;
        const long long nr = std::stoll(str, &idx);
        if (idx == str.length()
                && nr >= (long long)(std::numeric_limits<T>::min)()
                && nr <= (long long)(std::numeric_limits<T>::max)())
        {
            number = (T)nr;
            return true;
        }
    }
    catch(...) {};
    return false;
}

// Converts string to integral number of given type
template<typename T>
bool StrToNumber(const std::string& str,
        typename std::enable_if<
            std::is_integral<T>::value && std::is_unsigned<T>::value,
            T>::type& number)
{
    try {
        size_t idx;
        const unsigned long long nr = std::stoull(str, &idx);
        if (idx == str.length()
                && nr >= (unsigned long long)(std::numeric_limits<T>::min)()
                && nr <= (unsigned long long)(std::numeric_limits<T>::max)())
        {
            number = (T)nr;
            return true;
        }
    }
    catch(...) {};
    return false;
}

// Splits string into sub-strings separated by given delimiter
std::vector<std::string> SplitString(const std::string& string, char delimiter);

// Joins strings from vector into one string using given delimiter
std::string JoinStrings(const std::vector<std::string>& strings, char delimiter);

// Type size_t limits size of total memory to 4096TB
size_t GetTotalRamMB();

// Type size_t limits size of available memory to 4096TB
size_t GetAvailRamMB();

// Return a list of file names in given folder with given extension
std::vector<std::string> GetFiles(const std::string& dir, const std::string& ext);

} // namespace pm

#endif /* _UTILS_H */

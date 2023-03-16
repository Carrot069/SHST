#include "Utils.h"

/* System */
#include <sstream>

#if defined(_WIN32)
    #include <Windows.h>
#elif defined(__linux__)
    #include <dirent.h>
    #include <unistd.h>
    #include <sys/stat.h>
    #include <sys/types.h>
#else
    #error Unsupported OS
#endif

std::vector<std::string> pm::SplitString(const std::string& string, char delimiter)
{
    std::vector<std::string> items;
    std::istringstream ss(string);
    std::string item;
    while (std::getline(ss, item, delimiter))
        items.push_back(item);
    return items;
}

std::string pm::JoinStrings(const std::vector<std::string>& strings, char delimiter)
{
    std::string string;
    for (const std::string& item : strings)
        string += item + delimiter;
    if (!strings.empty())
        string.pop_back(); // Remove delimiter at the end
    return string;
}

size_t pm::GetTotalRamMB()
{
#if defined(_WIN32)

    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    // Right shift by 20 bytes converts bytes to megabytes
    return (size_t)(status.ullTotalPhys >> 20);

#elif defined(__linux__)

    return sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE) >> 20;

#endif
}

size_t pm::GetAvailRamMB()
{
#if defined(_WIN32)

    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    // Right shift by 20 bytes converts bytes to megabytes
    return (size_t)(status.ullAvailPhys >> 20);

#elif defined(__linux__)

    return sysconf(_SC_AVPHYS_PAGES) * sysconf(_SC_PAGESIZE) >> 20;

#endif
}

std::vector<std::string> pm::GetFiles(const std::string& dir, const std::string& ext)
{
    std::vector<std::string> files;

#if defined(_WIN32)

    WIN32_FIND_DATAA fdFile;
    const std::string filter = dir + "/*" + ext;
    HANDLE hFind = FindFirstFileA(filter.c_str(), &fdFile);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            const std::string fullFileName = dir + "/" + fdFile.cFileName;
            /* Ignore folders */
            if ((fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                continue;
            /* Check file name ends with extension */
            const size_t extPos = fullFileName.rfind(ext);
            if (extPos == std::string::npos
                || extPos + ext.length() != fullFileName.length())
                continue;
            files.push_back(fullFileName);
        }
        while (FindNextFileA(hFind, &fdFile));

        FindClose(hFind);
    }

#elif defined(__linux__)

    DIR* dr = opendir(dir.c_str());
    if (dr)
    {
        struct dirent* ent;
        struct stat st;
        while ((ent = readdir(dr)) != NULL)
        {
            const std::string fileName = ent->d_name;
            const std::string fullFileName = dir + "/" + fileName;
            /* Ignore folders */
            if (stat(fullFileName.c_str(), &st) != 0)
                continue;
            if (st.st_mode & S_IFDIR)
                continue;
            /* Check file extension */
            const size_t extPos = fileName.rfind(ext);
            if (extPos == std::string::npos
                    || extPos + ext.length() != fileName.length())
                continue;
            files.push_back(fullFileName);
        }
        closedir(dr);
    }

#endif

    return files;
}

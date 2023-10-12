#pragma once
#ifndef OS_UTILITY_H
#define OS_UTILITY_H
#include <vector>

class Utility {
public:
    template<class T>
    static void copy(T* from, T* to, int count)
    {
        while (count--)
            *to++ = *from++;
    }
    // static void copy(T* from, T* to, int count)
    // {
    //     std::memcpy(to, from, count * sizeof(T));
    // }

    static void StringCopy(char* src, char* dst)
    {
        while ((*dst++ = *src++) != 0);
    }
    // static void StringCopy(const char* src, char* dst)
    // {
    //     std::strcpy(dst, src);
    // }
    
    static int strlen(char* pString)
    {
        int length = 0;
        char* pChar = pString;
        while (*pChar++)
            length++;
        return length;
    }
    // static int strlen(const char* pString)
    // {
    //     return std::strlen(pString);
    // }

    static vector<char*> parseCmd(char* s)
    {
        char* p = s, *q = s;
        vector<char*> result;
        while (*q != '\0')
        {
            if (*p == ' ') p++, q++;
            else
            {
                while (*q != '\0' && *q != ' ') q++;
                char* newString = new char[q - p + 1];
                for (int i = 0; i < q - p; i++) newString[i] = *(p + i);
                newString[q - p] = '\0';
                result.push_back(newString);
                p = q;
            }
        }
        return result;
    }
    // static std::vector<std::string> parseCmd(const char* s)
    // {
    //     std::vector<std::string> result;
    //     while (*s != '\0')
    //     {
    //         while (*s == ' ')
    //             s++;
    //         if (*s == '\0')
    //             break;
    //         const char* p = s;
    //         while (*s != '\0' && *s != ' ')
    //             s++;
    //         result.emplace_back(p, s - p);
    //     }
    //     return result;
    // }

    static void tool_print_surface()
    {
        string SiyuOS =
                "███████╗██╗██╗   ██╗██╗   ██╗ ██████╗ ███████╗             \n"
                "██╔════╝██║╚██╗ ██╔╝██║   ██║██╔═══██╗██╔════╝             \n"
                "███████╗██║ ╚████╔╝ ██║   ██║██║   ██║███████╗             Hi !\n"
                "╚════██║██║  ╚██╔╝  ██║   ██║██║   ██║╚════██║             \n"
                "███████║██║   ██║   ╚██████╔╝╚██████╔╝███████║             by 2054435 廖偲宇\n"
                "╚══════╝╚═╝   ╚═╝    ╚═════╝  ╚═════╝ ╚══════╝             \n"
                "\n";

        cout << endl << SiyuOS << endl;

        return;
    }
};

#endif





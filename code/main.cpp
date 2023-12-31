#include <sstream>

#include "_Kernel.h"
#include "Utility.h"

int main()
{
    Utility::tool_print_surface();

    _Kernel* k = _Kernel::getInstance();
    k->initialize();

    while (true)
    {
        if (strcmp(k->curdir, "/") == 0)
            cout << "siyu@MacBook % ";
        else
            cout << "siyu@MacBook " << k->curdir+1 << " % ";

        const int bufferSize = 1024; // 缓冲区大小
        char input[bufferSize];
        cin.getline(input, bufferSize);

        vector<char*> result = Utility::parseCmd(input);

        if (result.size() > 0)
        {
            if (strcmp(result[0], "cd") == 0)
            {
                if (result.size() > 1){
                    k->cd(result[1]);
                    if (k->error == _Kernel::NOTDIR)
                        cout << "cd: not a directory: " << result[1] << endl;
                    else if (k->error == _Kernel::NOENT)
                        cout<< "cd: no such file or directory: " << result[1] << endl;
                }
                else{
                    k->cd("/");
                }
            }
            else if (strcmp(result[0], "fformat") == 0)
            {
                cout<<"This command is used to format disk. Please use with caution!"<<endl;
                cout<<"Enter \'fformat\' again to confirm the format operation:";

                string tmp;
                cin>>tmp;
                if(tmp=="fformat"){
                    k->fformat();
                    k->initialize();
                    cout<<"FORMAT SUCCESS!"<<endl;
                }
                else{
                    cout<<"Exiting the fformat command."<<endl;
                }
            }
            else if (strcmp(result[0], "mkdir") == 0)
            {
                if (result.size() > 1)
                {
                    k->mkdir(result[1]);
                    if (k->error == _Kernel::ISDIR)
                        cout << "mkdir: " << result[1] << ": File exists" << endl;
                }
                else{
                    cout << "mkdir: missing operand" << endl;
                    cout << "usage: mkdir [directory_name]" << endl;
                }
            }
            else if (strcmp(result[0], "ls") == 0)
            {
                k->ls();
            }
            else if (strcmp(result[0], "fopen") == 0)
            {
                int fd;
                if (result.size() > 1)
                {
                    fd = k->fopen(result[1], 511);
                    if (k->error == _Kernel::NO_ERROR)
                        cout << "The file " << result[1] << " is opened, fd = " << fd << endl;
                    else if (k->error == _Kernel::ISDIR)
                        cout << "fopen: " << result[1] << ": Is a directory" << endl;
                    else if (k->error == _Kernel::NOENT)
                        cout << "The file " << result[1] << " does not exist." << endl;
                }
                else{
                    cout << "fopen: missing operand" << endl;
                    cout << "Usage: fopen [filename]" << endl;
                }

            }
            else if (strcmp(result[0], "fcreat") == 0)
            {
                int fd;
                if (result.size() > 1)
                {
                    fd = k->fcreat(result[1], 511);

                    if (k->error == _Kernel::ISDIR)
                        cout << "fcreat: " << result[1] << ": Is a directory" << endl;
                    if (k->error == _Kernel::NO_ERROR)
                        cout << "The file " << result[1] << " is created, fd = " << fd << endl;
                }
                else{
                    cout << "fcreat: missing operand" << endl;
                    cout << "Usage: fcreat [filename]" << endl;
                }

            }
            else if (strcmp(result[0], "fclose") == 0)
            {
                if (result.size() > 1)
                    k->fclose(atoi(result[1]));
                else{
                    cout << "fclose: missing operand" << endl;
                    cout << "Usage: fclose [fd]" << endl;
                }
            }
            else if (strcmp(result[0], "fread") == 0)
            {
                int actual;
                if (result.size() > 2) {
                    char* buf;
                    buf = new char[atoi(result[2])];
                    buf[0] = '\0';
                    actual = k->fread(atoi(result[1]), buf, atoi(result[2]));
                    if (actual == -1)
                    {
                        if (k->error == _Kernel::BADF)
                            cout << "fread: " << atoi(result[1]) << ": Wrong fd" << endl;
                    }
                    else
                    {
                        if (actual > 0)
                        {
                            for (int i = 0; i < actual; i++)
                                printf("%c", buf[i]);
                            cout << endl;
                        }
                        cout << "fread: " << "Actually read " << actual << " bytes." << endl;
                    }
                    delete buf;
                }
                else{
                    cout << "fread: missing operand" << endl;
                    cout << "Usage: fread [fd][nbytes]" << endl;
                }
            }
            else if (strcmp(result[0], "fwrite") == 0)
            {
                int actual;
                if (result.size() > 3)
                {
                    if (atoi(result[2]) > strlen(result[3]))
                    {
                        cout << "nbytes can\'t be larger than the length of the string" << endl;
                        continue;
                    }
                    actual = k->fwrite(atoi(result[1]), result[3], atoi(result[2]));
                    if (actual == -1)
                    {
                        if (k->error == _Kernel::BADF)
                            cout << "fwrite: " << atoi(result[1]) << ": Wrong fd" << endl;
                    }
                    else
                    {
                        cout << "fwrite: " << "Actually write " << actual << " bytes." << endl;
                    }
                }
                else
                {
                    cout << "fwrite: missing operand" << endl;
                    cout << "Usage: fwrite [fd][nbytes]" << endl;
                }
            }
            else if (strcmp(result[0], "fseek") == 0)
            {
                if (result.size() > 3)
                {
                    if (atoi(result[3]) >= 0 && atoi(result[3]) <= 5)
                    {
                        k->fseek(atoi(result[1]), atoi(result[2]), atoi(result[3]));
                        if (k->error == _Kernel::BADF)
                            cout << "fseek: " << atoi(result[1]) << ": Wrong fd." << endl;
                    }
                    else
                        cout << "fseek: " << result[3] << ": Wrong ptrname." << endl;
                }
                else
                {
                    cout << "fseek: missing operand" << endl;
                    cout << "Usage: fseek [fd][offset][ptrname]" << endl;
                }
            }
            else if (strcmp(result[0], "fdelete") == 0)
            {
                if (result.size() > 1)
                {
                    k->fdelete(result[1]);
                    if (k->error == _Kernel::NOENT)
                        cout << "fdelete: " << result[1] << ": No such file or directory" << endl;
                }
                else
                {
                    cout << "fdelete: missing operand" << endl;
                    cout << "Usage: fdelete [filename]" << endl;
                }
            }
            else if (strcmp(result[0], "fmount") == 0)
            {
                if (result.size() > 2)
                {
                    k->fmount(result[1], result[2]);
                    if (k->error == _Kernel::NOENT)
                        cout << result[2] << ": No such file or directory" << endl;
                    else if (k->error == _Kernel::NOOUTENT)
                        cout << result[1] << ": No such file or directory" << endl;
                    else if (k->error == _Kernel::ISDIR)
                        cout << result[2] << ": Is a directory" << endl;
                }
                else
                {
                    cout << "fmount: missing operand" << endl;
                }
            }
            else if (strcmp(result[0], "exit") == 0)
            {
                cout << endl << "Saving session..." << endl;
                cout << "...copying shared history..." << endl;
                cout << "...saving history...truncating history files..." << endl;
                cout << "...completed." << endl << endl;
                cout << "Goodbye! Have a great day!" << endl << endl;
                cout << "[Process completed]" << endl;
                k->clear();
                break;
            }
            else {
                cout<< "zsh: command not found: " << result[0] << endl;
            }
        }
        cout << endl;
    }
    return 0;
}
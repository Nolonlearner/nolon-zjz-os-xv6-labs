#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char *argv[])
{
    int argc2 = argc - 1;
    char *argv2[MAXARG];
    for (int i = 1; i < argc; i++)
    {
        argv2[i - 1] = argv[i];
    }
    char s[64];
    int c = 0, flag = 0;
    while (read(0, &s[c++], sizeof(char)))
    {
        if (s[c - 1] != ' ' && (flag = s[c - 1] != '\n'))
            continue;
        s[c - 1] = '\0';
        argv2[argc2] = (char *)malloc(c);
        strcpy(argv2[argc2++], s);
        c = 0;
        if (flag)
            continue;
        argv2[argc2] = 0;
        if (fork() == 0)
        {
            exec(argv2[0], argv2);
            exit(0);
        }
        else
            wait(0);
        argc2 = argc - 1;
    }
    exit(0);
}

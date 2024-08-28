#include "user/user.h"

void prime(int pi[])
{
    int n, p;
    if (!read(pi[0], &p, sizeof(int)))
        return;
    printf("prime %d\n", p);
    int new_pi[2];
    pipe(new_pi);
    if (fork())
    {
        while (read(pi[0], &n, sizeof(int)))
        {
            if (n % p)
                write(new_pi[1], &n, sizeof(int));
        }
        close(new_pi[1]);
        wait(0);
    }
    else
    {
        close(new_pi[1]);
        prime(new_pi);
        exit(0);
    }
    exit(0);
}

int main(int argc, char *argv[])
{
    int pi[2];
    pipe(pi);
    if (fork())
    {
        for (int i = 2; i <= 35; i++)
        {
            write(pi[1], &i, sizeof(int));
        }
        close(pi[1]);
        wait(0);
    }
    else
    {
        close(pi[1]);
        prime(pi);
        exit(0);
    }
    exit(0);
}

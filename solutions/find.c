#include "user/user.h"
#include "kernel/stat.h"
#include "kernel/fs.h"

void find(char *path, char *filename)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if ((fd = open(path, 0)) < 0)
    {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }
    if (fstat(fd, &st) < 0)
    {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }
    if (st.type != T_DIR)
    {
        fprintf(2, "find: %s is not a directory\n", path);
        close(fd);
        return;
    }
    if (strlen(path) + 1 > sizeof buf)
    {
        printf("find: path too long\n");
        close(fd);
        return;
    }
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';
    while (read(fd, &de, sizeof(de)) == sizeof(de))
    {
        if (de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
            continue;
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        if (stat(buf, &st) < 0)
        {
            printf("find: cannot stat %s\n", buf);
            continue;
        }
        if (st.type == T_DIR)
        {
            find(buf, filename);
        }
        else if (st.type == T_FILE && strcmp(de.name, filename) == 0)
        {
            printf("%s\n", buf);
        }
    }
    close(fd);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(2, "usage: find path filename...\n");
        exit(1);
    }
    find(argv[1], argv[2]);
    exit(0);
}

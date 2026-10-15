#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        fprintf(2, "usage: sleep <sec>\n");
    }
    int sec = atoi(argv[1]);
    pause(sec);
    return 0;
}
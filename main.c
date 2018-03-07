#include <stdlib.h>
#include "cmdline.h"
#include "echoserver.h"

int main(int argc, char **argv)
{
    struct gengetopt_args_info ai;
    if (cmdline_parser(argc, argv, &ai) != 0) {
        exit(1);
    }



    return 0;
}


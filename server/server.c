#include "server.h"

int main(int argc, char* argv[])
{
    Config* configuration = malloc(sizeof(*configuration));

    initializeConfig(configuration, "./config/server.config");

    return 0;
}
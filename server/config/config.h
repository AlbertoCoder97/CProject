#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ROOT_LEN    128

#define MAX_COMMANDS    8
#define MAX_CMDS_LEN    8

//For simplicity purposes I need to assume that I won't be reading lines longer than 128 bytes.
#define MAX_LINE_READ   128


typedef struct {
    int port;
    char root[MAX_ROOT_LEN];
    char commands[MAX_COMMANDS][MAX_CMDS_LEN];
} Config;


int initializeConfig(Config* configuration, char* configFilePath);
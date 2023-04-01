#pragma once
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <limits.h>

#include "config/config.h"
#include "StringUtils/StringUtils.h"
#include "FileUtils/FileUtils.h"

#define IP_ADDRESS              "127.0.0.1"
#define MAX_CLIENTCMDS_LENGTH   1024
#define MAX_CONNECTIONS         5
#define MAX_PIPE_LINE           1024
#define DIR_PERMISSIONS         0700

int setChildIDs();

int executeCommandType1(int clientSocket, char* command);
int executeCommandType2(int clientSocket, char* command);
int executeCommandType3(int clientSocket, char* command);
int executeCommandType4(int clientSocket, char* command);
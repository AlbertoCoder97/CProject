
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#include "FileUtils/FileUtils.h"
#include "StringUtils/StringUtils.h"

//#define SERVER_IP "127.0.0.1"   // Change this to the IP address of your server
//#define SERVER_PORT 45000        // Change this to the port number used by your server
#define LINE_SIZE 1024        // Maximum size of the command buffer

int copyFileToServer(int socket, const char* filePath);
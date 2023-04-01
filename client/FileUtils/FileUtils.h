#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>

#define MAX_LINE_LEN    1024

int copyFile(char *srcPath, char *dstPath);
int moveFile(char *srcPath, char *dstPath);
int createEmptyFile(char* filePath);

int appendToFile(char* filePath, char* strToWrite);

int removeDirectory(char *path);
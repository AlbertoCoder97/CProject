#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

char* concat(char* str1, char* str2);
char* getFirstWord(char* str);
char* removeFirstWord(char* str);
int countWords(char* str);
char* getWord(char *str, int i);
char* removeQuotes(char* str);
int containsChar(char* str, char c);
char* removeChar(char* str, char c);
int findWordIndex(char* str, char* targetWord);
int startsAndEndsWithDoubleQuotes(char* str);
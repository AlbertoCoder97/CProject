#include "StringUtils.h"

//Sane functions
char* concat(char* str1, char* str2) 
{
    char* result = malloc(strlen(str1) + strlen(str2) + 1);  // allocate memory for the concatenated string
    if (result == NULL) 
    {
        fprintf(stderr, "Error: memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    strcpy(result, str1);  // copy the first string to the result
    strcat(result, str2);  // concatenate the second string to the result
    return result;
}

//Functions to get first word of string as it simplifies lots of command handling.
char* getFirstWord(char* str) {
    size_t len = strlen(str);
    char* word = (char*) malloc(len + 1);
    int i = 0;
    while (isspace(str[i])) {
        i++;
    }
    int j = i;
    while (j < len && !isspace(str[j])) {
        j++;
    }
    memcpy(word, &str[i], j - i);
    word[j - i] = '\0';
    return word;
}

//This function is used to remove the first word of a string which in our case is the command received by the client to leave only the parameters for easier use.
char* removeFirstWord(char* str) {
    // Find the index of the first space or end of string
    int i = 0;
    while (str[i] != ' ' && str[i] != '\0') {
        i++;
    }

    // Allocate memory for the new string
    char* newStr = (char*) malloc(strlen(str) - i + 1);
    if (newStr == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    // Copy the second word and the rest of the string to the new string by moving forward pointer.
    strcpy(newStr, str + i + 1);

    return newStr;
}

int countWords(char* str) {
    int wordCount = 0;
    int strLength = strlen(str);
    int i;

    // Iterate through the string, incrementing the word count
    // whenever a space character is encountered
    for (i = 0; i < strLength; i++) {
        if (str[i] == ' ') {
            wordCount++;
        }
    }

    // Add one to the word count to account for the final word
    // (which is not followed by a space character)
    wordCount++;

    return wordCount;
}

//This is a bad split() implementation, but it works. I wrote this cause I'm in need to finish this project and I don't have much time.
char* getWord(char* str, int index) {
    char* str_copy;
    char* token;
    char* delimiter = " ";
    int count = 0;

    // create copy of input string
    str_copy = strdup(str);
    if (str_copy == NULL) {
        printf("Error copying string\n");
        return NULL;
    }

    // get first token
    token = strtok(str_copy, delimiter);

    // iterate through all tokens
    while (token != NULL) {
        if (count == index) {
            // found word, return it
            char* result = strdup(token);
            free(str_copy);
            return result;
        }
        token = strtok(NULL, delimiter);
        count++;
    }

    // word not found
    free(str_copy);
    return NULL;
}

//Simplifying command handling for run command.
char* removeQuotes(char* str) {
    char* result = malloc(strlen(str) + 1);
    memset(result, 0, strlen(str));

    int j = 0;
    for (int i = 0; str[i]; i++) {
        if (str[i] != '\"' && str[i] != '\'') {
            //Sets char of result to the char in string that is not single or double quote.
            result[j++] = str[i];
        }
    }
    //Adding EOL.
    result[j] = '\0';

    return result;
}

//I'm not sure whether is faster to write the code or writing the function name.
int startsAndEndsWithDoubleQuotes(char* str) {
    int len = strlen(str);
    if (len < 2) return 1;
    if(str[0] == '\"' && str[len-1] == '\"') return 0;
    return 1;
}


int containsChar(char* str, char c) {
    int len = strlen(str);
    for (int i = 0; i < len; i++) {
        if (str[i] == c) {
            return 0;
        }
    }
    return 1;
}

char* removeChar(char* str, char c) {
  // Allocate memory for the new string
  char* newStr = malloc(strlen(str) + 1);
  if (newStr == NULL) {
    fprintf(stderr, "Error: failed to allocate memory\n");
    exit(EXIT_FAILURE);
  }

  // Copy the characters from the original string to the new string, skipping the ones that match the given character
  int j = 0;
  for (int i = 0; i < strlen(str); i++) {
    if (str[i] != c) {
      newStr[j] = str[i];
      j++;
    }
  }
  newStr[j] = '\0';

  return newStr;
}

int findWordIndex(char* str, char* targetWord) {
    int index = 0;

    char *word = strtok(str, " ");
    while (word != NULL) {
        if (strcmp(word, targetWord) == 0) {
            return index;
        }
        word = strtok(NULL, " ");
        index++;
    }

    return -1;
}
#include "config.h"

//For logging purposes
char* className = "config";

int initializeConfig(Config* configuration, char* configFilePath)
{
    //For logging purposes
    char* methodName = "initializeConfig";

    //Opening file in read mode
    FILE* configFile = fopen(configFilePath, "r");

    //Return if file doesn't exist, as 0 = false in boolean if you use the initializeConfig in a check if needed.
    if(configFile == NULL) {
        printf("%s::%s - File %s doesn't exist.\n", className, methodName, configFilePath);
        return 0;
    } 

    printf("%s::%s - File %s opened. Initializing configuration object.\n", className, methodName, configFilePath);
    
    //Using a 128 bytes buffer to read one line at the time. Assuming lines won't be longer than 128 bytes.
    char line[MAX_LINE_READ];
    while(fgets(line, sizeof(line), configFile) != NULL)
    {
        //strtok() allows me to get only the chars before the newline. Allowing me to make comparisons.
        if(strcmp(strtok(line, "\n"), "#Port") == 0)
        {
            int port = atoi(fgets(line, sizeof(line), configFile));
            printf("%s::%s - Port configuration found. Setting listening port to: %d\n", className, methodName, port);
            configuration->port = port;
        }
        if(strcmp(strtok(line, "\n"), "#Root") == 0)
        {
            char* fakeRoot = strtok(fgets(line, sizeof(line), configFile),"\n");
            char* absolute_path = malloc(MAX_ROOT_LEN);
            if (absolute_path == NULL) {
                perror("malloc");
                return 1;
            }

            if (realpath(fakeRoot, absolute_path) == NULL) {
                perror("realpath");
                free(absolute_path);
                return 1;
            }
            printf("%s::%s - Root configuration found. Setting root to: %s\n", className, methodName, absolute_path);
            strncpy(configuration->root, absolute_path, strlen(absolute_path));
        }
        if(strcmp(strtok(line, "\n"), "#Cmds") == 0)
        {
            for(int i = 0; i <= MAX_COMMANDS; i++)
            {
                char* command = strtok(fgets(line, sizeof(line), configFile),"\n");
                if(command != NULL)
                {
                    printf("%s::%s - Command found. Added command: %s to list of available commands.\n", className, methodName, command);
                    strncpy(configuration->commands[i], command, strlen(command));
                }
            }
        }
    }

    
    printf("%s::%s - File %s closed. Configuration initialized correctly.\n", className, methodName, configFilePath);
    
    //Always close file
    fclose(configFile);

    return 0;
}


void printConfig(Config* pConfiguration)
{
    char* methodName = "printConfig";
    printf("%s::%s - Configuration's Port: %d\n", className, methodName, pConfiguration->port);
    printf("%s::%s - Configuration's Root: %s\n", className, methodName, pConfiguration->root);

    for(int i = 0; i < MAX_COMMANDS; i++)
    {
        printf("%s::%s - Allowed command: %s\n", className, methodName, pConfiguration->commands[i]);
    }
}


int isCommandAllowed(Config* config, char* command)
{
    char* methodName = "isCommandAllowed";
    char* temp = getFirstWord(command);
    for(int i = 0; i < MAX_COMMANDS; i++)
    {
        if(strcmp(strtok(temp, "\n"), config->commands[i]) == 0)
        {
            printf("%s::%s - The command [%s] is allowed!\n", className, methodName, command);
            return 0;
        }
    }
    printf("%s::%s - The command [%s] is not allowed!\n", className, methodName, command);
    return 1;
}
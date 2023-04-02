#include "server.h"

Config* pConfiguration;

int main(int argc, char* argv[]) {

    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[MAX_CLIENTCMDS_LENGTH] = {0};
    int bytesSent = 0;

    //All processes inherit the same configuration but the only real element used by child processes is the root path for checks in the commands handling.
    pConfiguration = malloc(sizeof(*pConfiguration));

    initializeConfig(pConfiguration, "./config/server.config");
    printConfig(pConfiguration);

    // Create a socket for the server
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        printf("Socket creation failed.\n");
        return -1;
    }

    // Set the server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htons(INADDR_ANY);
    address.sin_port = htons(pConfiguration->port);

    // Bind the socket to the server address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        printf("Binding socket failed.\n");
        return -1;
    }

    // Listen for incoming connections
    if (listen(server_fd, MAX_CONNECTIONS) < 0) {
        printf("Error while listening to connections.\n");
        return -1;
    }

    while(1) {
        // Accept a new connection
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // Create a child process to handle the new connection
        int pid = fork();

        //Could've used switch to check for the process, but I like ifs more.
        if (pid == 0) {
            // Child process code
            close(server_fd);

            //We don't want to make child processes execute commands as root therefore, just as failsafe, we manually set the child PID and GID to the PID and GID of the user that
            //executed the server. It is a bit inefficient, but it's better to be safe than sorry.
            if(setChildIDs() != 0) {
                perror("Set Child Process ID");
                // Shutdown socket -- a good habit
                shutdown(new_socket,SHUT_RDWR);
                close(new_socket);
                return 1;
            }

            //Getting IP of the client that connected.
            char* client_ip = inet_ntoa(address.sin_addr);
            printf("Client connected from IP address: %s\n", client_ip);

            //On client's connection changing directory to fakeRoot in configuration. If fake root doesn't exist, shut down the server as safety measure.
            if(chdir(pConfiguration->root) == 0) {
                printf("Directory changed to configuration's root.\n");

                //Debug log just to make sure the chdir actually worked.
                char cwd[MAX_ROOT_LEN];
                if(getcwd(cwd, sizeof(cwd))) {
                    printf("Current working dir: %s\n", cwd);
                }
            } else {
                char* message= "Server error. Directory does not exist. Contact System Administrator.";
                send(new_socket, message, strlen(message), 0);
                // Shutdown socket -- a good habit
                shutdown(new_socket,SHUT_RDWR);
                // Close the connection and exit the child process
                close(new_socket);
                return -1;
            }

            // Read commands from the client until the connection is closed
            while (1) {
                //Getting messages from client.
                valread = read(new_socket, buffer, MAX_CLIENTCMDS_LENGTH);

                if (valread <= 0) {
                    //Connection closed by the client
                    printf("Client connected with IP: %s closed the connection.\n", client_ip);
                    break;
                }

                

                /* Commands handling */

                //Remove the newline character at the end of the command
                char* command = strtok(buffer, "\n");

                //Prevents command injection.
                if(containsChar(command, ';')) {
                    char* message = "Operation not allowed!";
                    send(new_socket, message, strlen(message), 0);
                    memset(buffer, 0, MAX_CLIENTCMDS_LENGTH);
                    continue;
                }

                if(strcmp(getFirstWord(command), "exit") == 0) {
                    
                    //Sending exitACK to client
                    char* message = "exitACK";
                    send(new_socket, message, strlen(message), 0);

                    printf("Exit message received. Closing connection.\n");
                    break;
                } else if(strcmp(getFirstWord(command), "run") == 0) {
                    char* commandToExecute = removeFirstWord(command);
                    printf("Command to execute: %s\n", commandToExecute);
                    //Defining which type of the 4 allowed command types to execute.
                    /*
                        1: run cmd , it runs cmd on server (stdout to client).
                        2: run server:”cmd1 | cmd2” , it runs the pipe cmd1 | cmd2 on server (stdout to client).
                        3: run server:”cmd > file” , it runs cmd on server, and output to file on server.
                        4: run server:cmd > file , it runs cmd on server, output file on client.
                    */
                    
                    //To be fair, command type 1 and 2 are handled in almost the same exact way. The only operation done on it is the removal of the double quote and command allowed check.
                    //Although I'm aware it is inefficient, I want to keep them separated for my own mental structure even though they call the same function.

                    //Command type 1
                    if(startsAndEndsWithDoubleQuotes(commandToExecute) != 0 && containsChar(commandToExecute, '>') != 0 && containsChar(commandToExecute, '|') != 0) {
                        executeCommandType1(new_socket, commandToExecute);
                    }
                    //Command type 2
                    else if(startsAndEndsWithDoubleQuotes(commandToExecute) == 0 && containsChar(commandToExecute, '|') == 0 && strcmp(getWord(commandToExecute, 1), "|") == 0) {
                        executeCommandType2(new_socket, removeChar(commandToExecute, '\"'));
                    }
                    //Command type 3
                    else if(startsAndEndsWithDoubleQuotes(commandToExecute) == 0 && containsChar(commandToExecute, '>') == 0 && strcmp(getWord(commandToExecute, 1), ">") == 0){
                        executeCommandType3(new_socket, removeChar(commandToExecute, '\"'));
                    }
                    //Command type 4
                    else if(startsAndEndsWithDoubleQuotes(commandToExecute) == 1 && containsChar(commandToExecute, '>') == 0 && strcmp(getWord(commandToExecute, 1), ">") == 0) {
                        executeCommandType4(new_socket, commandToExecute);
                    }
                    //Syntax helper message
                    else {
                        char* message = "Wrong syntax!\n Usage: run cmd, run \"cmd1 | cmd2\", run \"cmd > file\", run cmd > file.";
                        send(new_socket, message, strlen(message), 0);
                    }
                }

                //Handle list
                else if (strcmp(getFirstWord(command), "list") == 0) {
                    if(countWords(command) != 1) {
                        char* message = "Wrong command usage. Syntax: list";
                        send(new_socket, message, strlen(message), 0);
                    } else {
                        DIR *dir;
                        struct dirent *ent;

                        // Open the current working directory
                        if ((dir = opendir(".")) != NULL) {
                            // Loop through all the entries in the directory
                            while ((ent = readdir(dir)) != NULL) {
                                // Send the entry name to the client
                                send(new_socket, concat(ent->d_name, "\n"), strlen(ent->d_name) + 1, 0);
                                //send(new_socket, "\n", 1, 0);
                            }
                            closedir(dir);
                        } else {
                            // Send an error message to the client if the directory can't be opened
                            char* error_msg = "Unable to open directory.";
                            send(new_socket, error_msg, strlen(error_msg), 0);
                        }
                    }
                }

                //Handle cd command
                else if(strcmp(getFirstWord(command), "cd") == 0) {
                    char cwd[MAX_ROOT_LEN];
                    if(getcwd(cwd, sizeof(cwd))) {
                        printf("Current working dir: %s\n", cwd);
                    }

                    char* param = removeFirstWord(command);

                    //This ensures that only one parameter is passed as the directory name.
                    if(countWords(param) != 1) {
                        char* message = "Wrong command usage. Syntax: cd <directory>";
                        send(new_socket, message, strlen(message), 0);
                    } else {
                        /* Absolute Paths handling */
                        if (param[0] == '/') {
                            if (strncmp(param, pConfiguration->root, strlen(pConfiguration->root)) != 0) {
                                char* message = "Cannot navigate outside the fake root path.";
                                send(new_socket, message, strlen(message), 0);
                            } else {
                                if(chdir(param) == 0) {
                                    if(getcwd(cwd, sizeof(cwd))) {
                                        printf("New working dir: %s\n", cwd);
                                        char* message = concat("Directory changed to: ", cwd);
                                        send(new_socket, message, strlen(message), 0);
                                    }
                                } else {
                                    char* message = "Error while executing command.";
                                    send(new_socket, message, strlen(message), 0);
                                }    
                            }
                        }

                        /* Relative Paths handling */

                        else if(strcmp(param, "..") == 0 && strcmp(strtok(cwd, "\n"), pConfiguration->root) == 0) {
                            char* message = "You reached the root. You cannot go further.";
                            send(new_socket, message, strlen(message), 0);
                        }
                        else if(chdir(param) == 0) {
                            if(getcwd(cwd, sizeof(cwd))) {
                                printf("New working dir: %s\n", cwd);
                                char* message = concat("Directory changed to: ", cwd);
                                send(new_socket, message, strlen(message), 0);
                            }
                        } else {
                            char* message = "Error while executing command.";
                            send(new_socket, message, strlen(message), 0);
                       }   
                    } 
                }

                //Handle create_dir command.
                else if(strcmp(getFirstWord(command), "create_dir") == 0) {
                    char* param = removeFirstWord(command);

                    //This ensures that only one parameter is passed as the directory name.
                    if(countWords(param) != 1) {
                        char* message = "Wrong command usage. Syntax: create_dir <directory>";
                        send(new_socket, message, strlen(message), 0);
                    } else {
                        if (param[0] == '/') {
                            if (strncmp(param, pConfiguration->root, strlen(pConfiguration->root)) != 0) {
                                char* message = "Cannot create directory outside the fake root path.";
                                send(new_socket, message, strlen(message), 0);
                            } else {
                                int status = mkdir(param, DIR_PERMISSIONS);
                                if (status == -1) {
                                    char* message = concat("Error creating directory: ", param);
                                    send(new_socket, message, strlen(message), 0);
                                } else {
                                    char* message = concat("Created directory: ", param);
                                    send(new_socket, message, strlen(message), 0);
                                }
                            }
                        } else {
                            int status = mkdir(param, DIR_PERMISSIONS);
                            if (status == -1) {
                                char* message = concat("Error creating directory: ", param);
                                send(new_socket, message, strlen(message), 0);
                            } else {
                                char* message = concat("Created directory: ", param);
                                send(new_socket, message, strlen(message), 0);
                            }
                        }
                    }
                }

                //Handle delete_dir command.
                else if(strcmp(getFirstWord(command), "delete_dir") == 0) {
                    char* param = removeFirstWord(command);

                    //This ensures that only one parameter is passed as the directory name.
                    if(countWords(param) != 1) {
                        char* message = "Wrong command usage. Syntax: delete_dir <directory>";
                        send(new_socket, message, strlen(message), 0);
                    } else {
                        //All these checks to avoid clients being able to delete anything on the server's disk other than folders inside the configuration's root directory.
                        if (param[0] == '/') {
                            if (strncmp(param, pConfiguration->root, strlen(pConfiguration->root)) != 0) {
                                char* message = "Cannot delete directory outside the fake root path.";
                                send(new_socket, message, strlen(message), 0);
                            } else {
                                int status = removeDirectory(param);
                                if (status == 1) {
                                    char* message = concat("Error deleting directory. Directory is not empty: ", param);
                                    send(new_socket, message, strlen(message), 0);
                                } else if (status == -1) {
                                    char* message = concat("Error while trying to delete directory: ", param);
                                    send(new_socket, message, strlen(message), 0);
                                } else {
                                    char* message = concat("Deleted directory: ", param);
                                    send(new_socket, message, strlen(message), 0);
                                }
                            }
                        } else {
                            int status = removeDirectory(param);
                            if (status == 1) {
                                char* message = concat("Error deleting directory. Directory is not empty: ", param);
                                send(new_socket, message, strlen(message), 0);
                            } else if (status == -1) {
                                    char* message = concat("Error while trying to delete directory: ", param);
                                    send(new_socket, message, strlen(message), 0);
                            } else {
                                char* message = concat("Deleted directory: ", param);
                                send(new_socket, message, strlen(message), 0);
                            }
                        }
                    }
                }

                //Handle delete file.
                else if(strcmp(getFirstWord(command), "delete") == 0) {
                    char* param = removeFirstWord(command);

                    //This ensures that only one parameter is passed as the directory name.
                    if(countWords(param) != 1) {
                        char* message = "Wrong command usage. Syntax: delete <file>";
                        send(new_socket, message, strlen(message), 0);
                    } else {
                        //All these checks to avoid clients being able to delete anything on the server's disk other than folders inside the configuration's root directory.
                        if (param[0] == '/') {
                            if (strncmp(param, pConfiguration->root, strlen(pConfiguration->root)) != 0) {
                                char* message = "Cannot delete file outside the fake root path.";
                                send(new_socket, message, strlen(message), 0);
                            } else {
                                int status = remove(param);
                                if (status == -1) {
                                    char* message = concat("Error while trying to delete file: ", param);
                                    send(new_socket, message, strlen(message), 0);
                                } else {
                                    char* message = concat("Deleted file: ", param);
                                    send(new_socket, message, strlen(message), 0);
                                }
                            }
                        } else {
                            int status = remove(param);
                            if (status == -1) {
                                char* message = concat("Error while trying to delete file: ", param);
                                send(new_socket, message, strlen(message), 0);
                            } else {
                                char* message = concat("Deleted file: ", param);
                                send(new_socket, message, strlen(message), 0);
                            }
                        }
                    }
                }

                //Copy file receives a file from the client and copies it in the 2nd parameter of the command.
                //TODO: Make sure copy happens inside fakeRoot directory.
                //Handle copy
                else if(strcmp(getFirstWord(command), "copy") == 0) {
                    //Once we know it's a copy command, we need to create the file (if it's not on disk) and write in it.
                    //TODO: Add semaphore to allow only one writer at the time.

                    //Check for correct syntax:
                    char* param = removeFirstWord(command);
                    char* fileToWrite = getWord(param, 1);

                    if(countWords(param) != 2) {
                        char* message = "Wrong command usage. Syntax: copy <file1> <file2>";
                        send(new_socket, message, strlen(message), 0);
                    } else {
                        //Check if second param (file where to write) is inside fakeRoot.
                        //Handling absolute path
                        if(fileToWrite[0] == '/') {
                            if (strncmp(fileToWrite, pConfiguration->root, strlen(pConfiguration->root)) != 0) {
                                char* message = "Cannot copy files from outside the fake root path.";
                                send(new_socket, message, strlen(message), 0);
                            } else {
                                //To make sure it's a new file, delete it for safety and then create a new one.
                                remove(fileToWrite);
                                createEmptyFile(fileToWrite);
                                //Start loop for writing on file.
                                while ((recv(new_socket, buffer, MAX_LINE_LEN, 0)) > 0) {
                                    if(strcmp(getFirstWord(buffer), "endw") == 0) break;
                                    appendToFile(fileToWrite, buffer);
                                    memset(buffer, 0, MAX_LINE_LEN);
                                }

                                char* message = "File copied to server.";
                                send(new_socket, message, strlen(message), 0);
                            }
                        } else {
                            //To make sure it's a new file, delete it for safety and then create a new one.
                            remove(fileToWrite);
                            createEmptyFile(fileToWrite);
                            //Start loop for writing on file.
                            while ((recv(new_socket, buffer, MAX_LINE_LEN, 0)) > 0) {
                                if(strcmp(getFirstWord(buffer), "endw") == 0) break;
                                appendToFile(fileToWrite, buffer);
                                memset(buffer, 0, MAX_LINE_LEN);
                            }

                            char* message = "File copied to server.";
                            send(new_socket, message, strlen(message), 0);
                        }
                    }
                }

                //Handle move
                else if(strcmp(getFirstWord(command), "move") == 0) {
                    char* param = removeFirstWord(command);
                    char* param1 = getWord(param, 0);
                    char* param2 = getWord(param, 1);

                    //This ensures that only one parameter is passed as the directory name.
                    if(countWords(param) != 2) {
                        char* message = "Wrong command usage. Syntax: move <file1> <file2>";
                        send(new_socket, message, strlen(message), 0);
                    } else {
                        /* Handle copy file absolute paths */
                        //Both absolute paths
                        if(param1[0] == '/' && param2[0] == '/') {
                            if (strncmp(param1, pConfiguration->root, strlen(pConfiguration->root)) != 0 || 
                                strncmp(param2, pConfiguration->root, strlen(pConfiguration->root)) != 0) {
                                char* message = "Cannot copy files outside the fake root path.";
                                send(new_socket, message, strlen(message), 0);
                            } else {
                                moveFile(param1, param2);
                                char* message = "Filed moved.";
                                send(new_socket, message, strlen(message), 0);
                            }
                        }

                        //Only first is absolute path
                        else if(param1[0] == '/') {
                            if (strncmp(param1, pConfiguration->root, strlen(pConfiguration->root)) != 0) {
                                char* message = "Cannot copy files from outside the fake root path.";
                                send(new_socket, message, strlen(message), 0);
                            } else {
                                moveFile(param1, param2);
                                char* message = "Filed moved.";
                                send(new_socket, message, strlen(message), 0);
                            }
                        }

                        //Only second is absolute path
                        else if(param2[0] == '/') {
                            if (strncmp(param1, pConfiguration->root, strlen(pConfiguration->root)) != 0) {
                                char* message = "Cannot copy files from outside the fake root path.";
                                send(new_socket, message, strlen(message), 0);
                            } else {
                                moveFile(param1, param2);
                                char* message = "Filed moved.";
                                send(new_socket, message, strlen(message), 0);
                            }
                        } else {
                            moveFile(param1, param2);
                            char* message = "Filed moved.";
                            send(new_socket, message, strlen(message), 0);
                        }
                    }
                }

                else{
                    printf("Received command: %s\n", command);
                    char* message = "Unkwown command.";
                    send(new_socket, message, strlen(message), 0);
                }

                // Clear the buffer for the next iteration
                memset(buffer, 0, MAX_CLIENTCMDS_LENGTH);
            }
            
            // Shutdown socket -- a good habit
            shutdown(new_socket,SHUT_RDWR);
            // Close the connection and exit the child process
            close(new_socket);
            exit(0);
        } else if (pid < 0) {
            perror("fork");
        } else {
            // Parent process code
            close(new_socket);
            int status;
            waitpid(pid, &status, 0);
        }
    }

    return 0;
}



int setChildIDs()
{
    uid_t uid = getuid(); // get current user ID
    gid_t gid = getgid(); // get current group ID
    
    printf("Child process before changing user ID:\n");
    printf("User ID: %d\n", uid);
    printf("Group ID: %d\n", gid);
    
    // change user and group ID
    if (setuid(uid) < 0) { // replace 1000 with the desired user ID
        perror("setuid");
        return 1;
    }
    
    if (setgid(gid) < 0) { // replace 1000 with the desired group ID
        perror("setgid");
        return 1;
    }
    
    uid = getuid(); // get new user ID
    gid = getgid(); // get new group ID
    
    printf("Child process after changing user ID:\n");
    printf("User ID: %d\n", uid);
    printf("Group ID: %d\n", gid);
    return 0;
}

//Defining which type of the 4 allowed command types to execute.
/*
    1: run cmd , it runs cmd on server (stdout to client).
    2: run server:”cmd1 | cmd2” , it runs the pipe cmd1 | cmd2 on server (stdout to client).
    3: run server:”cmd > file” , it runs cmd on server, and output to file on server.
    4: run server:cmd > file , it runs cmd on server, output file on client.
*/


int executeCommandType1(int clientSocket, char* command) {
    if(isCommandAllowed(pConfiguration, getFirstWord(command)) != 0) {
        char* message = "Operation not allowed!";
        send(clientSocket, message, strlen(message), 0);
        return -1;
    }
    char buffer[MAX_PIPE_LINE];
    FILE* pipe = popen(command, "r"); // open the command as a pipe so that we can read the output.
    if (!pipe) {
        return 1;
    }

    while (fgets(buffer, MAX_PIPE_LINE, pipe) != NULL) {
        // Send the output to the client socket
        if (send(clientSocket, buffer, strlen(buffer), 0) == -1) {
            return 1;
        }
    }

    if (pclose(pipe) == -1) { // close the pipe
        return 1;
    }
    return 0;
}

int executeCommandType2(int clientSocket, char* command) {
    char* command1 = getFirstWord(command);
    //Need to get the 3rd word as "|" counts as a word.
    char* command2 = getWord(command, 2);

    if(isCommandAllowed(pConfiguration, getFirstWord(command1)) != 0 || isCommandAllowed(pConfiguration, getFirstWord(command2))) {
        char* message = "Operation not allowed!";
        send(clientSocket, message, strlen(message), 0);
        return -1;
    }

    char buffer[MAX_PIPE_LINE];
    FILE* pipe = popen(command, "r"); // open the command as a pipe so that we can read the output.
    if (!pipe) {
        return 1;
    }

    while (fgets(buffer, MAX_PIPE_LINE, pipe) != NULL) {
        // Send the output to the client socket
        if (send(clientSocket, buffer, strlen(buffer), 0) == -1) {
            return 1;
        }
    }

    if (pclose(pipe) == -1) { // close the pipe
        return 1;
    }
    return 0;
}

//Once command is executed I don't need to do anything but letting the client know it was done.
int executeCommandType3(int clientSocket, char* command) {
    if(isCommandAllowed(pConfiguration, getFirstWord(command)) != 0) {
        char* message = "Operation not allowed!";
        send(clientSocket, message, strlen(message), 0);
        return -1;
    }

    FILE* pipe = popen(command, "r"); // open the command as a pipe so that we can read the output.
    if (!pipe) {
        return 1;
    }

    if (pclose(pipe) == -1) { // close the pipe
        return 1;
    }

    char* message = "Command executed.";
    send(clientSocket, message, strlen(message), 0);

    return 0;
}

//TODO: Find a way to write on a file onto the client.
//Idea: Send everything as response and let client handle write on file.
//This is the opposite of the copy.
int executeCommandType4(int clientSocket, char* command) {
    if(isCommandAllowed(pConfiguration, getFirstWord(command)) != 0) {
        char* message = "Operation not allowed!";
        send(clientSocket, message, strlen(message), 0);
        return -1;
    }

    //Creating temp file where output of command is executed, reading it and sending the output to client
    //Deleting the file on server when done.
    //Executing command. Example: ls > file1.txt
    
    //To make this easier we can:
    // 1 - Ignore the file name (as it might exist on the server)
    // 2 - Construct a new command where we execute the command and output it to a local file.
    // 3 - Open the local file and send its content to the client + endw
    // 4 - Delete server file.
    // This is possible by making the client handling the writing.

    //This is ugly but functional!
    char* param1 = getFirstWord(command);
    char* tempFileName = "../commandExec.txt";
    char* finalCommand = concat(param1, " > ");
    finalCommand = concat(finalCommand, tempFileName);

    printf("Command executed on server: %s\n", finalCommand);


    FILE* pipe = popen(finalCommand, "r"); // open the command as a pipe so that we can read the output.
    if (!pipe) {
        return 1;
    }

    //Reading from pipe.

    if (pclose(pipe) == -1) { // close the pipe
        return 1;
    }

    char fileBuffer[MAX_LINE_LEN];
    memset(fileBuffer, 0, MAX_LINE_LEN);
    FILE* file;

    // open the file for reading
    file = fopen(tempFileName, "r");
    if (file < 0) {
        printf("Error while opening file.\n");
        return -1;
    }

    //Read the file content and send it to the server
    while (fgets(fileBuffer, MAX_LINE_LEN, file) != NULL) {
        if (send(clientSocket, fileBuffer, MAX_LINE_LEN, 0) < 0) {
            printf("Error while sending file to server.\n");
            fclose(file);
            return -1;
        }
        printf("Buffer content: %s\n", fileBuffer);
    }

    char* endOfWriting = "endw";
    send(clientSocket, endOfWriting, strlen(endOfWriting), 0);

    fclose(file);
    remove(tempFileName);
    return 0;
}
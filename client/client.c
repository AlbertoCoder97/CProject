#include "client.h"

int main(int argc, char* argv[]) {

    if (argc!=3) {
	    printf("\nUsage:\n\t%s <IP> <PORT>\n",argv[0]);
	    return 1;
    }

    char* SERVER_IP = argv[1];
    int SERVER_PORT = atoi(argv[2]);

    //Create a socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);   
    if (sock == -1) {
        printf("Error while creating socket. Exiting client.\n");
        return 1;
    }

    //Creating connection entity.
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(SERVER_PORT);

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        printf("Error while connecting to server. Exiting client.\n");
        return 1;
    }

    
    /**
     * On client we need to handle only 3 elements: 
     * 1 - Exit command -> we need to close socket and terminate the program.
     * 2 - On command send that is not copy file1 > file2 then we just send the command and receive the response that is printed on stdout.
     * 3 - copy file1 > file2 where we copy a local file on the client's pc in a file on the server. Server handles file creation, check if inside fakeRoot etc...
     * */

    //Buffer is a string that we use to store commands to send and received responses from the server.
    char buffer[LINE_SIZE];
    while (1) {
        
        memset(buffer, 0, LINE_SIZE);
        printf("Enter command: ");
        //Read a command from the console

        fgets(buffer, LINE_SIZE, stdin);
        //Prevent sending empty messages.
        if(strlen(buffer) == 1 && buffer[0] == '\n') {
            printf("You need to enter a command.\n");
            continue;
        }
        //Removing new line from anything in the buffer.
        strtok(buffer, "\n");

        //If we send exit, then we break out of while, close the socket and terminate the client.
        if(strcmp(getFirstWord(buffer), "exit") == 0) {
            char* exitMessage = "exit";
            send(sock, exitMessage, strlen(exitMessage), 0);
            
            //Receive the server's response
            if(read(sock, buffer, LINE_SIZE) < 0) {
                printf("Error while reading response. Exiting.\n");
                break;
            }

            printf("Buffer content: %s\n", buffer);

            //We want to make sure that we receive "exitACK" as response as well."
            if(strcmp(getFirstWord(buffer), "exitACK") == 0) {
                printf("Exit command received and exit ACK received. Exiting program.\n");
                break;
            }       
        }

        else if(strcmp(getFirstWord(buffer), "copy") == 0) {
            char* fileToSend = getWord(buffer, 1);
            printf("Sending file %s to server.\n", fileToSend);

            //Letting the server know we're about to send the file.
            send(sock, buffer, strlen(buffer), 0);
            
            copyFileToServer(sock, fileToSend);
            

            //Receiving response and then skipping to next cycle.
            memset(buffer, 0, LINE_SIZE);
            recv(sock, buffer, LINE_SIZE, 0);
            
            printf("Server response: %s\n", buffer);
            memset(buffer, 0, LINE_SIZE);
            continue;
        }

        else if (strcmp(getFirstWord(buffer), "run") == 0 && containsChar(buffer, '>') == 0 && strcmp(getWord(buffer, 2),">") == 0){

            //Creating empty file using the file name passed in input. '>' counts as a word.
            char* fileToWrite = getWord(buffer, 3);
            printf("File to create on disk: %s\n", fileToWrite);
            //Removing file in case it already exists.
            remove(fileToWrite);
            createEmptyFile(fileToWrite);
            
            //Sending command to server.
            send(sock, buffer, strlen(buffer), 0);

            //Start loop for writing on file.
            while ((recv(sock, buffer, MAX_LINE_LEN, 0)) > 0) {
                if(strcmp(getFirstWord(buffer), "endw") == 0) break;
                appendToFile(fileToWrite, buffer);
                memset(buffer, 0, MAX_LINE_LEN);
            }

            printf("Command output copied to file %s\n", fileToWrite);
            memset(buffer, 0, LINE_SIZE);
            continue;
        }

        //Send the command to the server when the command is not exit nor copy.
        else if (send(sock, buffer, strlen(buffer), 0) == -1) {
            printf("Error occurred while sending command to server. Exiting client.\n");
            break;
        }

        //Receive the server's response + cleaning buffer before receiving response.
        memset(buffer, 0, LINE_SIZE);
        if (recv(sock, buffer, LINE_SIZE, 0) == -1) {
            printf("Error on server's response. Exiting client.\n");
            break;
        }

        //Print the server's response to the console
        printf("Server response: %s\n", buffer);
    }

    //It's good habit to shutdown the socket before closing it.
    shutdown(sock,SHUT_RDWR);
    close(sock);    // Close the socket
    return 0;
}

int copyFileToServer(int socket, const char* filePath) {
    char fileBuffer[LINE_SIZE];
    memset(fileBuffer, 0, LINE_SIZE);
    FILE* file;

    // open the file for reading
    file = fopen(filePath, "r");
    if (file < 0) {
        printf("Error while opening file.\n");
        return -1;
    }

    //Read the file content and send it to the server
    while (fgets(fileBuffer, LINE_SIZE, file) != NULL) {
        if (send(socket, fileBuffer, LINE_SIZE, 0) < 0) {
            printf("Error while sending file to server.\n");
            fclose(file);
            return -1;
        }
        printf("Buffer content: %s\n", fileBuffer);
    }

    char* endOfWriting = "endw";
    send(socket, endOfWriting, strlen(endOfWriting), 0);

    fclose(file);
    return 0;
}
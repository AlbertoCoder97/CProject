#include "FileUtils.h"

int createEmptyFile(char* filePath) {
    FILE* sourceFile = fopen(filePath, "w");
    if (!sourceFile) {
        printf( "Error: Unable to open source file %s\n", filePath);
        return -1;
    }

    fclose(sourceFile);

    return 0;
}

int copyFile(char *srcPath, char *dstPath) {

    //Defining line to write each read.
    char buffer[MAX_LINE_LEN];

    // Open the source file for reading
    FILE* sourceFile = fopen(srcPath, "r");
    if (!sourceFile) {
        printf( "Error: Unable to open source file %s\n", srcPath);
        return -1;
    }

    // Open the destination file for writing, creating it if it doesn't exist
    FILE* destinationFile = fopen(dstPath, "w");
    if (!destinationFile) {
        printf( "Error: Unable to open destination file %s\n", dstPath);
        fclose(destinationFile);
        return -1;
    }

    // Copy the contents of the source file to the destination file one char at the time.
    while(fgets(buffer, MAX_LINE_LEN, sourceFile) != NULL) {
        fputs(buffer, destinationFile);
    }
    
    //Closing files and freeing line string.
    fclose(sourceFile);
    fclose(destinationFile);

    return 0;
}

//Copy file + remove. Using copyFile function to avoid code repetition.
int moveFile(char *srcPath, char *dstPath) {

    //Recycling copy function.
    copyFile(srcPath, dstPath);

    // Delete the source file
    if (remove(srcPath) != 0) {
        printf("Error: Unable to delete source file %s\n", srcPath);
        return -1;
    }

    return 0;
}

//This only removes a directory if it is empty
int removeDirectory(char *path) {
    // Open the directory for reading
    DIR *dirp = opendir(path);
    if (!dirp) {
        printf("Error: Unable to open directory %s\n", path);
        return -1;
    }

    // Read the contents of the directory
    struct dirent *entry;
    int count = 0;
    while ((entry = readdir(dirp)) != NULL) {
        // Skip the "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        // Increment the count for non-hidden files and directories
        count++;
    }

    // Close the directory
    closedir(dirp);

    // If the directory is not empty, return an error
    if (count > 0) {
        printf("Error: Directory %s is not empty\n", path);
        return 1;
    }

    // Delete the directory
    if (rmdir(path) != 0) {
        printf( "Error: Unable to delete directory %s\n", path);
        return -1;
    }

    return 0;
}

//Function to append string to file.
int appendToFile(char* filePath, char* strToWrite) {
    FILE* file = fopen(filePath, "a"); // open the file in append mode
    if (file == NULL) {
        return -1; // return -1 if unable to open file
    }

    int result = fputs(strToWrite, file); // write the string to the file
    fclose(file); // close the file pointer

    if (result == EOF) {
        return -2; // return -2 if unable to write to file
    }

    return 0; // return 0 if write is successful
}
#include "ulib.h"

#define FILENAME "testfile.txt"
#define LINKNAME "testfile_link.txt"
#define BUF_SIZE 1024

int main() {
    int fd;
    char buffer[BUF_SIZE];
    const char *content = "This is a test content for the original file.\n";
    
    // Step 1: Create and write to the original file
    fd = open(FILENAME, O_WRONLY | O_CREATE | O_TRUNC);
    if (fd < 0) {
        printf("Error: Failed to open %s\n", FILENAME);
        return 1;
    }
    if (write(fd, content, strlen(content)) != (size_t)strlen(content)) {
        printf("Error: Failed to write to %s\n", FILENAME);
        close(fd);
        return 1;
    }
    close(fd);

    // Step 2: Create a hard link to the original file
    if (link(FILENAME, LINKNAME) < 0) {
        printf("Error: Failed to create link %s\n", LINKNAME);
        return 1;
    }

    // Step 3: Read from the hard link file
    fd = open(LINKNAME, O_RDONLY);
    if (fd < 0) {
        printf("Error: Failed to open %s\n", LINKNAME);
        return 1;
    }
    size_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);
    if (bytesRead < 0) {
        printf("Error: Failed to read from %s\n", LINKNAME);
        close(fd);
        return 1;
    }
    buffer[bytesRead] = '\0';  // Null-terminate the string
    close(fd);

    // Print the content read from the hard link
    printf("Content read from %s:\n%s", LINKNAME, buffer);

    // Step 4: Verify that the content matches the original content
    if (strcmp(content, buffer) != 0) {
        printf("Error: Content mismatch between the original file and the hard link.\n");
        return 1;
    }

    // Step 5: Unlink the files
    if (unlink(FILENAME) < 0) {
        printf("Error: Failed to unlink %s\n", FILENAME);
        return 1;
    }
    if (unlink(LINKNAME) < 0) {
        printf("Error: Failed to unlink %s\n", LINKNAME);
        return 1;
    }

    printf("Hard link test passed successfully.\n");
    return 0;
}

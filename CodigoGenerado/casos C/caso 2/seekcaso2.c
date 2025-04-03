#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Function to execute system commands with basic validation
void execute_command(const char *command) {
    // Basic security check - reject commands with dangerous characters
    if (strchr(command, ';') || strchr(command, '&') || strchr(command, '|') || 
        strchr(command, '>') || strchr(command, '<') || strchr(command, '`')) {
        printf("Error: Command contains potentially dangerous characters\n");
        return;
    }
    
    // Execute the command
    printf("Executing: %s\n", command);
    int result = system(command);
    
    if (result == -1) {
        printf("Failed to execute command\n");
    }
}

// Function to get user input and execute command
void get_and_execute() {
    char command[100]; // Limited buffer size
    
    printf("Enter a command to execute: ");
    
    // Safer alternative to gets()
    if (fgets(command, sizeof(command), stdin) == NULL) {
        printf("Error reading input\n");
        return;
    }
    
    // Remove newline character
    command[strcspn(command, "\n")] = '\0';
    
    // Basic input validation
    if (strlen(command) == 0) {
        printf("No command entered\n");
        return;
    }
    
    execute_command(command);
}

int main() {
    printf("Simple Command Executor\n");
    printf("Warning: This program executes system commands - use with caution!\n");
    
    get_and_execute();
    
    return 0;
}
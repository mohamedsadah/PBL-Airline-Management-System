#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <emscripten/emscripten.h>

#define MAX_FIELD_SIZE 128
// Store the credentials file under the persistent directory.
#define CRED_FILE "/persistent/users.txt"

// Function to register a new user by appending the username and password.
// Credentials are stored as "username:password" per line.
EMSCRIPTEN_KEEPALIVE
int register_user(const char *username, const char *password) {
    FILE *file = fopen(CRED_FILE, "a");
    if (!file) {
        // Failed to open file for writing.
        return -1;
    }
    // Note: In a real application, passwords should be hashed instead of stored in plaintext.
    fprintf(file, "%s:%s\n", username, password);
    fclose(file);

    EM_ASM({
        FS.syncfs(false, function(err) {
            if (err) {
                console.error('Failed to sync to IndexedDB:', err);
            } else {
                console.log('Saved to IndexedDB.');
            }
        });
    });

    return 0;
}

// Function to verify login credentials.
// Returns 1 if credentials match, 0 if not, and -2 if file cannot be opened.
EMSCRIPTEN_KEEPALIVE
int login(const char *username, const char *password) {
    char buf[512];
    FILE *file = fopen(CRED_FILE, "r");
    if (!file) {
        // File could not be opened (e.g., no user registered yet).
        return -2;
    }
    while (fgets(buf, sizeof(buf), file) != NULL) {
        char file_username[MAX_FIELD_SIZE];
        char file_password[MAX_FIELD_SIZE];
        // Extract the username and password stored in the file.
        if (sscanf(buf, "%127[^:]:%127s", file_username, file_password) == 2) {
            if (strcmp(username, file_username) == 0 && strcmp(password, file_password) == 0) {
                fclose(file);
                return 1;  // Successful login.
            }
        }
    }
    fclose(file);
    return 0;  // Login failed.
}

// Function to mount and initialize a persistent filesystem (IDBFS)
EMSCRIPTEN_KEEPALIVE
void init_persistent_fs() {
    // The following code is executed in JavaScript via EM_ASM.
    EM_ASM({
        // Create a directory to store persistent files.
        FS.mkdir('/persistent');
        // Mount the persistent file system based on IndexedDB.
        FS.mount(IDBFS, {}, '/persistent');
        // Synchronize from IndexedDB to MEMFS (true = populate the in-memory FS).
        FS.syncfs(true, function(err) {
            if (err) {
                console.error('Error loading persistent filesystem:', err);
            } else {
                console.log('Persistent filesystem loaded successfully.');
            }
        });
    });
}

// Function to synchronize any changes back to IndexedDB.
// This should be called after file modifications.


int main() {
    // Initialize the persistent filesystem at startup.
    init_persistent_fs();
    // main() must return, but initialization is complete.
    return 0;
}

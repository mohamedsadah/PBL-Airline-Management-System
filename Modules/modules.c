#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <emscripten/emscripten.h>

#define MAX_FIELD_SIZE 128
// Store the credentials file under the persistent directory.
#define CRED_FILE "/persistent/users.dat"
#define FLIGHT_file "/persistent/Flight.dat"
#define ADMIN_FILE "/persistent/admin.dat"
#define BOOKINGS "/persistent/Bookings.dat"


// Structure for Flight
typedef struct Flight {
    char flightNumber[20];
    char departure[50];
    char destination[50];
    char departDate[20];
    char arrivalDate[20];
    char time[10];
    char stops[4][50]; // up to 4 stops
    int stopCount;
    int price;

    struct Flight *left, *right;
} Flight;

// admin
typedef struct {
    char username[50];
    char password[50];
} Users;


// Root of the BST
Flight *root = NULL;

char jsonBuffer[100000]; //for storing json format of flight data 

// Create new flight node
Flight* createFlightNode(const char *fNum, const char *dep, const char *dest, const char *dDate, const char *aDate, const char *time, char stops[4][50], int stopCount, int price) {
    Flight *newNode = (Flight *)malloc(sizeof(Flight));
    strcpy(newNode->flightNumber, fNum);
    strcpy(newNode->departure, dep);
    strcpy(newNode->destination, dest);
    strcpy(newNode->departDate, dDate);
    strcpy(newNode->arrivalDate, aDate);
    strcpy(newNode->time, time);
    newNode->stopCount = stopCount;
    for (int i = 0; i < stopCount; i++) {
        strcpy(newNode->stops[i], stops[i]);
    }
    newNode->price = price;
    newNode->left = NULL;
    newNode->right = NULL;
    return newNode;
}

// Insert into BST
int insertFlight(Flight **node, Flight *newNode) {
    if (*node != NULL){
        if (strcmp(newNode->flightNumber, (*node)->flightNumber) < 0){
            return insertFlight(&(*node)->left, newNode);
            
        }
        else if (strcmp(newNode->flightNumber, (*node)->flightNumber) > 0){
            return insertFlight(&(*node)->right, newNode); 
            
        }
        else{
            return 0; //duplicate
        }
        
    }
    else{
        *node = newNode;
        return 1;
    }
    
}

// Save to file
int saveFlightToFile(Flight *flight) {
    FILE *fp = fopen(FLIGHT_file, "ab"); // binary append
    if (!fp) return 0;

    size_t written = fwrite(flight, sizeof(Flight), 1, fp);
    fclose(fp);

    // Sync to IndexedDB after writing
    EM_ASM({
        FS.syncfs(false, function(err) {
            if (err) {
                console.error('Failed to sync to IndexedDB:', err);
            } else {
                console.log('Saved to IndexedDB.');
            }
        });
    });

    return written == 1; // return success (1) or failure (0)
}

// Exposed to JS
EMSCRIPTEN_KEEPALIVE
int addFlight(const char *fNum, const char *dep, const char *dest, const char *dDate, const char *aDate, const char *time, char **stopList, int stopCount, int price) {
    char stops[4][50];
    for (int i = 0; i < stopCount && i < 4; i++) {
        strncpy(stops[i], stopList[i], 49);
        stops[i][49] = '\0';
        printf("Received stop %d: %s\n", i + 1, stops[i]); // DEBUG
    }



    Flight *newFlight = createFlightNode(fNum, dep, dest, dDate, aDate, time, stops, stopCount, price);
    if(insertFlight(&root, newFlight)){
        if(saveFlightToFile(newFlight) == 0){
            return -2;
        }
        return 1;    
    }else {

        free(newFlight);
        return -1;
        
    }

}

EMSCRIPTEN_KEEPALIVE
void loadFlightsFromFile() {
    FILE *fp = fopen(FLIGHT_file, "rb");
    if (!fp) return;

    Flight temp;
    while (fread(&temp, sizeof(Flight), 1, fp) == 1) {

        Flight *node = createFlightNode(
            temp.flightNumber, temp.departure, temp.destination,
            temp.departDate, temp.arrivalDate, temp.time, temp.stops, temp.stopCount, 
            temp.price);

        
        insertFlight(&root, node);
        printf("Loading flight: %s\n", node->flightNumber);
    }
    fclose(fp);

    printf("BST Successfully Populated. Flights:\n");

    // Inline in-order traversal
    Flight *stack[100]; // simple manual stack for up to 100 nodes
    int top = -1;
    Flight *curr = root;

    while (curr != NULL || top != -1) {
        while (curr != NULL) {
            stack[++top] = curr;
            curr = curr->left;
        }

        curr = stack[top--];

        // Print flight details
        printf("Flight Number: %s\n", curr->flightNumber);
        printf("Departure: %s -> Destination: %s\n", curr->departure, curr->destination);
        printf("Departure Date: %s | Arrival Date: %s | Time: %s\n", curr->departDate, curr->arrivalDate, curr->time);
        printf("Stops (%d): ", curr->stopCount);
        for (int i = 0; i < curr->stopCount; i++) {
            printf("%s ", curr->stops[i]);
        }
        printf("\n----------------------\n");

        curr = curr->right;
    }

    EM_ASM({
        console.log('BST Loaded and Flights Printed to Console');
    });
}


/*function to traverse BST and convert all data to json format, 
so it can be displayed on web using JS */
void buildJSON(Flight *node, char *buffer) {
    if (node == NULL) return;

    buildJSON(node->left, buffer); // traverse left subtree

    char flightEntry[1024];
    sprintf(flightEntry,
        "{ \"flightNumber\": \"%s\", \"departure\": \"%s\", \"destination\": \"%s\", "
        "\"departDate\": \"%s\", \"arrivalDate\": \"%s\", \"time\": \"%s\", \"stopCount\": %d,\"price\": %d, \"stops\": [",
        node->flightNumber, node->departure, node->destination,
        node->departDate, node->arrivalDate, node->time, node->stopCount, node->price);

    strcat(buffer, flightEntry);

    int addedStop = 0;
    for (int i = 0; i < node->stopCount; ++i) {
        if (strlen(node->stops[i]) > 0 && node->stops[i][0] >= 32 && node->stops[i][0] <= 126) {
            if (addedStop > 0) strcat(buffer, ", ");
            strcat(buffer, "\"");
            strcat(buffer, node->stops[i]);
            strcat(buffer, "\"");
            addedStop++;
        }
    }

    strcat(buffer, "] },");
    buildJSON(node->right, buffer); // traverse right subtree
}

void buildSearchJSON(Flight *node, char *buffer, const char *departure, const char *destination, const char *date) {
    if (node == NULL) return;

    buildSearchJSON(node->left, buffer, departure, destination, date); // traverse left subtree

    if(strcmp(node->departure, departure)==0 && 
       strcmp(node->destination, destination)==0 && 
       strcmp(node->departDate, date)==0)
       {
    char flightEntry[1024];
    sprintf(flightEntry,
        "{ \"flightNumber\": \"%s\", \"departure\": \"%s\", \"destination\": \"%s\", "
        "\"departDate\": \"%s\", \"arrivalDate\": \"%s\", \"time\": \"%s\", \"stopCount\": %d,\"price\": %d, \"stops\": [",
        node->flightNumber, node->departure, node->destination,
        node->departDate, node->arrivalDate, node->time, node->stopCount, node->price);

    strcat(buffer, flightEntry);

    int addedStop = 0;
    for (int i = 0; i < node->stopCount; ++i) {
        if (strlen(node->stops[i]) > 0 && node->stops[i][0] >= 32 && node->stops[i][0] <= 126) {
            if (addedStop > 0) strcat(buffer, ", ");
            strcat(buffer, "\"");
            strcat(buffer, node->stops[i]);
            strcat(buffer, "\"");
            addedStop++;
        }
    }

    strcat(buffer, "] },");
}
    buildSearchJSON(node->right, buffer, departure, destination, date); // traverse right subtree
}


EMSCRIPTEN_KEEPALIVE
const char* getAllFlightsJSON() {
    strcpy(&jsonBuffer[0], "[");  // Clear the buffer
    jsonBuffer[1] = '\0';  // Clear the buffer

    buildJSON(root, jsonBuffer + 1);

    // Remove trailing comma
    int len = strlen(jsonBuffer);
    if (len > 1 && jsonBuffer[len - 1] == ',') {
        jsonBuffer[len - 1] = '\0';
    }

    strcat(jsonBuffer, "]");
    return jsonBuffer;
}

EMSCRIPTEN_KEEPALIVE
const char* getOneWayFlightsJSON(const char *departure, const char *destination, const char *date) {
    strcpy(&jsonBuffer[0], "[");  // Clear the buffer
    jsonBuffer[1] = '\0';  // Clear the buffer

    buildSearchJSON(root, jsonBuffer + 1, departure, destination, date);

    // Remove trailing comma
    int len = strlen(jsonBuffer);
    if (len > 1 && jsonBuffer[len - 1] == ',') {
        jsonBuffer[len - 1] = '\0';
    }

    strcat(jsonBuffer, "]");
    return jsonBuffer;
}


// Function to register a new user by appending the username and password.
// Credentials are stored as "username:password" per line.
EMSCRIPTEN_KEEPALIVE
int register_user(const char *username, const char *password) {
    FILE *file = fopen(CRED_FILE, "ab");
    if (!file) {
        // Failed to open file for writing.
        return -1;
    }

    Users user;
    strncpy(user.username, username, sizeof(user.username));
    strncpy(user.password, password, sizeof(user.password));
    // Note: In a real application, passwords should be hashed instead of stored in plaintext.
    fwrite(&user, sizeof(user), 1, file);
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
    if(strcmp(username,"admin") == 0){
    
        FILE *adminFile = fopen(ADMIN_FILE, "rb");
        if(!adminFile) return -2;

        Users admin;
        while(fread(&admin, sizeof(Users), 1, adminFile) == 1){
            //retrive and check if password match any password in file
            if(strcmp(admin.password, password) == 0){
                fclose(adminFile);
                return 1; // success
            } 
        }
        
        fclose(adminFile);
        return 0; //login failed

    }

    else{

        FILE *file = fopen(CRED_FILE, "rb");
        if (!file) {
            // File could not be opened (e.g., no user registered yet).
            return -2;
        }

        Users user;
        while (fread(&user, sizeof(user), 1, file) == 1) {
            if(strcmp(user.username, username) == 0 && strcmp(user.password, password) == 0){
                fclose(file);
                return 1; // success
            }
        }

        fclose(file);
        return 0;  // Login failed.
    }
}


EMSCRIPTEN_KEEPALIVE
void createInitialAdmins() {
    FILE *fp = fopen(ADMIN_FILE, "wb");  // 'wb' will truncate any existing data
    if (!fp) {
        printf("Failed to open admin.dat\n");
        return;
    }

    Users admins[] = {
        {"admin", "mcac"},
        {"admin", "mscit"},
        {"admin", "mcab"},
        {"admin", "mcaa"},
        {"admin", "mentor"}
    };

    int count = sizeof(admins) / sizeof(admins[0]);

    for (int i = 0; i < count; i++) {
        fwrite(&admins[i], sizeof(Users), 1, fp);
        printf("Admin written: %s\n", admins[i].username);
    }

    fclose(fp);
    printf("Admins successfully created in ADMIN_FILE\n");
}





int main() {
    
    return 0;
}
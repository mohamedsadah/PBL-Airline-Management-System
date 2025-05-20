#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <emscripten/emscripten.h>

#define MAX_FIELD_SIZE 128
// Store the credentials file under the persistent directory.
#define CRED_FILE "/persistent/users.dat"
#define FLIGHT_file "/persistent/Flight.dat"
#define ADMIN_FILE "/persistent/admin.dat"
#define BOOKINGS "/persistent/Bookings.dat"
#define PASSENGERS_file "/persistent/passengers.dat"

// Structure for Flight
typedef struct Flight
{
    char flightNumber[20];
    char departure[50];
    char destination[50];
    char departDate[20];
    char arrivalDate[20];
    char time[10];
    char arrTime[50];
    char stops[4][50];
    char status[10];
    int stopCount;
    int price;

    struct Flight *left, *right;
} Flight;

// admin
typedef struct
{
    char username[50];
    char password[50];
} Users;

typedef struct Passengers
{
    char Name[50];
    char PassportNumber[50];
    char flightNumber[50];
    char Nationality[50];
    char contact[15];
    struct Passengers *next;
} passenger;

// Root of the BST
Flight *root = NULL;

// head and tail of passengers list
passenger *P_head = NULL, *Tail = NULL;

int LoadingFromFile = 0;

char jsonBuffer[200000]; // for storing json format of flight data

// Create new flight node
Flight *createFlightNode(const char *fNum, const char *dep, const char *dest, const char *dDate, const char *aDate, const char *time, const char *atime, char *status, char stops[4][50], int stopCount, int price)
{
    Flight *newNode = (Flight *)malloc(sizeof(Flight));
    strcpy(newNode->flightNumber, fNum);
    strcpy(newNode->departure, dep);
    strcpy(newNode->destination, dest);
    strcpy(newNode->departDate, dDate);
    strcpy(newNode->arrivalDate, aDate);
    strcpy(newNode->time, time);
    strcpy(newNode->arrTime, atime);
    newNode->stopCount = stopCount;
    for (int i = 0; i < stopCount; i++)
    {
        strcpy(newNode->stops[i], stops[i]);
    }
    strcpy(newNode->status, status);
    newNode->price = price;
    newNode->left = NULL;
    newNode->right = NULL;
    return newNode;
}

// Insert into BST
int insertFlight(Flight **node, Flight *newNode)
{
    if (*node != NULL)
    {
        if (strcmp(newNode->flightNumber, (*node)->flightNumber) < 0)
        {
            return insertFlight(&(*node)->left, newNode);
        }
        else if (strcmp(newNode->flightNumber, (*node)->flightNumber) > 0)
        {
            return insertFlight(&(*node)->right, newNode);
        }
        else
        {
            return 0; // duplicate
        }
    }
    else
    {
        *node = newNode;
        return 1;
    }
}

// Save to file
int saveFlightToFile(Flight *flight)
{

    FILE *fp = fopen(FLIGHT_file, "ab"); // binary append
    if (!fp)
        return 0;

    size_t written = fwrite(flight, sizeof(Flight), 1, fp);
    fclose(fp);

    // Sync to IndexedDB after writing
    EM_ASM({
        FS.syncfs(false, function(err) {
            if (err) {
                console.error('Failed to sync to IndexedDB:', err);
            } else {
                console.log('Saved to IndexedDB.');
            } });
    });

    return written == 1; // return success (1) or failure (0)
}

// Exposed to JS
EMSCRIPTEN_KEEPALIVE
int addFlight(const char *fNum, const char *dep, const char *dest, const char *dDate, const char *aDate, const char *time, const char *atime, char **stopList, int stopCount, int price)
{
    char stops[4][50];
    for (int i = 0; i < stopCount && i < 4; i++)
    {
        strncpy(stops[i], stopList[i], 49);
        stops[i][49] = '\0';
        printf("Received stop %d: %s\n", i + 1, stops[i]); // DEBUG
    }

    char status[10] = "Active";
    Flight *newFlight = createFlightNode(fNum, dep, dest, dDate, aDate, time, atime, status, stops, stopCount, price);
    if (insertFlight(&root, newFlight))
    {
        if (saveFlightToFile(newFlight) == 0)
        {
            return -2;
        }
        return 1;
    }
    else
    {

        free(newFlight);
        return -1;
    }
}

void freeBST(Flight *node)
{
    if (node == NULL)
        return;
    freeBST(node->left);
    freeBST(node->right);
    free(node);
}

EMSCRIPTEN_KEEPALIVE
void loadFlightsFromFile()
{
    freeBST(root);

    FILE *fp = fopen(FLIGHT_file, "rb");
    if (!fp)
        return;
    Flight temp;
    while (fread(&temp, sizeof(Flight), 1, fp))
    {
        printf("%s", temp.flightNumber);

        Flight *node = createFlightNode(
            temp.flightNumber, temp.departure, temp.destination,
            temp.departDate, temp.arrivalDate, temp.time, temp.arrTime,
            temp.status, temp.stops, temp.stopCount, temp.price);

        insertFlight(&root, node);
        printf("Loading flight: %s\n", node->flightNumber);
    }
    fclose(fp);

    // Inline in-order traversal
    Flight *stack[100];
    int top = -1;
    Flight *curr = root;

    while (curr != NULL || top != -1)
    {
        while (curr != NULL)
        {
            stack[++top] = curr;
            curr = curr->left;
        }

        curr = stack[top--];

        // Print flight details
        printf("Flight Number: %s\n", curr->flightNumber);
        printf("Departure: %s -> Destination: %s\n", curr->departure, curr->destination);
        printf("Departure Date: %s | Arrival Date: %s | DepartTime: %s | ArrivalTime %s\n", curr->departDate, curr->arrivalDate, curr->time, curr->arrTime);
        printf("Stops (%d): ", curr->stopCount);
        printf("status (%s): ", curr->status);
        for (int i = 0; i < curr->stopCount; i++)
        {
            printf("%s ", curr->stops[i]);
        }
        printf("\n----------------------\n");

        curr = curr->right;
    }

    printf("BST Successfully Populated\n");
}

/*function to traverse BST and convert all data to json format,
so it can be displayed on web using JS */
void buildJSON(Flight *node, char *buffer)
{
    if (node == NULL)
        return;

    buildJSON(node->left, buffer); // traverse left subtree

    char flightEntry[4096];
    sprintf(flightEntry,
            "{ \"flightNumber\": \"%s\", \"departure\": \"%s\", \"destination\": \"%s\", "
            "\"departDate\": \"%s\", \"arrivalDate\": \"%s\", \"Dtime\": \"%s\", \"Atime\": \"%s\", \"status\": \"%s\", \"stopCount\": %d,\"price\": %d, \"stops\": [",
            node->flightNumber, node->departure, node->destination,
            node->departDate, node->arrivalDate, node->time, node->arrTime, node->status, node->stopCount, node->price);

    strcat(buffer, flightEntry);

    int addedStop = 0;
    for (int i = 0; i < node->stopCount; ++i)
    {
        if (strlen(node->stops[i]) > 0 && node->stops[i][0] >= 32 && node->stops[i][0] <= 126)
        {
            if (addedStop > 0)
                strcat(buffer, ", ");
            strcat(buffer, "\"");
            strcat(buffer, node->stops[i]);
            strcat(buffer, "\"");
            addedStop++;
        }
    }

    strcat(buffer, "] },");
    buildJSON(node->right, buffer); // traverse right subtree
}

void buildSearchJSON(Flight *node, char *buffer, const char *departure, const char *destination, const char *date)
{
    if (node == NULL)
        return;

    buildSearchJSON(node->left, buffer, departure, destination, date); // traverse left subtree

    if (strcmp(node->departure, departure) == 0 &&
        strcmp(node->destination, destination) == 0 &&
        strcmp(node->departDate, date) == 0)
    {
        char flightEntry[4096];
        sprintf(flightEntry,
                "{ \"flightNumber\": \"%s\", \"departure\": \"%s\", \"destination\": \"%s\", "
                "\"departDate\": \"%s\", \"arrivalDate\": \"%s\", \"Dtime\": \"%s\", \"Atime\": \"%s\", \"status\": \"%s\", \"stopCount\": %d,\"price\": %d, \"stops\": [",
                node->flightNumber, node->departure, node->destination,
                node->departDate, node->arrivalDate, node->time, node->arrTime, node->status, node->stopCount, node->price);

        strcat(buffer, flightEntry);

        int addedStop = 0;
        for (int i = 0; i < node->stopCount; ++i)
        {
            if (strlen(node->stops[i]) > 0 && node->stops[i][0] >= 32 && node->stops[i][0] <= 126)
            {
                if (addedStop > 0)
                    strcat(buffer, ", ");
                strcat(buffer, "\"");
                strcat(buffer, node->stops[i]);
                strcat(buffer, "\"");
                addedStop++;
            }
        }

        strcat(buffer, "] },");
    }
    buildSearchJSON(node->right, buffer, departure, destination, date);
}

EMSCRIPTEN_KEEPALIVE
const char *getAllFlightsJSON()
{
    strcpy(&jsonBuffer[0], "[");
    jsonBuffer[1] = '\0';

    buildJSON(root, jsonBuffer + 1);

    // Remove trailing comma
    int len = strlen(jsonBuffer);
    if (len > 1 && jsonBuffer[len - 1] == ',')
    {
        jsonBuffer[len - 1] = '\0';
    }

    strcat(jsonBuffer, "]");
    return jsonBuffer;
}

EMSCRIPTEN_KEEPALIVE
const char *getOneWayFlightsJSON(const char *departure, const char *destination, const char *date)
{
    strcpy(&jsonBuffer[0], "[");
    jsonBuffer[1] = '\0';

    buildSearchJSON(root, jsonBuffer + 1, departure, destination, date);

    // Remove trailing comma
    int len = strlen(jsonBuffer);
    if (len > 1 && jsonBuffer[len - 1] == ',')
    {
        jsonBuffer[len - 1] = '\0';
    }

    strcat(jsonBuffer, "]");
    return jsonBuffer;
}

// Function to register a new user by appending the username and password.
// Credentials are stored as "username:password" per line.
EMSCRIPTEN_KEEPALIVE
int register_user(const char *username, const char *password)
{
    FILE *file = fopen(CRED_FILE, "ab");
    if (!file)
    {
        return -1;
    }

    Users user;
    strncpy(user.username, username, sizeof(user.username));
    strncpy(user.password, password, sizeof(user.password));
    fwrite(&user, sizeof(user), 1, file);
    fclose(file);

    EM_ASM({
        FS.syncfs(false, function(err) {
            if (err) {
                console.error('Failed to sync to IndexedDB:', err);
            } else {
                console.log('Saved to IndexedDB.');
            } });
    });

    return 0;
}

// Function to verify login credentials.
EMSCRIPTEN_KEEPALIVE
int login(const char *username, const char *password)
{
    if (strcmp(username, "admin") == 0)
    {

        FILE *adminFile = fopen(ADMIN_FILE, "rb");
        if (!adminFile)
            return -2;

        Users admin;
        while (fread(&admin, sizeof(Users), 1, adminFile) == 1)
        {
            // retrive and check if password match any password in file
            if (strcmp(admin.password, password) == 0)
            {
                fclose(adminFile);
                return 1; // success
            }
        }

        fclose(adminFile);
        return 0; // login failed
    }

    else
    {

        FILE *file = fopen(CRED_FILE, "rb");
        if (!file)
        {
            return -2;
        }

        Users user;
        while (fread(&user, sizeof(user), 1, file) == 1)
        {
            if (strcmp(user.username, username) == 0 && strcmp(user.password, password) == 0)
            {
                fclose(file);
                return 1; // success
            }
        }

        fclose(file);
        return 0; // Login failed.
    }

    
}

EMSCRIPTEN_KEEPALIVE
void createInitialAdmins()
{
    FILE *fp = fopen(ADMIN_FILE, "wb");
    if (!fp)
    {
        printf("Failed to open admin.dat\n");
        return;
    }

    Users admins[] = {
        {"admin", "mcac"},
        {"admin", "mscit"},
        {"admin", "mcab"},
        {"admin", "mcaa"},
        {"admin", "mentor"}};

    int count = sizeof(admins) / sizeof(admins[0]);

    for (int i = 0; i < count; i++)
    {
        fwrite(&admins[i], sizeof(Users), 1, fp);
        printf("Admin written: %s\n", admins[i].username);
    }

    fclose(fp);
    printf("Admins successfully created in ADMIN_FILE\n");

    EM_ASM({
        FS.syncfs(false, function(err) {
            if (err) {
                console.error('Failed to sync to IndexedDB:', err);
            } else {
                console.log('Saved to IndexedDB.');
            } });
    });
}

void saveFlights(Flight *flight, FILE *fp)
{
    if (flight != NULL)
    {
        saveFlights(flight->left, fp);
        fwrite(flight, sizeof(Flight), 1, fp);
        saveFlights(flight->right, fp);
    }
}

int saveAfterUpdate(Flight *flight)
{
    FILE *fp = fopen(FLIGHT_file, "wb"); // truncate
    if (!fp)
        return 0;

    if (flight != NULL)
    {
        saveFlights(flight, fp);
    }

    fclose(fp);

    // Sync to IndexedDB after writing
    EM_ASM({
        FS.syncfs(false, function(err) {
            if (err) {
                console.error('Failed to sync to IndexedDB:', err);
            } else {
                console.log('Saved to IndexedDB.');
            } });
    });

    // debug
    FILE *check = fopen(FLIGHT_file, "rb");
    fseek(check, 0, SEEK_END);
    long size = ftell(check);
    printf("DEBUG: File size after save = %ld bytes\n", size);
    fclose(check);

    return 1;
}

EMSCRIPTEN_KEEPALIVE
int updateFlight(const char *departuredate, const char *arrivaldate, const char *time, const char *atime, const char *status, const char *fnum)
{
    if (root != NULL)
    {
        printf("Data received: %s %s %s %s %s %s\n", departuredate, arrivaldate, time, atime, status, fnum);
        Flight *current = root;

        while (current != NULL && strcmp(current->flightNumber, fnum) != 0)
        {
            current = (strcmp(fnum, current->flightNumber) < 0) ? current->left : current->right;
        }

        // this may never run, because function is called in c only if flight exists
        if (current == NULL)
        {
            return -1;
        }

        // Update fields only if not empty
        if (departuredate != NULL && strlen(departuredate) > 0)
        {
            strcpy(current->departDate, departuredate);
        }
        if (arrivaldate != NULL && strlen(arrivaldate) > 0)
        {
            strcpy(current->arrivalDate, arrivaldate);
        }
        if (time != NULL && strlen(time) > 0)
        {
            strcpy(current->time, time);
        }
        if (status != NULL && strlen(status) > 0)
        {
            strcpy(current->status, status);
        }

        if (atime != NULL && strlen(atime) > 0)
        {
            strcpy(current->arrTime, atime);
        }

        saveAfterUpdate(root); // rewrite updated flight to file
        return 1;              // Success
    }

    return -2; // BST empty
}

int deleteFlight(const char *flightNumber)
{
    if (root == NULL)
        return -1; // no flights available
    Flight *current = root, *parent = NULL;

    while (current != NULL && strcmp(current->flightNumber, flightNumber) != 0)
    {
        parent = current;
        current = (strcmp(flightNumber, current->flightNumber) < 0) ? current->left : current->right;
    }

    if (current == NULL)
    {
        return 0; // flight not found;
    }

    if (current->left == NULL || current->right == NULL)
    {
        Flight *child = (current->left != NULL) ? current->left : current->right;
        printf("child: %s\n", child->flightNumber); //debug

        if (parent == NULL)
        {
            root = child;
            if (child == NULL)
            {
                remove(FLIGHT_file);
                EM_ASM({
                    FS.syncfs(true, function(err) {
                        if (err) {
                            console.error('Failed to sync to IndexedDB:', err);
                        } else {
                            console.log('Saved to IndexedDB.');
                        } });
                });
            }
        }
        else
        {
            if (parent->left == current)
                parent->left = child;
            else
                parent->right = child;
        }

        free(current);
        saveAfterUpdate(root);
        return 1;
    }

    Flight *succParent = current, *succ = current->right;
    while (succ->left != NULL)
    {
        succParent = succ;
        succ = succ->left;
    }

    strcpy(current->flightNumber, succ->flightNumber);
    strcpy(current->departure, succ->departure);
    strcpy(current->destination, succ->destination);
    strcpy(current->departDate, succ->departDate);
    strcpy(current->arrivalDate, succ->arrivalDate);
    strcpy(current->time, succ->time);
    strcpy(current->arrTime, succ->arrTime);
    for (int i = 0; i < succ->stopCount; i++)
    {
        strcpy(current->stops[i], succ->stops[i]);
    }
    strcpy(current->status, succ->status);
    current->stopCount = succ->stopCount;
    current->price = succ->price;

    if (succParent->left == succ)
        succParent->left = succ->right;
    else
        succParent->right = succ->right;

    free(succ);

    saveAfterUpdate(root);
    return 1;
}

int savePassengers(passenger *p){
        FILE *pfile = fopen(PASSENGERS_file, "ab");
        if(!pfile) return -1;

        fwrite(p, sizeof(passenger), 1, pfile);
            
        fclose(pfile);

        EM_ASM({
            FS.syncfs(false, function(err) {
                if (err) {
                    console.error('Failed to sync to IndexedDB:', err);
                } else {
                    console.log('Saved to IndexedDB.');
                } });
        });


    return 1;
}

EMSCRIPTEN_KEEPALIVE
int addPassenger(const char *name, const char *passportno, const char *fln, const char *nationality, const char *contact){
    passenger *p = (passenger*)malloc(sizeof(passenger));
    if(!p) return -1;

    strcpy(p->Name, name);
    strcpy(p->PassportNumber, passportno);
    strcpy(p->flightNumber, fln);
    strcpy(p->Nationality, nationality);
    strcpy(p->contact, contact);
    p->next = NULL;

    if(P_head == NULL){
        P_head = p;
        Tail = p;
    }else{
        Tail->next = p;
        Tail = p;
    }
    
    if(!LoadingFromFile){
        if(savePassengers(p) == 1) return 2;

    }
    
    return 1;
}

void freeList(){
    passenger *current = P_head;
    while (current != NULL) {
        passenger *temp = current;
        current = current->next;
        free(temp);
    }
    P_head = Tail = NULL;
}

EMSCRIPTEN_KEEPALIVE
void loadPassengers() {
    FILE *pfile = fopen(PASSENGERS_file, "rb");
    if (!pfile) {
        printf("Could not open file");
        return;
    }

    freeList();

    LoadingFromFile = 1;

    passenger temp;
    while (fread(&temp, sizeof(passenger), 1, pfile) == 1) {

        addPassenger(temp.Name, temp.PassportNumber, 
            temp.flightNumber, temp.Nationality ,temp.contact);
        
    }

    fclose(pfile);

    LoadingFromFile = 0;

    passenger *a = P_head;
    while(a!=NULL){
        printf("Name: %s\n", a->Name);
        printf("Passport: %s\n", a->PassportNumber);
        printf("Flight: %s\n", a->flightNumber);
        printf("Nationality: %s\n", a->Nationality);
        printf("Contact: %s\n", a->contact);
        printf("---------------------------\n");
        a=a->next;
    }
    free(a);
    
}

EMSCRIPTEN_KEEPALIVE
char* getAllPassengersJSON() {
    passenger *current = P_head;
    char* json = malloc(5000); // Allocate buffer for JSON string
    if (!json) return NULL;

    
    
    strcpy(json, "[");
    
    while (current) {
        printf("loading passengers.%s\n", current->Name);

        char passengerJSON[1024];
        snprintf(passengerJSON, sizeof(passengerJSON),
        "{\"Name\":\"%s\",\"PassportNumber\":\"%s\",\"flightNumber\":\"%s\",\"Nationality\":\"%s\",\"contact\":\"%s\"}",
        current->Name, current->PassportNumber, current->flightNumber, current->Nationality, current->contact);
        
        strcat(json, passengerJSON);
        if (current->next) strcat(json, ",");
        
        current = current->next;
    }

    strcat(json, "]");
    return json; // Return pointer to JSON string 
}



int main()
{

    return 0;
}
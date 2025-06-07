#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <emscripten/emscripten.h>

#define MAX_FIELD_SIZE 128
#define max 5
#define BUFFER_SIZE 5

// Store the credentials file under the persistent directory.
#define CRED_FILE "/persistent/users.dat"
#define FLIGHT_file "/persistent/Flight.dat"
#define ADMIN_FILE "/persistent/admin.dat"
#define BOOKINGS "/persistent/Bookings.dat"
#define NOTIFICATIONS_FILE "/persistent/notif.dat"
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
    int capacity;
    int availableSeats;
    int stopCount;
    int price;

    struct Flight *left, *right;
} Flight;

// admin
typedef struct
{
    char username[50];
    char passport[50];
    char password[50];
} Users;

typedef struct Passengers
{
    char Name[50];
    char PassportNumber[50];
    char flightNumber[50];
    char Nationality[50];
    char seatNo[10];
    char contact[15];
    struct Passengers *next;
} passenger;

typedef struct notifications
{
    char passNum[50];
    char flightno[50];
    char type[20];
} Notif;



// Root of the BST for flights and User-specific booked flight
Flight *root = NULL, *PR_root = NULL;

// head and tail of passengers list
passenger *P_head = NULL, *Tail = NULL;


char released_seats[max][BUFFER_SIZE];

static int seat_index = 0;
static int release_index = 0;


int LoadingFromFile = 0;

// basic seats simulation
void seatsGenerator(char seats[][BUFFER_SIZE]) {

    int current_number_part = 1;
    int current_char_offset = 0;

    for (int i = 0; i < max*max; i++) {
        char current_char_val = 'A' + current_char_offset;
        sprintf(seats[i], "%d%c", current_number_part, current_char_val);

        current_char_offset++;

        if (current_char_offset >= 5) {
            current_char_offset = 0;
            current_number_part++;
        }
    }

    printf("\nSeats Generated: ");
    for (int i = 0; i < max*max; i++) {
        printf("%s ", seats[i]);
    }
    printf("\n");

}



// Create new flight node
Flight *createFlightNode
    (const char *fNum, const char *dep, const char *dest, const char *dDate, 
    const char *aDate, const char *time, const char *atime,const char *status, 
    char stops[4][50], int stopCount, int price, int cap, int avail)
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
    newNode->capacity = cap;
    newNode->availableSeats = avail;
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

    char status[10] = "OnTime";
    int flight_cap = max * max;
    Flight *newFlight = createFlightNode(fNum, dep, dest, dDate, aDate, time, 
                        atime, status, stops, stopCount, price, flight_cap, flight_cap);

    if (insertFlight(&root, newFlight))
    {
        if (saveFlightToFile(newFlight) == 0)
        {
            return -2;
        }
        printf("Flight: %s\nCapacity: %d\nAvailable: %d\n", fNum, newFlight->capacity, newFlight->availableSeats);
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
            temp.status, temp.stops, temp.stopCount, temp.price,
            temp.capacity, temp.availableSeats);

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
        printf("seats Available: %d", curr->availableSeats);
        printf("capacity: %d", curr->capacity);
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
char* buildJSONRecursive(Flight* root, size_t* length, int* first, size_t* capacity, char** json) {
    if (!root) return *json;

    // Traverse left
    buildJSONRecursive(root->left, length, first, capacity, json);

    // Prepare stops array as string
    char stopsBuffer[512] = "[";
    for (int i = 0; i < root->stopCount; i++) {
        strcat(stopsBuffer, "\"");
        strcat(stopsBuffer, root->stops[i]);
        strcat(stopsBuffer, "\"");
        if (i < root->stopCount - 1)
            strcat(stopsBuffer, ",");
    }
    strcat(stopsBuffer, "]");

    // Prepare current node JSON
    char buffer[2048];
    snprintf(buffer, sizeof(buffer),
        "%s{\"flightNumber\":\"%s\",\"departure\":\"%s\",\"destination\":\"%s\","
        "\"departDate\":\"%s\",\"arrivalDate\":\"%s\",\"Dtime\":\"%s\",\"Atime\":\"%s\","
        "\"status\":\"%s\",\"stops\":%s,\"stopCount\":%d,\"seats\":%d,\"price\":%d}",
        *first ? "" : ",",
        root->flightNumber, root->departure, root->destination,
        root->departDate, root->arrivalDate, root->time, root->arrTime,
        root->status, stopsBuffer, root->stopCount, root->availableSeats, root->price
    );

    size_t bufLen = strlen(buffer);
    if (*length + bufLen + 2 > *capacity) {
        *capacity *= 2;
        *json = realloc(*json, *capacity);
        if (!*json) return NULL;
    }

    strcat(*json, buffer);
    *length += bufLen;
    *first = 0;

    // Traverse right
    buildJSONRecursive(root->right, length, first, capacity, json);

    return *json;
}


char* buildSearchJSONRecursive(Flight* root, size_t* length, int* first, size_t* capacity, 
    char** json, const char *departure, const char *destination, const char *date) 
    {
    if (!root) return *json;

    // Traverse left
    buildSearchJSONRecursive(root->left, length, first, capacity, json, departure, destination, date);

    if (strcmp(root->departure, departure) == 0 &&
        strcmp(root->destination, destination) == 0 &&
        strcmp(root->departDate, date) == 0)
    {
        char stopsBuffer[512] = "[";
        for (int i = 0; i < root->stopCount; i++) {
            strcat(stopsBuffer, "\"");
            strcat(stopsBuffer, root->stops[i]);
            strcat(stopsBuffer, "\"");
            if (i < root->stopCount - 1)
                strcat(stopsBuffer, ",");
        }
        strcat(stopsBuffer, "]");
    
        // Prepare current node JSON
        char buffer[1024];
        snprintf(buffer, sizeof(buffer),
            "%s{\"flightNumber\":\"%s\",\"departure\":\"%s\",\"destination\":\"%s\","
            "\"departDate\":\"%s\",\"arrivalDate\":\"%s\",\"Dtime\":\"%s\",\"Atime\":\"%s\","
            "\"status\":\"%s\",\"stops\":%s,\"stopCount\":%d,\"seats\":%d,\"price\":%d}",
            *first ? "" : ",",
            root->flightNumber, root->departure, root->destination,
            root->departDate, root->arrivalDate, root->time, root->arrTime,
            root->status, stopsBuffer, root->stopCount, root->availableSeats, root->price
        );
    
        size_t bufLen = strlen(buffer);
        if (*length + bufLen + 2 > *capacity) {
            *capacity *= 2;
            *json = realloc(*json, *capacity);
            if (!*json) return NULL;
        }
    
        strcat(*json, buffer);
        *length += bufLen;
        *first = 0;
    
    }
    // Traverse right
    buildSearchJSONRecursive(root->right, length, first, capacity, json, departure, destination, date);


    return *json;
}

EMSCRIPTEN_KEEPALIVE
const char *getAllFlightsJSON()
{
    size_t capacity = 4096;
    size_t length = 1;
    int first = 1;

    char* json = malloc(capacity);
    if (!json) return NULL;

    strcpy(json, "[");

    char* result = buildJSONRecursive(root, &length, &first, &capacity, &json);
    if (!result) return NULL;

    strcat(json, "]");
    return json;
}

EMSCRIPTEN_KEEPALIVE
const char *getOneWayFlightsJSON(const char *departure, const char *destination, const char *date)
{
    size_t capacity = 4096;
    size_t length = 1;
    int first2 = 1;

    char* json2 = malloc(capacity);
    if (!json2) return NULL;

    strcpy(json2, "[");

    char* result = buildSearchJSONRecursive(root, &length, &first2, &capacity, &json2, departure, destination, date);
    if (!result) return NULL;

    strcat(json2, "]");
    return json2;
}

// Function to register a new user by appending the username and password.
// Credentials are stored as "username:password" per line.
EMSCRIPTEN_KEEPALIVE
int register_user(const char *username, const char *password, const char *passport)
{
    FILE *file = fopen(CRED_FILE, "ab+");
    if (!file)
    {
        return -1;
    }

    int exists = 0;

    Users user, check;

    while(fread(&check, sizeof(Users), 1, file) == 1){
        if(strcmp(check.username, username) == 0){
            exists = 1;
            break;
        }
    }

    if(exists){
        fclose(file);
        return -2;
    }

    strncpy(user.username, username, sizeof(user.username));
    strncpy(user.password, password, sizeof(user.password));
    strncpy(user.passport, passport, sizeof(user.passport));


    fwrite(&user, sizeof(user), 1, file);
    fclose(file);

    

    return 0;
}

// Function to verify login credentials.
EMSCRIPTEN_KEEPALIVE
int login(const char *username, const char *password, const char *passport)
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
            if (strcmp(user.username, username) == 0 && 
                strcmp(user.password, password) == 0 && 
                strcmp(user.passport, passport) == 0)
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

int saveAfterUpdate(Flight *flight, const char *file)
{
    FILE *fp = fopen(file, "wb"); // truncate
    if (!fp)
        return 0;

    printf("file %s accessed \n", file);


    if (flight != NULL)
    {
        saveFlights(flight, fp);
    }

    fclose(fp);

    // Sync to IndexedDB after writing
    EM_ASM({
        FS.syncfs(true, function(err) {
            if (err) {
                console.error('Failed to sync to IndexedDB:', err);
            } else {
                console.log('Saved to IndexedDB.');
            } });
    });

    // debug
    FILE *check = fopen(file, "rb");
    fseek(check, 0, SEEK_END);
    long size = ftell(check);
    printf("DEBUG: File size after save = %ld bytes\n", size);
    fclose(check);

    return 1;
}

EMSCRIPTEN_KEEPALIVE
int updateFlight(const char *rt, const char *file, const char *departuredate, const char *arrivaldate, const char *time, const char *atime, const char *status, const char *fnum)
{
    Flight *Root = NULL;

    printf("value of root is %s \n", rt);

    if(strcmp(rt, "PR")==0){
        Root = PR_root;
    }else{
        Root = root;
    }

    if(strcmp(file, "file")==0) file = FLIGHT_file;


    if (Root != NULL)
    {
        printf("Data received: %s %s %s %s %s %s\n", departuredate, arrivaldate, time, atime, status, fnum);
        Flight *current = Root;

        while (current != NULL && strcmp(current->flightNumber, fnum) != 0)
        {
            current = (strcmp(fnum, current->flightNumber) < 0) ? current->left : current->right;
        }

        
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

        saveAfterUpdate(Root, file); // rewrite updated flight to file
        return 1;              // Success
    }

    return -2; // BST empty
}

int deleteFlight(const char *flightNumber, const char *rt, const char *file)
{
    Flight *Root = NULL;

    printf("value of root is %s \n", rt);

    if(strcmp(rt, "PR")==0){
        Root = PR_root;
    }else{
        Root = root;
    }

    if(strcmp(file, "file")==0) file = FLIGHT_file;

    if (Root == NULL)
        return -1; // no flights available
    Flight *current = Root, *parent = NULL;

    printf("file %s received \n", file);

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
            Root = child;
            if (child == NULL)
            {
                remove(file);
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
        saveAfterUpdate(Root, file);
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
    current->availableSeats = succ->availableSeats;
    current->capacity = succ->capacity;
    current->price = succ->price;

    if (succParent->left == succ)
        succParent->left = succ->right;
    else
        succParent->right = succ->right;

    free(succ);

    saveAfterUpdate(Root, file);
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

    char seats[max*max][BUFFER_SIZE];

    printf("%d %d", seat_index, release_index);

    seatsGenerator(seats);

    strcpy(p->Name, name);
    strcpy(p->PassportNumber, passportno);
    strcpy(p->flightNumber, fln);
    strcpy(p->Nationality, nationality);
    strcpy(p->contact, contact);
    printf("\ndebug out");
    if(seat_index < max*max) {
        strcpy(p->seatNo, seats[seat_index++]);
        printf("\ndebug: seat assigned %s", p->seatNo);
    }
    else if(release_index > 0) {
        strcpy(p->seatNo, released_seats[release_index--]);
        printf("\ndebug: seat assigned from released");

    }
    else {
        printf("\n no seats available\n");
    }
    p->next = NULL;

    printf("\ndebug: seat assignment complete");
    
    if(P_head == NULL){
        P_head = p;
        Tail = p;
    }else{
        Tail->next = p;
        Tail = p;
    }
    
    if(!LoadingFromFile){


        Flight *current = root, *parent = NULL;

        while (current != NULL && strcmp(current->flightNumber, fln) != 0)
        {
            parent = current;
            current = (strcmp(fln, current->flightNumber) < 0) ? current->left : current->right;
        }

        if (current != NULL)
        {
            printf("\nflight Found..\n");
            current->availableSeats-=1;
            saveAfterUpdate(root, FLIGHT_file);
        }
       

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
    free(current);
}

EMSCRIPTEN_KEEPALIVE
void loadPassengers() {

    freeList();

    FILE *file = fopen(PASSENGERS_file, "rb");
    if (!file) return;

    passenger temp;
    while (fread(&temp, sizeof(passenger), 1, file)) {
        passenger *p = malloc(sizeof(passenger));
        *p = temp;
        p->next = NULL;

        if (P_head == NULL) {
            P_head = p;
            Tail = p;
        } else {
            Tail->next = p;
            Tail = p;
        }
    }

    fclose(file);


    passenger *a = P_head;
    while(a!=NULL){
        printf("Name: %s\n", a->Name);
        printf("Passport: %s\n", a->PassportNumber);
        printf("Flight: %s\n", a->flightNumber);
        printf("Nationality: %s\n", a->Nationality);
        printf("Contact: %s\n", a->contact);
        printf("seat: %s\n", a->seatNo);
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

        char passengerJSON[2048];
        snprintf(passengerJSON, sizeof(passengerJSON),
        "{\"Name\":\"%s\",\"PassportNumber\":\"%s\",\"flightNumber\":\"%s\",\"Nationality\":\"%s\",\"contact\":\"%s\",\"seatNo\":\"%s\"}",
        current->Name, current->PassportNumber, current->flightNumber, current->Nationality, current->contact, current->seatNo);
        
        strcat(json, passengerJSON);
        if (current->next) strcat(json, ",");
        
        current = current->next;
    }

    strcat(json, "]");
    return json; // Return pointer to JSON string 
}

int createPassengerRecords(
    const char *filename, const char *fNum, const char *dep, const char *dest, 
    const char *dDate, const char *aDate, const char *time, const char *atime, 
    const char *status, char **stopList, int stopCount, int price, int avail)
    
{
    char path[256];
    snprintf(path, sizeof(path), "/persistent/%s.dat", filename);

    FILE *file = fopen(path, "ab+");
    if(!file) return -1;

    int exists = 0;

    Flight temp;
    rewind(file);

    while(fread(&temp, sizeof(Flight), 1, file) == 1){
        if(strcmp(temp.flightNumber, fNum) == 0 &&
            strcmp(temp.departDate, dDate) == 0){
                exists = 1;
                fclose(file);
                break;
            }
    }

    if(exists) return -2;

    char stops[4][50];
    for (int i = 0; i < stopCount && i < 4; i++)
    {
        strncpy(stops[i], stopList[i], 49);
        stops[i][49] = '\0';
        printf("Received stop %d: %s\n", i + 1, stops[i]); // DEBUG
    }

    Flight *record = createFlightNode(fNum, dep, dest, dDate, 
        aDate, time, atime, status, stops, stopCount, price, max*max, avail);

    fwrite(record, sizeof(Flight), 1, file);
    fclose(file);

    printf("Flight %s on %s successfully added.\n", fNum, dDate);

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

void loadUserRecord(const char *filename)
{
    freeBST(PR_root);

    char path[256];
    snprintf(path, sizeof(path), "/persistent/%s.dat", filename);
    FILE *fp = fopen(path, "rb");
    if (!fp) return;

    Flight temp;
    while (fread(&temp, sizeof(Flight), 1, fp))
    {
        printf("%s", temp.flightNumber);

        Flight *node = createFlightNode(
            temp.flightNumber, temp.departure, temp.destination,
            temp.departDate, temp.arrivalDate, temp.time, temp.arrTime,
            temp.status, temp.stops, temp.stopCount, temp.price, temp.capacity, temp.availableSeats);

        insertFlight(&PR_root, node);
        printf("Loading flight: %s\n", node->flightNumber);
    }
    fclose(fp);
}

EMSCRIPTEN_KEEPALIVE
const char *getPassengerRecJSON()
{
    size_t capacity = 4096;
    size_t length = 1;
    int first = 1;

    char* json = malloc(capacity);
    if (!json) return NULL;

    strcpy(json, "[");

    char* result = buildJSONRecursive(PR_root, &length, &first, &capacity, &json);
    if (!result) return NULL;

    strcat(json, "]");
    return json;
}

EMSCRIPTEN_KEEPALIVE
void deleteBookRecord(const char *flightNumberToDelete) {
    DIR *dir;
    struct dirent *entry;

    dir = opendir("/persistent");
    if (!dir) {
        perror("Could not open directory");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        // Only check files ending in .dat (skip flight.dat itself)
        if (strstr(entry->d_name, ".dat") && strcmp(entry->d_name, "flight.dat") != 0) {
            char path[256], tempPath[256];
            snprintf(path, sizeof(path), "/persistent/%s", entry->d_name);
            snprintf(tempPath, sizeof(tempPath), "/persistent/%s_tmp", entry->d_name);

            FILE *src = fopen(path, "rb");
            FILE *temp = fopen(tempPath, "wb");

            printf("Accessing %s\n", path);
            printf("created %s\n", tempPath);


            if (!src || !temp) {
                if (src) fclose(src);
                if (temp) fclose(temp);
                continue;
            }

            Flight p;
            int deleted = 0;
            while (fread(&p, sizeof(Flight), 1, src)) {
                if (strcmp(p.flightNumber, flightNumberToDelete) == 0) {
                    printf("flight to delete found - %s", p.flightNumber);
                    deleted = 1;
                    continue; // Skip this record
                }
                fwrite(&p, sizeof(Flight), 1, temp);
            }

            fclose(src);
            fclose(temp);

            printf("deleted: %d", deleted);

            if (deleted) {
                remove(path);           // Delete old file
                rename(tempPath, path); // Replace with new one
                printf("Removed flight %s from %s\n", flightNumberToDelete, entry->d_name);
            } else {
                remove(tempPath);       // No changes needed
            }
        }
    }

    closedir(dir);
}

EMSCRIPTEN_KEEPALIVE
int deletePassenger(const char *passportNumber, const char *flightNumber) {
    passenger *curr = P_head;
    passenger *prev = NULL;

    printf("\nData recieved %s %s\n", passportNumber, flightNumber);
    while (curr != NULL) {
        printf("\n %s %s", curr->PassportNumber, curr->flightNumber);

        


        if (strcmp(curr->PassportNumber, passportNumber) == 0 &&
            strcmp(curr->flightNumber, flightNumber) == 0) {
             printf("\nrecord found..\n");

            Flight *current = root, *parent = NULL;

            while (current != NULL && strcmp(current->flightNumber, flightNumber) != 0)
            {
                parent = current;
                current = (strcmp(flightNumber, current->flightNumber) < 0) ? current->left : current->right;
            }

            if (current != NULL)
            {
                printf("\n increasing seats...\n");
                if(current->availableSeats < current->capacity) current->availableSeats++;
                if(release_index < current->capacity){
                    strcpy(released_seats[release_index++], curr->seatNo);
                }
                saveAfterUpdate(root, FLIGHT_file);
            }else{
                printf("\nflight not found\n");
            }

            if (prev == NULL) {
                P_head = curr->next;  
                if (P_head == NULL) { 
                    Tail = NULL;
                    remove(PASSENGERS_file);
                }
            } else {
                prev->next = curr->next;
                if (curr->next == NULL) { 
                    Tail = prev; 
                }
            }

            free(curr);
            printf("\nrecord deleted..\n");

            FILE *pfile = fopen(PASSENGERS_file, "wb");
            if (!pfile) {
                perror("Failed to open file");
                return -1;
            }

            printf("\nFile %s accessed\n", PASSENGERS_file);

            passenger *ptr = P_head;
            while (ptr != NULL) {
                fwrite(ptr, sizeof(passenger), 1, pfile);
                ptr = ptr->next;
            }
            fclose(pfile);
            free(ptr);

            EM_ASM({
                FS.syncfs(true, function(err) {
                    if (err) {
                        console.error('Failed to sync to IndexedDB:', err);
                    } else {
                        console.log('Saved to IndexedDB.');
                    } });
            });

            return 1;
        }

        prev = curr;
        curr = curr->next;
    }

    return 0; 
}

EMSCRIPTEN_KEEPALIVE
int CancellationNotif(const char* cancelledFlightNumber, const char *type) {

    printf("flight received: %s\n",cancelledFlightNumber);


    FILE *pfile = fopen(PASSENGERS_file, "rb");
    if (!pfile) {
        perror("Failed to open passengers file");
        return -1;
    }

    FILE *nfile = fopen(NOTIFICATIONS_FILE, "ab");
    if (!nfile) {
        perror("Failed to open notifications file");
        fclose(pfile);
        return -1;
    }

    printf("\nAccessed %s\n%s", PASSENGERS_file, NOTIFICATIONS_FILE);

    passenger p;
    int count = 0;

    while (fread(&p, sizeof(passenger), 1, pfile)) {
        printf("\nloading passenger with flight %s\n", p.flightNumber);
        if (strcmp(p.flightNumber, cancelledFlightNumber) == 0) {
            Notif notif;

            strncpy(notif.passNum, p.PassportNumber, sizeof(notif.passNum));
            strncpy(notif.flightno, p.flightNumber, sizeof(notif.flightno));
            strncpy(notif.type, type, sizeof(notif.type));

            fwrite(&notif, sizeof(Notif), 1, nfile);
            printf("notification added for %s with type %s\n", p.PassportNumber, notif.type);
            count++;
        }
    }

    fclose(pfile);
    fclose(nfile);
    return count; // number of notifications written just to debug
}

EMSCRIPTEN_KEEPALIVE
char* loadNotificationsJSON() {
    FILE *file = fopen(NOTIFICATIONS_FILE, "rb");
    if (!file) {
        perror("Failed to open notifications file");
        return NULL;
    }

    // Allocate initial buffer
    size_t capacity = 1024;
    char *json = malloc(capacity);
    if (!json) {
        fclose(file);
        return NULL;
    }
    json[0] = '\0';

    strcat(json, "[");
    int first = 1;

    Notif temp;
    while (fread(&temp, sizeof(Notif), 1, file)) {
        // Escape values if needed here
        char entry[512];
        snprintf(entry, sizeof(entry),
                 "%s{\"passportNo\":\"%s\",\"flightNo\":\"%s\",\"type\":\"%s\"}",
                 first ? "" : ",",
                 temp.passNum, temp.flightno, temp.type);

        // Ensure capacity is enough
        if (strlen(json) + strlen(entry) + 2 >= capacity) {
            capacity *= 2;
            char *newJson = realloc(json, capacity);
            if (!newJson) {
                free(json);
                fclose(file);
                return NULL;
            }
            json = newJson;
        }

        strcat(json, entry);
        first = 0;
    }

    strcat(json, "]");
    fclose(file);

    // Optional: shrink memory to fit actual usage
    char *finalJson = realloc(json, strlen(json) + 1);
    return finalJson ? finalJson : json;
}

EMSCRIPTEN_KEEPALIVE
int deleteNotificationsForUser(const char *passportNo) {
    FILE *infile = fopen(NOTIFICATIONS_FILE, "rb");
    if (!infile) return -1;

    FILE *tempfile = fopen("/persistent/temp.dat", "wb");
    if (!tempfile) {
        fclose(infile);
        return -1;
    }

    Notif notif;
    int deleted = 0;

    while (fread(&notif, sizeof(Notif), 1, infile)) {
        printf("Comparing: '%s' and '%s'\n", notif.passNum, passportNo);
        if (strcmp(notif.passNum, passportNo) == 0) {
            deleted++;
            printf("\n Notification Deleted");
            continue;
        } else {
            fwrite(&notif, sizeof(Notif), 1, tempfile);
            printf("\n writing %s", notif.passNum);
            
        }
    }

    fclose(infile);
    fclose(tempfile);

    remove(NOTIFICATIONS_FILE);
    rename("/persistent/temp.dat", NOTIFICATIONS_FILE);

    return deleted;
}

EMSCRIPTEN_KEEPALIVE
int main() {
    
}


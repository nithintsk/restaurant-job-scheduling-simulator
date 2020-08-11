#include <iostream>
#include <pthread.h>
#include <stdlib.h>
#include <fstream>
#include "utility.h"
#include <cerrno>
#include <time.h>
#include <sstream>

using namespace std;

int main(int argc, char* argv[]) {

    /* Take input in a file */
    if(argc != 2) {
        errno = EINVAL;
        perror("Error Encountered");
        printUsage();
        exit(-1);
    }

    /* Open input file */
    ifstream infile;
    infile.open(argv[1]);
    if(!infile) {
        perror("Error Encountered. Unable to open file");
        printUsage();
        exit(-1);
    }

    /* Read first three lines of the file which contain:-
        * Number of Diners
        * Number of Tables
        * Number of Cooks
    */
    readThreeLines(&infile, &NUM_DINERS, &NUM_TABLES, &NUM_COOKS);
    cout<<"Number of Diners: "<<NUM_DINERS<<endl;
    cout<<"Number of Tables: "<<NUM_TABLES<<endl;
    cout<<"Number of Cooks: "<<NUM_COOKS<<endl;

    /* Initialize available table queue */
    for(int i=0; i<NUM_TABLES; i++) {
        TABLE_NUMBER.push(i);
    }

    /* Initialize all mutexes, semaphores and condition variables */
    pthread_t *cookThreadIds, *dinerThreadIds;
    cookThreadIds = new pthread_t[NUM_COOKS];
    dinerThreadIds = new pthread_t[NUM_DINERS];
    sem_init(&TABLE_SEM, 0, NUM_TABLES);
    DINER_LOCK = new pthread_mutex_t[NUM_DINERS];
    DINER_COND = new pthread_cond_t[NUM_DINERS];
    pthread_mutex_init(&BURGER_MACHINE, NULL);
    pthread_mutex_init(&FRIES_MACHINE, NULL);
    pthread_mutex_init(&COKE_MACHINE, NULL);
    pthread_mutex_init(&ALL_QUEUES, NULL);
    pthread_mutex_init(&TABLE_QUEUE, NULL);
    pthread_mutex_init(&PRINT_LOCK, NULL);
    pthread_mutex_init(&CLOSED_LOCK, NULL);
    pthread_cond_init(&NO_ORDERS_COND, NULL);
    for(int i=0; i<NUM_DINERS; i++) {
        pthread_mutex_init(&DINER_LOCK[i], NULL);
        pthread_cond_init(&DINER_COND[i], NULL);
    }

    /* Get global start time */
    time(&START_TIME);

    /* Create threads for each cook */
    int *cookIds = new int[NUM_COOKS];
    for (int i=0; i<NUM_COOKS; i++) {
        cookIds[i] = i; 
        pthread_create(&cookThreadIds[i], NULL, cookThread, (void*)(cookIds + i));
    }

    /* Restaurant has now started accepting patrons */
    time_t CURR_TIME;
    time(&CURR_TIME);
    pthread_mutex_lock(&PRINT_LOCK);
    cout<<endl<<"Current time is: " << print_time(difftime(CURR_TIME,START_TIME)) << endl;
    cout<<endl<<"Restaurant 6341 will now begin accepting diners and will continue accepting diners for 120 minutes."<<endl;
    pthread_mutex_unlock(&PRINT_LOCK);
    
    string line;
    time_t t;
    int num[3];
    int count=0;
    DINERS = new Diner*[NUM_DINERS]; // Create Diner* pointers for NUM_DINER number of Diners
    
    do {
        // Read diner information line from input file
        getline(infile, line);
        if(line.empty()) break;
        istringstream iss(line);
        iss >> t;
        if(iss.peek() == ',') iss.ignore();
        iss >> num[0];
        if(iss.peek() == ',') iss.ignore();
        iss >> num[1];
        if(iss.peek() == ',') iss.ignore();
        iss >> num[2];
        if(!iss.fail()) {
            try {
                // Create Diner object
                DINERS[count] = new Diner(t, count, num[0], num[1], num[2]);

                // Diner successfully created. Now we wait till Diner entry time is less than current time
                do {
                    time(&CURR_TIME);
                } while(difftime(CURR_TIME, START_TIME) < t);

                // Create thread for diner.
                pthread_create(&dinerThreadIds[count], NULL, dinerThread, (void*)DINERS[count]);
                count++;
            } catch (int a) {
                // If the Diner information is incorrect. Eg: Burgers=0 or Time entered is negative
                pthread_mutex_lock(&PRINT_LOCK);
                cerr << "Unable to process Diner: " << line << endl;
                pthread_mutex_unlock(&PRINT_LOCK);
                continue;
            }
        } else {
            // If the line does not contain 4 comma separated numbers.
            pthread_mutex_lock(&PRINT_LOCK);
            cerr << "Skipping line detected in the input file with incorrect format: ";
            cerr << line << endl;
            pthread_mutex_unlock(&PRINT_LOCK);
            continue;
        }
        time(&CURR_TIME);
        // Loop till CURR_TIME <=120 or end of file is reached
    } while(difftime(CURR_TIME, START_TIME) <= TWO_HOURS && !infile.eof());

    // Wait until all the diners have reached
    for (int i=0; i<count; i++) {
        pthread_join(dinerThreadIds[i], NULL);
    }

    //pthread_mutex_lock(&PRINT_LOCK);
    //std::cout << print_time(difftime(CURR_TIME, START_TIME)) << "Main is setting Closed to 1.\n"; 
    //pthread_mutex_unlock(&PRINT_LOCK);
    

    pthread_mutex_lock(&CLOSED_LOCK);
        CLOSED=1;
    pthread_mutex_unlock(&CLOSED_LOCK);


    pthread_mutex_lock(&ALL_QUEUES);
        pthread_cond_broadcast(&NO_ORDERS_COND);
    pthread_mutex_unlock(&ALL_QUEUES);

    for (int i=0; i<NUM_COOKS; i++) {
        pthread_join(cookThreadIds[i], NULL);
    }
    
    pthread_mutex_lock(&PRINT_LOCK);
    std::cout << "Restaurant has now closed.\n";
    pthread_mutex_unlock(&PRINT_LOCK);

    // Destroy all the mutexes
    sem_destroy(&TABLE_SEM);
    pthread_mutex_destroy(&BURGER_MACHINE);
    pthread_mutex_destroy(&FRIES_MACHINE);
    pthread_mutex_destroy(&COKE_MACHINE);
    pthread_mutex_destroy(&ALL_QUEUES);
    pthread_mutex_destroy(&TABLE_QUEUE);
    pthread_mutex_destroy(&PRINT_LOCK);
    pthread_mutex_destroy(&CLOSED_LOCK);
    pthread_cond_destroy(&NO_ORDERS_COND);
    for(int i=0; i<NUM_DINERS; i++) {
        pthread_mutex_destroy(&DINER_LOCK[i]);
        pthread_cond_destroy(&DINER_COND[i]);
    }
    
    exit(1);

}
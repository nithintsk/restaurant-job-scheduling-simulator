#ifndef UTILITY_H
#define UTILITY_H

#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <bits/stdc++.h>

using namespace std;

#define SCALE 1000000 // 100 milliseconds
#define SCALEDOWN 1000000/SCALE
#define TWO_HOURS 120*SCALE
#define BURGER_TIME 5*SCALE
#define FRIES_TIME 3*SCALE
#define COKE_TIME 1*SCALE
#define EAT_TIME 30*SCALE

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define PRINT_TIME(x) TOSTRING(x/60) ":" TOSTRING(x%60)

// Diner Definition
class Diner {
    private: 
        int diner_id;
        time_t entered, seated, served, exited;
        int num_burgers, num_fries, num_coke;
        int burger_status, fries_status, coke_status;
    public:
        Diner(time_t, int, int, int, int);
        int getId();
        int getBurgers();
        int getFries();
        int getCoke();

        time_t getEntered();
        time_t getSeated();
        time_t getServed();
        time_t getExited();
        void setSeated(time_t);
        void setServed(time_t);
        void setExited(time_t);

        void burger_update();
        void fries_update();
        void coke_update();
        void printTimeStats();
        void printDinerOrder();
        bool isOrderComplete(); 
};

// Global Variables
extern int NUM_DINERS;
extern int NUM_TABLES;
extern int NUM_COOKS;
extern int QUEUE_STATUS;
extern Diner** DINERS;
extern int CLOSED;

// Timer variables
extern time_t START_TIME;

// Work Queues for Food Items
extern queue< pair<int,int> > BURGER_ORDERS;
extern queue< pair<int,int> > FRIES_ORDERS;
extern queue< pair<int,int> > COKE_ORDERS;
extern queue<int> TABLE_NUMBER;

// Global locks and synchronization variables
extern pthread_mutex_t BURGER_MACHINE;
extern pthread_mutex_t FRIES_MACHINE;
extern pthread_mutex_t COKE_MACHINE;
extern pthread_mutex_t ALL_QUEUES;
extern pthread_mutex_t TABLE_QUEUE;
extern pthread_mutex_t PRINT_LOCK;
extern pthread_mutex_t CLOSED_LOCK;
extern pthread_mutex_t *DINER_LOCK;
extern pthread_cond_t *DINER_COND;
extern pthread_cond_t NO_ORDERS_COND;
extern sem_t TABLE_SEM;

/* Thread Functions */
void* dinerThread(void *arg); 
void* cookThread(void *arg);

/* Utility Functions */
int* takeInput(ifstream myfile);
void printUsage();
void readThreeLines(ifstream *infile, int *ND, int *NT, int *NC);
void queue_status();
void printCookOrderRecvd(int cookid, int num, int diner, string s);
void printCookMachineLocked(int cookid, int num, int diner, string s);
void printCookOrderComplete(int cookid, int num, int diner, string s);
string print_time(time_t t);
#endif


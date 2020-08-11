#include <iostream>
#include <stdlib.h>
#include <pthread.h>
#include <fstream>
#include <time.h>
#include <unistd.h>
#include <string>
#include <sstream>
#include <iomanip>
#include "utility.h"

using namespace std;

/* Declare extern variables */
int NUM_DINERS, NUM_TABLES, NUM_COOKS;
int QUEUE_STATUS;
int CLOSED = 0;
Diner** DINERS;
time_t START_TIME;
queue< pair<int,int> > BURGER_ORDERS;
queue< pair<int,int> > FRIES_ORDERS;
queue< pair<int,int> > COKE_ORDERS;
queue<int> TABLE_NUMBER;


/* Declare the variables corresponding to the threads that will be created. */
sem_t TABLE_SEM;
pthread_mutex_t BURGER_MACHINE;
pthread_mutex_t FRIES_MACHINE;
pthread_mutex_t COKE_MACHINE;
pthread_mutex_t ALL_QUEUES;
pthread_mutex_t TABLE_QUEUE;
pthread_mutex_t BURGER_QUEUE;
pthread_mutex_t FRIES_QUEUE;
pthread_mutex_t COKE_QUEUE;
pthread_mutex_t PRINT_LOCK;
pthread_mutex_t CLOSED_LOCK;
pthread_mutex_t *DINER_LOCK;
pthread_cond_t *DINER_COND;
pthread_cond_t NO_ORDERS_COND;

/* Thread for the cook who is the consumer in this case
    * Acquire ALL_QUEUES mutex
    * Get QUEUE_STATUS by calling queue_status() 
    * QUEUE_STATUS will have one of the below cases
        Case    Burgers Fries   Coke
        0       0       0       0   
        1       0       0       1
        2       0       1       0
        3       0       1       1
        4       1       0       0
        5       1       0       1
        6       1       1       0
        7       1       1       1
    * If all queues are empty, Call wait on cond variable NO_ORDERS_COND
    * Else, queues are not empty. Go to switch case of the State.
    * Depending on the queue status, try X_MACHINE lock
    * If lock is acquired, pop item from the respective queue
    * Release ALL_QUEUE lock
    * Complete making food
    * Release X_MACHINE lock
    * Update Diner order status
    * Get next food item to prepare
*/
void* cookThread(void *arg) {
    int *id = (int*)arg;
    // Previously was *id = *id + 1;
    *id = *id;
    time_t CURR_TIME;
    while (1) {
        // Get food item to cook by first calling a lock on all queues.
        pthread_mutex_lock(&ALL_QUEUES);
        // Update global QUEUE_STATUS variable
        queue_status();

        while (QUEUE_STATUS == 0) { // All queues are empty
            time(&CURR_TIME);
            
            pthread_mutex_lock(&PRINT_LOCK);
                std::cout << print_time(difftime(CURR_TIME, START_TIME)) << " : Cook " << *id << " is waiting because all order queues are empty." << endl;
            pthread_mutex_unlock(&PRINT_LOCK);
            
            pthread_cond_wait(&NO_ORDERS_COND, &ALL_QUEUES);
            queue_status();
            
            /*
            pthread_mutex_lock(&PRINT_LOCK);
                std::cout << print_time(difftime(CURR_TIME, START_TIME)) << " : Cook " << *id << " has been woken up. Queue status: " << QUEUE_STATUS << endl;
            pthread_mutex_unlock(&PRINT_LOCK);
            */
            
            pthread_mutex_lock(&CLOSED_LOCK);
            if (CLOSED==1) {
                pthread_mutex_unlock(&ALL_QUEUES);
                pthread_mutex_unlock(&CLOSED_LOCK);
                return (void*)0;
            }
            pthread_mutex_unlock(&CLOSED_LOCK);
        }

        /*  Debugging information
            pthread_mutex_lock(&PRINT_LOCK);
                time(&CURR_TIME);
                std::cout << print_time(difftime(CURR_TIME, START_TIME)) << " : Cook " << *id << " has queue status " << QUEUE_STATUS << endl;
            pthread_mutex_unlock(&PRINT_LOCK);            
        */

        // The order pair stores <dinerId,order_quantity>
        pair<int,int>order;
        switch(QUEUE_STATUS) {
            // Case 1: B-0,F-0,C-1 -> Only coke orders exist
            case 1: 
                // Cook is assigned Coke order. Pop coke order from queue.
                order = COKE_ORDERS.front();
                COKE_ORDERS.pop();
                pthread_mutex_unlock(&ALL_QUEUES);
                //printCookOrderRecvd(*id, order.second, order.first, "coke");

                pthread_mutex_lock(&COKE_MACHINE);                    
                    printCookMachineLocked(*id, order.second, order.first, "COKE");
                    usleep(COKE_TIME);
                pthread_mutex_unlock(&COKE_MACHINE);
                
                printCookOrderComplete(*id, order.second, order.first, "Coke");
                
                pthread_mutex_lock(&DINER_LOCK[order.first]);
                    DINERS[order.first]->coke_update();
                    pthread_cond_signal(&DINER_COND[order.first]);
                pthread_mutex_unlock(&DINER_LOCK[order.first]);
                break;
            case 2: 
                // Case 1: B-0,F-1,C-0
                // Cook is assigned Fries order. Pop fries order from queue.
                order = FRIES_ORDERS.front();
                FRIES_ORDERS.pop();
                pthread_mutex_unlock(&ALL_QUEUES);
                //printCookOrderRecvd(*id, order.second, order.first, "fries");

                pthread_mutex_lock(&FRIES_MACHINE);
                    printCookMachineLocked(*id, order.second, order.first, "FRIES");
                    usleep(FRIES_TIME*order.second);
                pthread_mutex_unlock(&FRIES_MACHINE);

                printCookOrderComplete(*id, order.second, order.first, "Fries");
                
                pthread_mutex_lock(&DINER_LOCK[order.first]);
                    DINERS[order.first]->fries_update();
                    pthread_cond_signal(&DINER_COND[order.first]);
                pthread_mutex_unlock(&DINER_LOCK[order.first]);
                break;
            case 4: 
                // Case 1: B-1,F-0,C-0
                // Cook is assigned Burger order. Pop Burger order from queue.

                order = BURGER_ORDERS.front();
                BURGER_ORDERS.pop();
                pthread_mutex_unlock(&ALL_QUEUES);
                //printCookOrderRecvd(*id, order.second, order.first, "burger");
                
                pthread_mutex_lock(&BURGER_MACHINE);
                    //cout<<"Case 4: "<<endl;
                    printCookMachineLocked(*id, order.second, order.first, "BURGER");
                    usleep(BURGER_TIME*order.second);
                pthread_mutex_unlock(&BURGER_MACHINE);

                printCookOrderComplete(*id, order.second, order.first, "Burger");

                pthread_mutex_lock(&DINER_LOCK[order.first]);
                    DINERS[order.first]->burger_update();
                    pthread_cond_signal(&DINER_COND[order.first]);
                pthread_mutex_unlock(&DINER_LOCK[order.first]);
                break;
            case 3: 
                // Case 1: B-0,F-1,C-1
                // Both Fries and Coke orders are present. No burger orders are present
                if(pthread_mutex_trylock(&FRIES_MACHINE) == 0) {
                    // Cook is assigned Fries order. Pop fries order from queue.
                    order = FRIES_ORDERS.front();
                    FRIES_ORDERS.pop();
                    pthread_mutex_unlock(&ALL_QUEUES);
                    //printCookOrderRecvd(*id, order.second, order.first, "fries");
                    printCookMachineLocked(*id, order.second, order.first, "FRIES");
                    usleep(FRIES_TIME*order.second);
                    pthread_mutex_unlock(&FRIES_MACHINE);
                    printCookOrderComplete(*id, order.second, order.first, "Fries");
                    pthread_mutex_lock(&DINER_LOCK[order.first]);
                        DINERS[order.first]->fries_update();
                        pthread_cond_signal(&DINER_COND[order.first]);
                    pthread_mutex_unlock(&DINER_LOCK[order.first]);
                }
                else if(pthread_mutex_trylock(&COKE_MACHINE) == 0) {
                    // Cook is assigned Coke order. Pop Coke order from queue and release lock.
                    order = COKE_ORDERS.front();
                    COKE_ORDERS.pop();
                    pthread_mutex_unlock(&ALL_QUEUES);
                    //printCookOrderRecvd(*id, order.second, order.first, "coke");
                    printCookMachineLocked(*id, order.second, order.first, "COKE");
                    usleep(COKE_TIME*order.second);
                    pthread_mutex_unlock(&COKE_MACHINE);
                    printCookOrderComplete(*id, order.second, order.first, "Coke");
                    pthread_mutex_lock(&DINER_LOCK[order.first]);
                    DINERS[order.first]->coke_update();
                    pthread_cond_signal(&DINER_COND[order.first]);
                    pthread_mutex_unlock(&DINER_LOCK[order.first]);
                }
                else {
                    // If neither fries lock or coke lock is free, unlock queue lock
                    // This is done so that new orders can be inserted and other cooks can try again. 
                    pthread_mutex_unlock(&ALL_QUEUES);
                }
                break;
            case 5: 
                // Case 1: B-1,F-0,C-1
                // Both Burger and Coke orders are present. No fries orders are present
                if(pthread_mutex_trylock(&BURGER_MACHINE) == 0) {
                    // Cook is assigned BURGER order. Pop BURGER order from queue.
                    order = BURGER_ORDERS.front();
                    BURGER_ORDERS.pop();
                    pthread_mutex_unlock(&ALL_QUEUES);
                    //printCookOrderRecvd(*id, order.second, order.first, "burger");
                    printCookMachineLocked(*id, order.second, order.first, "BURGER");
                    usleep(BURGER_TIME*order.second);
                    pthread_mutex_unlock(&BURGER_MACHINE);
                    printCookOrderComplete(*id, order.second, order.first, "Burger");
                    pthread_mutex_lock(&DINER_LOCK[order.first]);
                    DINERS[order.first]->burger_update();
                    pthread_cond_signal(&DINER_COND[order.first]);
                    pthread_mutex_unlock(&DINER_LOCK[order.first]);
                }
                else if(pthread_mutex_trylock(&COKE_MACHINE) == 0) {
                    // Cook is assigned Coke order. Pop Coke order from queue and release lock.
                    order = COKE_ORDERS.front();
                    COKE_ORDERS.pop();
                    pthread_mutex_unlock(&ALL_QUEUES);
                    //printCookOrderRecvd(*id, order.second, order.first, "coke");
                    printCookMachineLocked(*id, order.second, order.first, "COKE");
                    usleep(COKE_TIME*order.second);
                    pthread_mutex_unlock(&COKE_MACHINE);
                    printCookOrderComplete(*id, order.second, order.first, "Coke");
                    pthread_mutex_lock(&DINER_LOCK[order.first]);
                    DINERS[order.first]->coke_update();
                    pthread_cond_signal(&DINER_COND[order.first]);
                    pthread_mutex_unlock(&DINER_LOCK[order.first]);
                }
                else {
                    // If neither burger lock or coke lock is free, unlock queue lock
                    // This is done so that new orders can be inserted and other cooks can try again. 
                    pthread_mutex_unlock(&ALL_QUEUES);
                }
                break;
            case 6: 
                // Case 1: B-1,F-1,C-0
                // Both Burger and Fries orders are present. No coke orders are present
                if(pthread_mutex_trylock(&BURGER_MACHINE) == 0) {
                    // Cook is assigned BURGER order. Pop BURGER order from queue.
                    order = BURGER_ORDERS.front();
                    BURGER_ORDERS.pop();
                    pthread_mutex_unlock(&ALL_QUEUES);
                    //printCookOrderRecvd(*id, order.second, order.first, "burger");
                    printCookMachineLocked(*id, order.second, order.first, "BURGER");
                    usleep(BURGER_TIME*order.second);
                    pthread_mutex_unlock(&BURGER_MACHINE);
                    printCookOrderComplete(*id, order.second, order.first, "Burger");
                    pthread_mutex_lock(&DINER_LOCK[order.first]);
                    DINERS[order.first]->burger_update();
                    pthread_cond_signal(&DINER_COND[order.first]);
                    pthread_mutex_unlock(&DINER_LOCK[order.first]);
                }
                else if(pthread_mutex_trylock(&FRIES_MACHINE) == 0) {
                    // Cook is assigned Fries order. Pop Fries order from queue and release lock.
                    order = FRIES_ORDERS.front();
                    FRIES_ORDERS.pop();
                    pthread_mutex_unlock(&ALL_QUEUES);
                    //printCookOrderRecvd(*id, order.second, order.first, "fries");
                    printCookMachineLocked(*id, order.second, order.first, "FRIES");
                    usleep(FRIES_TIME*order.second);
                    pthread_mutex_unlock(&FRIES_MACHINE);
                    printCookOrderComplete(*id, order.second, order.first, "Fries");
                    pthread_mutex_lock(&DINER_LOCK[order.first]);
                    DINERS[order.first]->fries_update();
                    pthread_cond_signal(&DINER_COND[order.first]);
                    pthread_mutex_unlock(&DINER_LOCK[order.first]);
                }
                else {
                    // If neither burger lock or fries lock is free, unlock queue lock
                    // This is done so that new orders can be inserted and other cooks can try again. 
                    pthread_mutex_unlock(&ALL_QUEUES);
                }
                break;
            case 7: 
                // Case 1: B-1,F-1,C-1
                // Burger and Fries and coke orders are present.
                if(pthread_mutex_trylock(&BURGER_MACHINE) == 0) {
                    // Cook is assigned BURGER order. Pop BURGER order from queue.
                    order = BURGER_ORDERS.front();
                    BURGER_ORDERS.pop();
                    pthread_mutex_unlock(&ALL_QUEUES);
                    //printCookOrderRecvd(*id, order.second, order.first, "burger");
                    printCookMachineLocked(*id, order.second, order.first, "BURGER");
                    usleep(BURGER_TIME*order.second);
                    pthread_mutex_unlock(&BURGER_MACHINE);
                    printCookOrderComplete(*id, order.second, order.first, "Burger");
                    pthread_mutex_lock(&DINER_LOCK[order.first]);
                    DINERS[order.first]->burger_update();
                    pthread_cond_signal(&DINER_COND[order.first]);
                    pthread_mutex_unlock(&DINER_LOCK[order.first]);
                }
                else if(pthread_mutex_trylock(&FRIES_MACHINE) == 0) {
                    // Cook is assigned Fries order. Pop Fries order from queue and release lock.
                    order = FRIES_ORDERS.front();
                    FRIES_ORDERS.pop();
                    pthread_mutex_unlock(&ALL_QUEUES);
                    //printCookOrderRecvd(*id, order.second, order.first, "fries");
                    printCookMachineLocked(*id, order.second, order.first, "FRIES");
                    usleep(FRIES_TIME*order.second);
                    pthread_mutex_unlock(&FRIES_MACHINE);
                    printCookOrderComplete(*id, order.second, order.first, "Fries");
                    pthread_mutex_lock(&DINER_LOCK[order.first]);
                    DINERS[order.first]->fries_update();
                    pthread_cond_signal(&DINER_COND[order.first]);
                    pthread_mutex_unlock(&DINER_LOCK[order.first]);
                }
                else if(pthread_mutex_trylock(&COKE_MACHINE) == 0) {
                    // Cook is assigned Coke order. Pop Coke order from queue and release lock.
                    order = COKE_ORDERS.front();
                    COKE_ORDERS.pop();
                    pthread_mutex_unlock(&ALL_QUEUES);
                    //printCookOrderRecvd(*id, order.second, order.first, "coke");
                    printCookMachineLocked(*id, order.second, order.first, "COKE");
                    usleep(COKE_TIME*order.second);
                    pthread_mutex_unlock(&COKE_MACHINE);
                    printCookOrderComplete(*id, order.second, order.first, "Coke");
                    pthread_mutex_lock(&DINER_LOCK[order.first]);
                    DINERS[order.first]->coke_update();
                    pthread_cond_signal(&DINER_COND[order.first]);
                    pthread_mutex_unlock(&DINER_LOCK[order.first]);
                }
                else {
                    // If neither burger lock or fries lock or coke lock is free, unlock queue lock
                    // This is done so that new orders can be inserted and other cooks can try again. 
                    pthread_mutex_unlock(&ALL_QUEUES);
                }
                break;
            default: 
                pthread_mutex_unlock(&ALL_QUEUES);
                pthread_mutex_lock(&PRINT_LOCK);
                cerr << "Cook " << *id << "has encountered incorrect queue status value: " << QUEUE_STATUS << endl;
                pthread_mutex_unlock(&PRINT_LOCK);
                return (void*)0;
        }

        pthread_mutex_lock(&CLOSED_LOCK);
        if (CLOSED == 1) {
            pthread_mutex_unlock(&ALL_QUEUES);
            pthread_mutex_unlock(&CLOSED_LOCK);
            return (void*)0;
        }
        pthread_mutex_unlock(&CLOSED_LOCK);
    }
}

// QUEUE_STATUS is 0 if empty and +ve integer if not
void queue_status() {

    int burgers = !BURGER_ORDERS.empty();
    int fries = !FRIES_ORDERS.empty();
    int coke = !COKE_ORDERS.empty();

    // Left shift the bits
    burgers = burgers << 2;
    fries = fries << 1;

    QUEUE_STATUS = burgers+fries+coke;
}

/* Thread for the diner who is the producer in this case
    * Try to decrement table semaphore <- controls how diners are seated.
    * Get table number from table queue
    * Acquire mutex lock of ALL_QUEUES and populate with food orders
    * Wait on condition obj->isOrderComplete() to start eating
    * Eat, then release table and exit
*/
void* dinerThread(void *arg) {
    Diner *obj = (Diner*)arg;
    time_t CURR_TIME;
    int id = obj->getId();
    int print_id = id;
    int burgers = obj->getBurgers();
    int fries = obj->getFries();
    int coke = obj->getCoke();
    int table_num;

    // Print Diner entry message
    pthread_mutex_lock(&PRINT_LOCK);
        time(&CURR_TIME);
        std::cout << print_time(difftime(CURR_TIME,START_TIME)) << " : Diner " << print_id <<" has entered the restaurant. "; 
        std::cout<<"Diner will wait to be seated."<<endl;
    pthread_mutex_unlock(&PRINT_LOCK);
    
    // Try to get seated at a table by acquiring TABLE_SEM semaphore
    sem_wait(&TABLE_SEM);
    
    // Get available table form the table queue
    pthread_mutex_lock(&TABLE_QUEUE);
    table_num = TABLE_NUMBER.front();
    TABLE_NUMBER.pop();
    pthread_mutex_unlock(&TABLE_QUEUE);

    // Diner is now seated. Get time of seating
    time(&CURR_TIME);
    obj->setSeated(difftime(CURR_TIME, START_TIME));

    // Print Diner is seated message
    pthread_mutex_lock(&PRINT_LOCK);
    std::cout<< print_time(difftime(CURR_TIME,START_TIME)) << " : Diner " << print_id <<" is seated at Table " << table_num << endl;
    std::cout<<"        Diner will place the following orders."<<endl;
    obj->printDinerOrder();
    pthread_mutex_unlock(&PRINT_LOCK);

    // Place order by adding items to the order queue.
    // We are locking all queues to place items in them.
    pthread_mutex_lock(&ALL_QUEUES);
        BURGER_ORDERS.push( pair<int,int>(id, burgers));
        if(fries) {
            FRIES_ORDERS.push(pair<int,int>(id, fries));
        }
        if(coke) {
            COKE_ORDERS.push(pair<int,int>(id, coke));
        }
        queue_status(); // Update QUEUE_STATUS variable
        pthread_cond_broadcast(&NO_ORDERS_COND);
    pthread_mutex_unlock(&ALL_QUEUES);

    // Wait until signalled and order is complete.
    pthread_mutex_lock(&DINER_LOCK[id]);
    while(!obj->isOrderComplete()) {
        pthread_cond_wait(&DINER_COND[id], &DINER_LOCK[id]);
    }
    pthread_mutex_unlock(&DINER_LOCK[id]);

    // Food is now served. Eat for 30 minutes
    time(&CURR_TIME);
    obj->setServed(difftime(CURR_TIME, START_TIME));
    
    pthread_mutex_lock(&PRINT_LOCK);
    std::cout << print_time(difftime(CURR_TIME,START_TIME)) << " : Diner " << print_id << "'s order is ready. "; 
    std::cout << "Diner " << print_id << " will now begin eating." << endl;
    pthread_mutex_unlock(&PRINT_LOCK);

    // Finished eating, so leave table by releasing TABLE_SEM
    usleep(EAT_TIME);

    // Set exit time.
    time(&CURR_TIME);
    obj->setExited(difftime(CURR_TIME, START_TIME));

    // Acquire print lock and call print function
    pthread_mutex_lock(&PRINT_LOCK);
    std::cout << print_time(difftime(CURR_TIME, START_TIME)) << " : Diner Id: " << print_id << " has completed dining and is leaving the Restaurant." << endl;
    std::cout << "        Table number " << table_num << " is now free."<<endl; 
    obj->printDinerOrder();
    obj->printTimeStats();
    pthread_mutex_unlock(&PRINT_LOCK);

    // Release table number by adding it back into the table queue
    pthread_mutex_lock(&TABLE_QUEUE);
    TABLE_NUMBER.push(table_num);
    pthread_mutex_unlock(&TABLE_QUEUE);

    // Increment table semaphore
    sem_post(&TABLE_SEM);
    
    return (void*)0;
}

void printCookOrderComplete(int cookid, int num, int diner, string s) {
    time_t CURR_TIME;
    pthread_mutex_lock(&PRINT_LOCK);
        time(&CURR_TIME);
        std::cout  << print_time(difftime(CURR_TIME, START_TIME)) << " : Cook " << cookid << " has finished preparing " << num
        << " number of " << s << "s of Diner " << diner << endl; 
    pthread_mutex_unlock(&PRINT_LOCK);
}

void printCookOrderRecvd(int cookid, int num, int diner, string s) {
    time_t CURR_TIME;
    pthread_mutex_lock(&PRINT_LOCK);
        time(&CURR_TIME);
        std::cout << print_time(difftime(CURR_TIME, START_TIME)) << " : Cook " << cookid << " has received order for " << num
        << " number of " << s << "s of Diner " << diner << endl; 
    pthread_mutex_unlock(&PRINT_LOCK);
}

void printCookMachineLocked(int cookid, int num, int diner, string s) {
    time_t CURR_TIME;
    pthread_mutex_lock(&PRINT_LOCK);
        time(&CURR_TIME);
        std::cout << print_time(difftime(CURR_TIME, START_TIME)) << " : Cook " << cookid << " uses the "
        << s << " machine for "  << num << " number of " << s << "s of Diner " << diner << endl; 
    pthread_mutex_unlock(&PRINT_LOCK);
}

Diner::Diner(time_t te, int id, int nb, int nf, int nc) {
    if((te <= 120 && te >= 0) && nb >= 1 && nf >= 0 && (nc == 0 || nc == 1)) {
        entered=te;
        diner_id=id;
        num_burgers=nb;
        num_fries=nf;
        num_coke=nc;
    } else throw -1;
};

time_t Diner::getEntered() {return entered;}
time_t Diner::getSeated() {return seated;}
time_t Diner::getServed() {return served;}
time_t Diner::getExited() {return exited;}

void Diner::setSeated(time_t x) {seated = x;};
void Diner::setServed(time_t x) {served = x;};
void Diner::setExited(time_t x) {exited = x;};

int Diner::getId() {return diner_id;}
int Diner::getBurgers() {return num_burgers;}
int Diner::getFries() {return num_fries;}
int Diner::getCoke() {return num_coke;}

void Diner::burger_update() {burger_status=num_burgers;}
void Diner::fries_update() {fries_status=num_fries;}
void Diner::coke_update() {coke_status=num_coke;}
bool Diner::isOrderComplete() { 
    return ( burger_status==num_burgers && fries_status==num_fries && coke_status==num_coke ) ? true : false;
}

void Diner::printDinerOrder(){
    std::cout<< "        Number of Burgers: " << getBurgers() << endl;
    std::cout<< "        Orders of Fries: " << getFries() << endl;
    std::cout<< "        Orders of Coke: " << getCoke() <<endl;
}

void Diner::printTimeStats() {
    std::cout<< "        Time Entered: " << getEntered() << " minute mark." << endl;
    std::cout<< "        Time Seated: " << getSeated() << " minute mark." << endl;
    std::cout<< "        Time Served: " << getServed() << " minute mark." << endl;
    std::cout<< "        Time Exited: " << getExited() << " minute mark." << endl;
}

/* If any if the first three lines do not contain an integer as the first element,
   an error will be thrown. the function is designed to read only the first integer
   in the line and ignore the rest of the characters until an endl appears.
*/
void readThreeLines(ifstream *infile, int *ND, int *NT, int *NC) {
    *infile >> *ND;
    (*infile).ignore(256,'\n');
    *infile >> *NT;
    (*infile).ignore(256,'\n');
    *infile >> *NC;
    (*infile).ignore(256,'\n');
    if((*infile).fail()) {
        cerr<<"Encountered Error while reading first three lines from file."<<endl;
        std::cout<<"Exiting...."<<endl<<endl;
        (*infile).close();
        exit(-1);
    }
    return;
}

void printUsage() {
    std::cout<<"Usage is :"<<endl;
    std::cout<<"./Restaurant <file fqn>"<<endl<<endl;
    std::cout<<"Example:"<<endl;
    std::cout<<"./Restaurant /home/user/Document/OS-project/diners.txt"<<endl;
    return;
}

string print_time(time_t t) {
    stringstream ss;
    int hour = t/60;
    ss << std::setw(2) << std::setfill('0') << hour;
    string s = ss.str();
    s = s + ":";
    ss.str("");
    int minute = t%60;
    ss << std::setw(2) << std::setfill('0') << minute;
    s = s + ss.str();
    return s;
}
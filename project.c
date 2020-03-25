#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <assert.h>

/**
* Tecnical Description
* --------------------
*
* 1. Queue structure
*    Queue system is beased on frist come frist serve stragety storing as array
*    [0, 0] - Intitialized data
*    [9, 3] - Customer ID 9 requesting service type 3
*    [-1, -1] - Queue had been removed
*
* 2. Service type
*    0 - ATM service
*    1 - Cheque service
*    2 - Exchange service
*/

#include <stdbool.h>

// Config
#define MAX_QUEUE 1000000
#define MAX_TIME 60
#define TIME_ATM 1
#define TIME_CHEQUE 2
#define TIME_EXCHANGE 3

// Inititalize mutex and queue
pthread_t tid;
pthread_attr_t attr;
pthread_mutex_t mutex;
sem_t full, empty;

int queue[MAX_QUEUE][2]; // Customer queue [customerID, serviceType] ; customerID = -1 mean customer terminated
int queuePointer = 0;
int lastTimeCustomerCreated = -1; // Set when custom is created
int tempCustomerID = 0; // Store last set of customerID that created

int currentTime = 0; // Current running time
int nextCustomerSet = 0; // Specify when next customer is going to be added in queue

// Tell that service has done process in each minute
bool isATMDone = false;
bool isChequeDone = false;
bool isExchangeDone = false;

// Create customer thread
void *customer(void *threadNo);
void *customer(void *threadNo) {
  // Get customer unique ID
  int customerID = (int)threadNo;

  // Random action of customer (0 = ATM, 1 = Cheque, 2 = Exchange)
  int serviceType = (rand() % 3);

  // Get queue position
  sem_wait(&empty);
  pthread_mutex_lock(&mutex);
  int obtainedQueue = queuePointer;
  queue[queuePointer][0] = customerID;
  queue[queuePointer][0] = serviceType;
  printf("[customer]: customer %d obtained queue %d for service %d\n", customerID ,queuePointer, serviceType);
  queuePointer++;
  pthread_mutex_unlock(&mutex);
  sem_post(&full);
}

// Create service thread
void *service(void *threadNo);
void *service(void *threadNo) {
  int serviceType = threadNo;
  int lastRunMinute = -1;

  int servicingCustomer = -1; // -1 = Not servicing, x = Servicing customer ID X
  int servicingCustomerQueuePosition = 0;
  int serviceTimeLeft = -1;

  while(true) {
    sem_wait(&full);
    pthread_mutex_lock(&mutex);

    // If service did not run current minute yet then do so...
    if (lastRunMinute != currentTime) {
      // If not serving any customer...then find one
      if (servicingCustomer == -1) {
        for (int i = 0 ; i < MAX_QUEUE ; i++) {
          if (queue[i][1] == serviceType) {
            servicingCustomer = queue[i][0];
            servicingCustomerQueuePosition = i;
            serviceTimeLeft = serviceType;
            break;
          }
        }

        if (servicingCustomer == -1) {
          printf("[service %d]: no queue to serve right now...\n", serviceType);
          lastRunMinute = currentTime;
        } else {
          printf("[service %d]: found customer %d to service\n", serviceType, servicingCustomer);
        }
      } else {
        serviceTimeLeft--;
        printf("[service %d]: successfully service customer %d (%d / %d)\n", serviceType, servicingCustomer, serviceType - serviceTimeLeft, serviceType);

        // If service is done, then remove from queue
        if (serviceTimeLeft == 0) {
          printf("[service %d]: service customer %d is completed removing from queue...", serviceType, servicingCustomer);
          queue[servicingCustomerQueuePosition][0] = -1;
          queue[servicingCustomerQueuePosition][1] = -1;
          servicingCustomer = -1;
          serviceTimeLeft = -1;
          printf("[service %d]: done! ready to service next customer in the next minute", serviceType);
        }

        // Finalize process in current minute
        lastRunMinute = currentTime;
      }
    }

    pthread_mutex_unlock(&mutex);
    sem_post(&empty);
  }
}

// Main thrad
int main() {
  printf("[core]: intitializing...\n");
  /**
  * Inititalize
  */
  pthread_mutex_init(&mutex, NULL);
  sem_init(&full, 0, 0);
  sem_init(&empty, 0, 1);
  pthread_attr_init(&attr);

  // Set all value in queue to 0 (inactive)
  for (int i = 0 ; i < MAX_QUEUE ; i++) {
    queue[i][0] = 0;
  }
  printf("[core]: done\n");

  // Create service thread
  for (int i = 0 ; i < 3 ; i++) {
    pthread_create(&tid, &attr, service, (void *)i);
  }

  // Set time to 0
  while(currentTime < MAX_TIME) {
    /**
    * Action for every minute
    */

    // Create new customer every 5 minutes
    if (currentTime % 5 == 0 && lastTimeCustomerCreated != currentTime) {
      // Random customer between 5-10 customers
      lastTimeCustomerCreated = currentTime;
      int amountCustomer = (rand() % 5) - 5;

      printf("[system]: %d customers being created in minute %d\n", amountCustomer, currentTime + 1);

      for (int i = tempCustomerID ; i < tempCustomerID + amountCustomer ; i++) {
        pthread_create(&tid, &attr, customer, (void *)i);
      }

      tempCustomerID += amountCustomer;
    }

    /**
    * When all service is done, increment time by one and start all over
    */
    if (isATMDone == true && isChequeDone == true && isExchangeDone == true) {
      currentTime++;
      isATMDone == false;
      isChequeDone == false;
      isExchangeDone == false;
    }
  }
  printf("[system]: time up! service is now closed after %d mintutes\n", MAX_TIME);
  printf("[core]: terminated\n", MAX_TIME);
  exit(0);
}
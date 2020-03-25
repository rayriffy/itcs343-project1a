#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <assert.h>

typedef int buffer_item;

#define BUFFER_SIZE 10
#define RAND_DIVISOR 1000000
#define TRUE 1

pthread_mutex_t mutex;           //the mutex lock
sem_t full, empty;               //the semaphores
buffer_item buffer[BUFFER_SIZE]; //the buffer
int counter;                     //buffer counter
pthread_t tid;                   //Thread ID
pthread_attr_t attr;             //Set of thread attributes

void *producer(void *pno);       //the producer thread
void *consumer(void *cno);       //the consumer thread

void initializeData() {
  pthread_mutex_init(&mutex, NULL); //create the mutex lock

  //Create the full semaphore and initialize to 0
  sem_init(&full, 0, 0);
  //Create the empty semaphore and initialize to BUFFER_SIZE
  sem_init(&empty, 0, BUFFER_SIZE);

  pthread_attr_init(&attr); //get the default attributes

  counter = 0;              //init buffer
}

// Add an item to the buffer
int insert_item(buffer_item item) {
  //When the buffer is not full add the item and increment the counter
  if (counter < BUFFER_SIZE) {
    buffer[counter] = item;
    counter++;
    return 0;
  }
  else {
    //Error if the buffer is full
    return -1;
  }
}

// Remove an item from the buffer
int remove_item(buffer_item *item) {
  /* When the buffer is not empty remove the item
 and decrement the counter */
  if (counter > 0) {
    *item = buffer[(counter - 1)];
    counter--;
    return 0;
  }
  else {
    // Error buffer empty
    return -1;
  }
}

// Producer Thread
void *producer(void *pno) {
  buffer_item item;
  long tid;
  tid = (long)pno;
  while (TRUE) {
    item = (buffer_item)rand() % 100; // generate a random number
    sem_wait(&empty);                 // acquire the empty lock
    pthread_mutex_lock(&mutex);       // acquire the mutex lock
    if (insert_item(item)) {
      fprintf(stderr, "Producer report error condition\n");
    }
    else {
      printf("producer no %ld produced %d\n", tid, item);
    }
    pthread_mutex_unlock(&mutex); //release the mutex lock
    sem_post(&full);              //signal full
  }
}

// Consumer Thread
void *consumer(void *cno) {
  buffer_item item;
  long tid;
  tid = (long)cno;
  while (TRUE) {
    sem_wait(&full);            //acquire the full lock
    pthread_mutex_lock(&mutex); //acquire the mutex lock
    if (remove_item(&item)) {
      fprintf(stderr, "Consumer report error condition\n");
    }
    else {
      printf("consumer no %ld consumed %d\n", tid, item);
    }
    pthread_mutex_unlock(&mutex); //release the mutex lock
    sem_post(&empty);             //signal empty
  }
}

int main(int argc, char *argv[]) {
  long i; //loop counter
  // Verify the correct number of arguments were passed in
  if (argc != 4) {
    fprintf(stderr, "USAGE:./main.out <INT> <INT> <INT>\n");
    exit(-1);
  }

  int mainSleepTime = atoi(argv[1]); //time in seconds for main to sleep
  int numProd = atoi(argv[2]);       //number of producer threads
  int numCons = atoi(argv[3]);       //number of consumer threads

  initializeData();                  //Initialize the app

  // Create the producer threads
  for (i = 0; i < numProd; i++) {
    pthread_create(&tid, &attr, producer, (void *)i);
  }
  // Create the consumer threads
  for (i = 0; i < numCons; i++) {
    pthread_create(&tid, &attr, consumer, (void *)i);
  }

  // Sleep for the specified amount of time in milliseconds
  sleep(mainSleepTime);
  // Exit the program
  printf("Exit the program\n");
  exit(0);
}
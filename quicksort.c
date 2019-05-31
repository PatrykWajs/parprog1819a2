#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

//gcc -O2 -Wall -pthread quicksort.c -o quicksort

// Define constants

#define N 1000000
#define THREADS 4
#define SIZE 100
#define THRESHOLD 10

#define WORK 0
#define FINISH 1
#define SHUTDOWN 2

// Initialization

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t msg_in = PTHREAD_COND_INITIALIZER;
pthread_cond_t msg_out = PTHREAD_COND_INITIALIZER;

// The message structure

struct message
{
    int type;
    int begin;
    int end;
};

struct message mqueue[N];
int q_in = 0;
int q_out = 0;
int m_count = 0;

// Send

void send(int type, int begin, int end) {

  pthread_mutex_lock(&mutex);
  while (m_count >= N) {
        printf("Sender locked\n");
        pthread_cond_wait(&msg_out, &mutex);
      }

  mqueue[q_in].type = type;
  mqueue[q_in].begin = begin;
  mqueue[q_in].end = end;

  q_in = (q_in + 1) % N;

  m_count++;

  pthread_cond_signal(&msg_in);
  pthread_mutex_unlock(&mutex);

}

// Receive

void receive(int *type, int *begin, int *end){

  pthread_mutex_lock(&mutex);

  while (m_count < 1) {
        printf("Receiver locked\n");
        pthread_cond_wait(&msg_in, &mutex);
      }

  *type = mqueue[q_out].type;
  *begin = mqueue[q_out].begin;
  *end = mqueue[q_out].end;
  q_out = (q_out + 1) % N;
  m_count--;

  pthread_cond_signal(&msg_out);
  pthread_mutex_unlock(&mutex);

}

// Swap

void swap(double *a, double *b) {
    double tmp = *a;
    *a = *b;
    *b = tmp;
}

// Partition

int partition(double *a, int n){

  int first = 0;
  int middle = n/2;
  int last = n-1;

  if (a[first] > a[middle]) {
        swap(a+first, a+middle);
    }
  if (a[middle] > a[last]) {
        swap(a+middle, a+last);
    }
  if (a[first] > a[middle]) {
        swap(a+first, a+middle);
    }
  double p = a[middle];
  int i, j;
  for (i=1, j=n-2;; i++, j--) {
      while (a[i] < p) i++;
      while (a[j] > p) j--;
      if (i>=j){
        break;
      }
      swap(a+i, a+j);
  }
  return i;

}

// Sort

void ins_sort(double *a, int n) {
    int i, j;
    for (i=1; i<n; i++) {
        j = i;
        while (j>0 && a[j-1] > a[j]) {
            swap(a+j, a+j-1);
            j--;
        }
    }
}

// Thread

void *thread_func(void *params) {
    double *a = (double*) params;
    int t, s, e;

    receive(&t, &s, &e);

    while (t != SHUTDOWN) {
        if (t == FINISH) {
            send(FINISH, s, e);
        } else if (t == WORK) {
            if (e-s <= THRESHOLD) {
                ins_sort(a+s, e-s);
                send(FINISH, s, e);
            } else {
                int p = partition(a+s, e-s);
                send(WORK, s, s+p);
                send(WORK, s+p, e);
            }
        }
        receive(&t, &s, &e);
    }

    send(SHUTDOWN, 0, 0);
    printf("Done!\n");
    pthread_exit(NULL);
}

// Main

int main(int argc, char ** argv){

  double *array = (double*) malloc(sizeof(double) * SIZE);
  int i;

  if (array == NULL) {
        printf("Error during memory allocation\n");
        exit(1);
      }

  for(int i=0; i<SIZE; i++){
    		array[i] = (double) rand()/RAND_MAX;
      }

  pthread_t threads[THREADS];
  	for(i=0; i<THREADS; i++){
  		if(pthread_create(&threads[i], NULL, thread_func, array) != 0){
  			printf("Thread Creation Error!\n");
        free(array);
  			exit(1);
  		}
    }

  send(WORK, 0, SIZE);

  int t, s, e;
  int count = 0;

  receive(&t, &s, &e);

  while (1) {
          if (t == FINISH) {
              count += e-s;
              printf("Done with %d out of %d\n", count, SIZE);
              printf("Partition done: (%d, %d)\n", s, e);
              if (count == SIZE) {
                  break;
              }
          } else {
              send(t, s, e);
          }
          receive(&t, &s, &e);
      }

  send(SHUTDOWN, 0, 0);

  for (int i=0; i<THREADS; i++) {
    pthread_join(threads[i], NULL);
  }

  for(i=0; i<(SIZE-1); i++){
    if(array[i] > array[i+1]){
      printf("%lf, %lf\n", array[i], array[i+1]);
      printf("Array sort error\n");
      break;
    }
    if(i == SIZE-1) {
      printf("Sorted Succesfully");
    }
  }

  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&msg_in);
  pthread_cond_destroy(&msg_out);
  free(array);
  return 0;

}

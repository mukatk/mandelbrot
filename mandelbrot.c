#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "workstash.h"
#define NUM_THREADS 16

pthread_t thread_cons[NUM_THREADS];
pthread_t thread_prod[NUM_THREADS];

pthread_mutex_t buffer_mutex;

void *consumer(void *arg);

void *producer(void *arg);

typedef struct {
	int initial_x;
	int final_x;
	int initial_y;
	int final_y;
} region_param, *region_param_arg;

region_param regions[NUM_THREADS];

int main(int argc, char **argv) {
	int i;	
	pthread_mutex_init(&buffer_mutex, NULL);
	
	for(i=0; i < NUM_THREADS; i++) {
		pthread_create(&(thread_cons[i]), NULL, consumer, NULL);
	}
	
	for(i = 0; i < NUM_THREADS; i++) {
		regions[i].initial_x = i * 40;
		regions[i].initial_y = i * 40;
		regions[i].final_x = (i * 40) + 40;
		regions[i].final_y = (i * 40) + 40;
		pthread_create(&(thread_prod[i]), NULL, producer, &(regions[i]));
	}
	
	for(i = 0; i < NUM_THREADS; i++) {
		pthread_join(thread_cons[i], NULL);
	}
	
	for(i = 0 ; i < NUM_THREADS; i++) {
		pthread_join(thread_prod[i], NULL);
	}
	
	return 0;
}

void *producer(void *arg) {
	region_param_arg region = (region_param_arg)arg;
	printf("x0:%d - x1:%d\n", region->initial_x, region->final_x);
	printf("y0:%d - y1:%d\n", region->initial_y, region->final_y);
	pthread_mutex_lock(&buffer_mutex);
	struct work_param work;
	work.x = 0;
	work.y = 0;
	work.color = 0;
	push_work(work);
	pthread_mutex_unlock(&buffer_mutex);
}

void *consumer(void *arg) {
	
}

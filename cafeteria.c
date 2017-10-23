/*
 * cafeteria.c
 *
 *  Created on: Oct 21, 2017
 *      Author: Hasan Hüseyin PAY
 */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

sem_t tray_full_sem;
sem_t tray_emty_sem;
sem_t student_wait_sem;

pthread_mutex_t print_lock = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t tray_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t student_wait_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t student_fetch_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t student_total_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cook_sleep_lock = PTHREAD_MUTEX_INITIALIZER;

int trays = 8;
int waiting_line = 0;

int cook_sleep = 0;
int total_tray = 8;
int student_total = 0;
int student_fetch = 0;

static time_t START_TIME;

void *cook(void *);
void *student(void *);
void monitor(void *);
int main()
{
	pthread_t thread_cook;
	pthread_t thread_student;

	if(sem_init(&tray_full_sem, 0, 0) == -1
			|| sem_init(&tray_emty_sem, 0, 1) == -1
			|| sem_init(&student_wait_sem, 0, 1) == -1) {/* initialize mutex to 1 - binary semaphore second param = 0 - semaphore is local */
		perror("sem_init");
		exit(EXIT_FAILURE);
	}
	START_TIME = time(NULL);
	printf("DEU-CAFETERIA starting at %s\n", ctime(&START_TIME));

	pthread_create(&thread_cook, NULL, &cook, NULL);

	while(1) {
		pthread_create(&thread_student, NULL, &student, NULL);
		sleep(rand() % 3 + 1);
	}

	void *status;
	pthread_join(thread_cook, status);

	return 0;
}

void *cook(void *arg)
{
	pthread_mutex_lock(&print_lock);
	printf("Cook created .. at %ld\n", time(NULL) - START_TIME);
	pthread_mutex_unlock(&print_lock);

	unsigned int random_time;
	time_t now;

	while(1) {

		pthread_mutex_lock(&tray_lock);
		if(trays == 8) {
			pthread_mutex_lock(&cook_sleep_lock);
			cook_sleep = 1;
			pthread_mutex_unlock(&cook_sleep_lock);

			pthread_mutex_lock(&print_lock);
			printf("[!] %ld - cook started to sleep \n",
					time(NULL) - START_TIME);
			pthread_mutex_unlock(&print_lock);

			pthread_mutex_unlock(&tray_lock);
			sem_wait(&tray_full_sem); /* down semaphore */

			pthread_mutex_lock(&cook_sleep_lock);
			cook_sleep = 0;
			pthread_mutex_unlock(&cook_sleep_lock);

			pthread_mutex_lock(&print_lock);
			printf("[!] %ld - cook awake \n",
					time(NULL) - START_TIME);
			pthread_mutex_unlock(&print_lock);
		} else
			pthread_mutex_unlock(&tray_lock);

		random_time = rand() % 5 + 2;
		now = time(0);

		pthread_mutex_lock(&print_lock);
		printf("*%ld cook started to fill %d'th tray\n",
				time(NULL) - START_TIME, total_tray + 1);
		pthread_mutex_unlock(&print_lock);

		while(time(0) - now < random_time)
			;/* filling the tray - busy waiting */

		pthread_mutex_lock(&tray_lock);
		trays++;
		pthread_mutex_unlock(&tray_lock);

		total_tray++;

		pthread_mutex_lock(&print_lock);
		printf("*%ld cook fished to fill %d'th tray\n",
				time(NULL) - START_TIME, total_tray);
		pthread_mutex_unlock(&print_lock);
	}
}

void *student(void *arg)
{
	pthread_mutex_lock(&student_total_lock);
	student_total++;
	pthread_mutex_unlock(&student_total_lock);

	pthread_mutex_lock(&print_lock);
	printf("\t- %d'th student arrived .. \n", student_total);
	pthread_mutex_unlock(&print_lock);

	pthread_mutex_lock(&student_wait_lock);

	while(trays == 0)
		;/* wait the tray - busy waiting */

	sleep(1);

	pthread_mutex_lock(&tray_lock);
	trays--;
	if(trays == 7) {
		pthread_mutex_lock(&cook_sleep_lock);
		if(cook_sleep == 1)
			sem_post(&tray_full_sem); /* up semaphore */
		pthread_mutex_unlock(&cook_sleep_lock);
	}
	student_fetch++;

	pthread_mutex_lock(&print_lock);
	printf("\t -%d'th student fetched his tray .. \n", student_fetch);
	pthread_mutex_unlock(&print_lock);

	pthread_mutex_unlock(&tray_lock);

	pthread_mutex_unlock(&student_wait_lock);

	pthread_exit(NULL);
}

void monitor(void *arg)
{

}

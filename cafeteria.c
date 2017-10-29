/*
 * cafeteria.c
 *
 *  Created on: Oct 21, 2017
 *      Author: Hasan Hüseyin PAY
 */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RESET   "\x1b[0m"

sem_t tray_full_sem;

pthread_mutex_t print_lock = PTHREAD_MUTEX_INITIALIZER; // ekrana yazdırırken ortalık karışmaması için

pthread_mutex_t tray_lock = PTHREAD_MUTEX_INITIALIZER; // anlık tray sayısına erişmek için
pthread_mutex_t student_wait_lock = PTHREAD_MUTEX_INITIALIZER; // yeni student geldiği zaman karşıklığı önlemek için
pthread_mutex_t student_total_lock = PTHREAD_MUTEX_INITIALIZER; // toplam student sayısına erişmek için
pthread_mutex_t cook_sleep_lock = PTHREAD_MUTEX_INITIALIZER; // aşçının uyku durumu için

int trays = 8; // current trays

int cook_sleep = 0;
char *cook_sleep_str[] = {"Working", "Sleep"};
int total_tray = 8;
int student_total = 0;
int student_fetch = 0;

static time_t START_TIME;

void *cook(void *);
void *student(void *);
void monitor(void);

int main()
{
	pthread_t thread_cook;
	pthread_t thread_student;

	if(sem_init(&tray_full_sem, 0, 0) == -1) { /* initialize mutex to 1 - binary semaphore second param = 0 - semaphore is local */
		perror("sem_init");
		exit(EXIT_FAILURE);
	}
	START_TIME = time(NULL);
	printf("DEU-CAFETERIA starting at %s\n", ctime(&START_TIME));

	pthread_create(&thread_cook, NULL, &cook, NULL); // aşçı oluşturuldu

	// 1-4sn de student oluşturmak için
	while(1) {
		sleep(rand() % 3 + 1);
		pthread_create(&thread_student, NULL, &student, NULL);
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
			/* 8 tane tray varsa aşçı uyuyacak */

			pthread_mutex_lock(&cook_sleep_lock);
			cook_sleep = 1;
			pthread_mutex_unlock(&cook_sleep_lock);

			pthread_mutex_lock(&print_lock);
			printf(
					ANSI_COLOR_GREEN"[ %ld ] - cook started to sleep \n" ANSI_COLOR_RESET,
					time(NULL) - START_TIME);
			monitor();
			pthread_mutex_unlock(&print_lock);

			pthread_mutex_unlock(&tray_lock); // uyumadan önce tray lock'ı açılıyor.

			sem_wait(&tray_full_sem); /* down semaphore */

			pthread_mutex_lock(&cook_sleep_lock);
			cook_sleep = 0;
			pthread_mutex_unlock(&cook_sleep_lock);

			pthread_mutex_lock(&print_lock);
			printf(
					ANSI_COLOR_GREEN "[ %ld ] - cook awake \n" ANSI_COLOR_RESET,
					time(NULL) - START_TIME);
			monitor();
			pthread_mutex_unlock(&print_lock);
		} else
			pthread_mutex_unlock(&tray_lock);

		random_time = rand() % 5 + 2;
		now = time(0);

		pthread_mutex_lock(&print_lock);
		printf(
				ANSI_COLOR_GREEN "[ %ld ] cook started to fill %d'th tray\n" ANSI_COLOR_RESET,
				time(NULL) - START_TIME, total_tray + 1);
		monitor();
		pthread_mutex_unlock(&print_lock);

		while(time(0) - now < random_time)
			;/* filling the tray - busy waiting */

		pthread_mutex_lock(&tray_lock);
		trays++;
		pthread_mutex_unlock(&tray_lock);

		total_tray++;

		pthread_mutex_lock(&print_lock);
		printf(
				ANSI_COLOR_GREEN"[ %ld ] cook fished to fill %d'th tray\n" ANSI_COLOR_RESET,
				time(NULL) - START_TIME, total_tray);
		monitor();
		pthread_mutex_unlock(&print_lock);
	}
}

void *student(void *arg)
{
	pthread_mutex_lock(&student_total_lock);
	student_total++;
	pthread_mutex_unlock(&student_total_lock);

	pthread_mutex_lock(&print_lock);
	printf(
	ANSI_COLOR_GREEN "[ %ld ] %d'th student arrived .. \n" ANSI_COLOR_RESET,
			time(NULL) - START_TIME, student_total);
	monitor();
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
	printf(
			ANSI_COLOR_GREEN "[ %ld ] %d'th student fetched his tray .. \n" ANSI_COLOR_RESET,
			time(NULL) - START_TIME, student_fetch);
	monitor();
	pthread_mutex_unlock(&print_lock);

	pthread_mutex_unlock(&tray_lock);

	pthread_mutex_unlock(&student_wait_lock);

	pthread_exit(NULL);
}

void monitor(void)
{
	printf("\n|-------------------------------------------------------|\n");
	printf(ANSI_COLOR_RED "|\t\t\tCONVEYOR \t\t\t|\n" ANSI_COLOR_RESET);
	printf("|\t\t---------------------- \t\t\t|\n");
	printf("|\t\tTRAYS READY\t:%d \t\t\t|\n", trays);
	printf("|\t\tAVAILABLE PLACE\t:%d \t\t\t|\n", (8 - trays));

	printf(
	ANSI_COLOR_RED "|\tCOOK\t\t\t\tWAITING LINE \t|\n" ANSI_COLOR_RESET);
	printf("|  ----------------\t\t    ------------------- |\n");
	printf("|  %s\t\t\t\t      %d \t|\n", cook_sleep_str[cook_sleep],
			(student_total - student_fetch));
	printf("|\t\t\t\t    Students are waiting|\n");
	if(cook_sleep == 0)
		printf("|  Filling %d'th tray\t\t\t\t\t|\n", (total_tray + 1));
	printf("|\t\t\t\t\t\t\t|\n");

	printf("|\t\t\t\t\t\t\t|\n");
	printf(
			ANSI_COLOR_RED "|\t\t    CAFETERIA STATISTICS \t\t|\n" ANSI_COLOR_RESET);
	printf("|\t\t---------------------------- \t\t|\n");
	printf("|\t\tTOTAL TRAYS FILLED\t:%d\t\t|\n", total_tray);
	printf("|\t\tTOTAL STUDENTS CAME\t:%d\t\t|\n", student_total);
	printf("|\t\tTOTAL STUDENTS FETCHED\t:%d\t\t|\n", student_fetch);
	printf("|-------------------------------------------------------|\n\n");
}

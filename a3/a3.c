#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h> /* sleep */
#include <time.h> /* time  */

#define OUTSIDE 0
#define ENTERING 1
#define STUDYING 2
#define LEAVING 3
#define WAITING 4

#define STUDENT_SIZE 5

int student_num = 2; // total number of students in library
int student_studying = 2; // students studying
int student_waiting = 0; // students waiting
int student_enter = 2;
int student_left = 0;
char *status[] = {"OUTSIDE", "ENTERING", "STUDYING", "LEAVING", "WAITING"}; // student status

time_t timer;
struct tm* tm_info;

// semaphore to check whether student can leave or not
sem_t wait_leave;
sem_t lock;

/**
 * Thread specific information for student threads
 *
 * This struct is used to instruct the thread about
 * the shared resources and the semaphore to use
 */
typedef struct _student_t {
	int sid;
	int status;
} student_t;

/**
 * Function prototypes
 */
void *student(void *arg);
void print_info(student_t *student);
int decide();

/**
 * Student routine
 * 
 * This is the routine for the student.
 * It behaves independently, and check on other student when leaving.
 *
 * The student thread has to:
 * 1. Increment semaphore when entering.
 * 2. Decrrement semaphore when leaving.
 * 3. The should be at least 2 students studying at all time.
 * 4. The second to last student need to wait for the last student to finish studying before leave
 *	In this scenario, student A is waiting for student B to leave.
 *	A will busy waiting B until B is leaving and give signal to A to leave
 *	Then A can leave.
 */
void *student(void *arg) {
	
	/* Initialize some data to put in the library */
	student_t *s = (student_t *) arg;
	int value;
	print_info(s);
	sleep(2); // help to see if starter student decide to leave immediately

	/* Student loop  */
	while(1) {
		value = decide();
		// try to figure out student action
		sem_trywait(&lock);
		if (s->status == OUTSIDE && value == 1) {
			s->status = ENTERING;
			student_num += 1;
			student_enter += 1;
            print_info(s);
			sem_post(&wait_leave);
		}
		else if (s->status == ENTERING && value == 1) {
			s->status = STUDYING;
			student_studying += 1;
            print_info(s);
		}
		else if (s->status == STUDYING && value == 1) {
			s->status = WAITING;
			student_studying -= 1;
			student_waiting += 1;
            print_info(s);
		}
		sem_post(&lock);

		// student ready to leave
		if (s->status == WAITING) {
			// if you are the last student studying
			if (student_waiting == 2 && student_studying == 0 && student_enter == STUDENT_SIZE) {
				s->status = LEAVING;
				student_waiting -= 1;
				student_num -= 1;
				student_left += 1;
				print_info(s);
				sem_post(&wait_leave); // tell the one waiting he can leave now
				break;
			}
			// only when there are at least 3 students inside or second to last is ready to leave
			// sem_wait should be able to handle it
		    sem_wait(&wait_leave);
			s->status = LEAVING;
			student_waiting -= 1;
			student_num -= 1;
			student_left += 1;
			print_info(s);
			break;
		}

		// hold thread to view sequence better, between 1-5s
		// int randomTime = rand() % 5 + 1;
		// sleep(randomTime)
		sleep(2);
	}

	pthread_exit(NULL);
}

/**
 * This function will generate random number to decide the student
 * ENTERING the library,
 * start STUDYING
 * done STUDYING
 * LEAVING the library
 */
int decide() {
	int r = rand() % 2;
	if (r > 0) {
		return 1;
	}
	return 0;
}

void print_info(student_t *s) {
	time(&timer);
    tm_info = localtime(&timer);
    char time_buf[26];
    strftime(time_buf, 26, "%Y-%m-%d %H:%M:%S", tm_info);

	printf("%s Student %d is %s.\n"
		"In library: %d, studying: %d, waiting: %d.\n",
		time_buf, s->sid, status[s->status],
		student_num, student_studying, student_waiting);
}

/**
 * Main routine
 * 
 * The main will have to:
 * 1. Parse the parameters
 * 2. Initialize semaphores
 * 3. Span students
 *
 * @param argc number of parameters
 * @param args command line parameters
 * @returns 0 on success, 1 on error
 */
int main(int argc, char **argv) {
	// Variable definition

	// sem_t study_sem;
	pthread_t *students = malloc(sizeof(pthread_t)*STUDENT_SIZE);
	student_t *s = malloc(sizeof(student_t)*STUDENT_SIZE);
	int i;

	srand(time(NULL)); // random number generator

	// Initialize semaphores
	sem_init(&wait_leave, 0, 0);
	sem_init(&lock, 0, 1);

	// Span threads
	// First 2 threads, student already in library
	for (i=0; i<STUDENT_SIZE; i++) {
		s[i].sid = i;
		s[i].status = OUTSIDE;
		if (i < 2) {
			s[i].status = STUDYING;
		}

		// create threads
		if (pthread_create(&students[i], NULL, student, &s[i]) != 0) {
			fprintf(stderr, "ERROR: Cannot create thread # %d\n", i);
			break;
		}
	} 

	// Join threads
	for (i=0; i<STUDENT_SIZE; i++) {
		if (pthread_join(students[i], NULL) != 0) {
			fprintf(stderr, "ERROR: Cannot join thread #%d\n", i);
		}	
	}

	// close semaphore
	sem_destroy(&wait_leave);
	// free space
	free(students);
	free(s);

	return 0;
}

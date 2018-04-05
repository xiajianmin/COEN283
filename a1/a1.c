#include <stdio.h> /* printf, perror */
#include <string.h> /* strcpy, strstr */
#include <stdlib.h> /* exit */
#include <pthread.h>

#define BUFFER_SIZE 2056
#define MAX_WORD 16

int total_word_count = 0;
int total_input_word_count = 0;
char word[MAX_WORD+1];

/**
 * Struct to pass param to thread
 * tid: keep track of the thread id
 * lineBuffer: 
 */
typedef struct _line_t {
	int tid;
	char lineBuffer[BUFFER_SIZE];
} line_t;

/**
 * This function count the number of word occurence
 * from given string.
 * args: (char*) string
 * return: (int) count
 * 
 */
void* wordCount(void* arg)
{
	line_t* ptr = (line_t *) arg;
	int word_count = 0;
	int input_word_count = 0;
	// tokenize word
	char *saveptr;
	char *token = strtok_r(ptr->lineBuffer, " .", &saveptr);
	while (token)
	{
		// compare string
		if (strstr(token, word))
		{
			input_word_count++;
		}
		//printf("%s ", token);
		token = strtok_r(NULL, " ", &saveptr);
		// increment word count
		word_count++;
	}

	//result_t* r = malloc(sizeof(result_t));
	total_word_count += word_count;
	total_input_word_count += input_word_count;
	printf("Total word on segment #%d is %d.\n", ptr->tid, word_count);
	printf("I'm thread #%d find %d occurences.\n", ptr->tid, input_word_count);
	pthread_exit(NULL);
}

int main(int argc, char **argv)
{
	FILE* fp;
	char* filename = "InputFileAssignment1.txt";
	char lineBuffer[BUFFER_SIZE], ch;
	pthread_t *threads;
	line_t *lines;
	int i, num_threads=0;

	strcpy(word, "Glassdoor");
	// Default input file and search word
	if (argc < 2) {
		printf("Use default input file. Use default search word.\n\n");
	}
	// user input file
	if (argc >= 2) {
		filename = argv[1];
		printf("Use input file %s.\n", filename);
	}
	// user input word search
	if (argc >= 3) {
		strncpy(word, argv[2], MAX_WORD);
		word[MAX_WORD] = '\0';
		printf("Search word: %s.\n", word);
	}

	// read file
	fp = fopen(filename, "r");
	if (fp == NULL)
	{
		printf("Invalid filename or file is empty.\n");
		return 1;
	}

	// find # of lines, @threads
	while (!feof(fp))
	{
		ch = fgetc(fp);
		if (ch == '\n')
		{
			num_threads++;
		}
	}
	printf("There are %d lines in the file.\n", num_threads);
	// reset file pointer
	rewind(fp);

	// allocate memory for threads and lines
	threads = malloc(sizeof (pthread_t)*num_threads);
	lines = malloc(sizeof (line_t)*num_threads);

	while (fgets(lineBuffer, BUFFER_SIZE, fp)) 
	{
		if (i >= num_threads)
		{
			break;
		}
		// initialize struct
		lines[i].tid = i;
		strcpy(lines[i].lineBuffer, lineBuffer);
		// create child process
		if (pthread_create(&threads[i], NULL, wordCount, &lines[i]) != 0)
		{
			fprintf(stderr, "ERROR: Cannot create thread # %d\n", i);
			break;
		}
		i++;
	}
	// join thread
	for (i=0; i<num_threads; i++)
	{
		if (pthread_join(threads[i], NULL) != 0)
		{
			fprintf(stderr, "ERROR: Cannot join thread # %d\n", i);
		}
	}
	
	printf("Total word count: %d.\n", total_word_count);
	printf("Word %s is found %d times.\n", word, total_input_word_count);
	// close file
	fclose(fp);
	// free space
	free(threads);
	free(lines);
	return 0;
}

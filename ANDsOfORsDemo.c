#include "MultiClassTsetlinMachine.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define NUMBER_OF_EXAMPLES 20000

int X_train[NUMBER_OF_EXAMPLES][FEATURES];
int y_train[NUMBER_OF_EXAMPLES];

int X_test[NUMBER_OF_EXAMPLES][FEATURES];
int y_test[NUMBER_OF_EXAMPLES];

void read_file(void)
{
	FILE * fp;
	char * line = NULL;
	size_t len = 0;

	const char *s = " ";
	char *token = NULL;

	fp = fopen("ANDsOfORsTrainingData.txt", "r");
	if (fp == NULL) {
		printf("Error opening\n");
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < NUMBER_OF_EXAMPLES; i++) {
		getline(&line, &len, fp);

		token = strtok(line, s);
		for (int j = 0; j < FEATURES; j++) {
			X_train[i][j] = atoi(token);
			token=strtok(NULL,s);
		}
		y_train[i] = atoi(token);
	}

	fp = fopen("ANDsOfORsTestingData.txt", "r");
	if (fp == NULL) {
		printf("Error opening\n");
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < NUMBER_OF_EXAMPLES; i++) {
		getline(&line, &len, fp);

		token = strtok(line, s);
		for (int j = 0; j < FEATURES; j++) {
			X_test[i][j] = atoi(token);
			token=strtok(NULL,s);
		}
		y_test[i] = atoi(token);
	}
}


int main(void)
{	
	srand(time(NULL));

	read_file();

	struct MultiClassTsetlinMachine *mc_tsetlin_machine = CreateMultiClassTsetlinMachine();

	float average = 0.0;
	for (int e = 0; e < 100; e++) {
		mc_tm_initialize(mc_tsetlin_machine);
		clock_t start_total = clock();
		mc_tm_fit(mc_tsetlin_machine, X_train, y_train, NUMBER_OF_EXAMPLES, 200, 1.0);
		clock_t end_total = clock();
		double time_used = ((double) (end_total - start_total)) / CLOCKS_PER_SEC;

		printf("EPOCH %d TIME: %f\n", e+1, time_used);
		average += mc_tm_evaluate(mc_tsetlin_machine, X_test, y_test, NUMBER_OF_EXAMPLES);

		printf("Average accuracy: %f\n", average/(e+1));

		for (int i = 0; i < 2; i++) {
			printf("Class %d\n", i);
			for (int j = 0; j < CLAUSES; ++j) {
				printf("\tClause %d:", j);
				for (int k = 0; k < FEATURES; ++k) {
					printf(" %03d", tm_get_state(mc_tsetlin_machine->tsetlin_machines[i], j, k));
				}
				printf("\n");
			}
		}
	}

	return 0;
}

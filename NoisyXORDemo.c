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

	fp = fopen("NoisyMultiValuedXORTrainingData.txt", "r");
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

	fp = fopen("NoisyMultiValuedXORTestingData.txt", "r");
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

	float accuracy_sum = 0.0;
	for (int i = 0; i < 100; i++) {
		clock_t start_total = clock();
		mc_tm_fit(mc_tsetlin_machine, X_train, y_train, NUMBER_OF_EXAMPLES, 2500, 1.1);
		clock_t end_total = clock();
		double time_used = ((double) (end_total - start_total)) / CLOCKS_PER_SEC;

		printf("EPOCH %d TIME: %f\n", i+1, time_used);
		float accuracy = mc_tm_evaluate(mc_tsetlin_machine, X_test, y_test, NUMBER_OF_EXAMPLES);

		accuracy_sum += accuracy;

		printf("\n**** TM 1 ****\n");
		tm_print(mc_tsetlin_machine->tsetlin_machines[0]);

		printf("\n**** TM 2 ****\n");

		tm_print(mc_tsetlin_machine->tsetlin_machines[1]);

		printf("Accuracy: %f (%f)\n", accuracy, accuracy_sum / (i + 1));

		mc_tm_initialize(mc_tsetlin_machine);
	}

	return 0;
}

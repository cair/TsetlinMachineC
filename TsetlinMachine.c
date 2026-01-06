/*

Copyright (c) 2019 Ole-Christoffer Granmo

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

This code implements the Tsetlin Machine from paper arXiv:1804.01508
https://arxiv.org/abs/1804.01508

*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "TsetlinMachine.h"

/**************************************/
/*** The Multiclass Tsetlin Machine ***/
/**************************************/

/*** Initialize Tsetlin Machine ***/
struct TsetlinMachine *CreateTsetlinMachine(int sign)
{
	struct TsetlinMachine *tm = (void *)malloc(sizeof(struct TsetlinMachine));

	/* Set up the Tsetlin Machine structure */

	tm_initialize(tm, sign);
	
	return tm;
}


void tm_initialize(struct TsetlinMachine *tm, int sign)
{
	for (int j = 0; j < COMPONENTS; j++) {				
		for (int k = 0; k < VALUES; k++) {
			// if (1.0 * rand()/RAND_MAX <= 0.5) {
			// 	(*tm).ta_state[j][k] = NUMBER_OF_STATES;
			// } else {
			// 	(*tm).ta_state[j][k] = NUMBER_OF_STATES + 1;
			// }

			//(*tm).ta_state[j][k] = NUMBER_OF_STATES;
			(*tm).ta_state[j][k] = 1;
		}
	}

	for (int j = 0; j < CLAUSES; j++) {				
		for (int k = 0; k < LAYER_TWO_FEATURES; k++) {
			// if (1.0 * rand()/RAND_MAX <= 0.5) {
			// 	(*tm).ta_state[j][k] = NUMBER_OF_STATES;
			// } else {
			// 	(*tm).ta_state[j][k] = NUMBER_OF_STATES + 1;
			// }

			//(*tm).ta_state[j][k] = NUMBER_OF_STATES;
			(*tm).layer_two_ta_state[j][k] = 1;
		}
	}

	printf("SIGN: %d\n", sign);

	for (int k = 0; k < pow(2, VARIABLES); k++) {
		printf("Clause %d:", k);
		int one_count = 0;
		for (int l = 0; l < VARIABLES; l++) {
			(*tm).clause_components[k][l] = (k >> l) % 2;
			one_count += (*tm).clause_components[k][l];
			printf(" %d", (*tm).clause_components[k][l]);			
		}
		// if ((one_count % 2) == 0) {
		// 	if (sign > 0) {
		// 		(*tm).clause_weight[k] = 1;
		// 	} else {
		// 		(*tm).clause_weight[k] = -1;
		// 	}
		// } else {
		// 	if (sign > 0) {
		// 		(*tm).clause_weight[k] = -1;
		// 	} else {
		// 		(*tm).clause_weight[k] = 1;
		// 	}
		// }

		(*tm).clause_weight[k] = 1 - 2*(rand() % 2);

		printf(" (%d)\n", (*tm).clause_weight[k]);
	}
}

/* Translates automata state to action */
static inline int action(int state)
{
		return state > NUMBER_OF_STATES;
}

/* Calculate the output of each clause using the actions of each Tsetline Automaton. */
/* Output is stored an internal output array. */

static inline void calculate_clause_output(struct TsetlinMachine *tm, int Xi[], int predict)
{
	// Evaluate components on each variable...

	for (int k = 0; k < VARIABLES; k++) {
		for (int l = 0; l < COMPONENTS; l++) {
			(*tm).layer_two_X[k][l] = 1;
			for (int m = 0; m < VALUES; m++) {
				int action_include = action((*tm).ta_state[l][m]);

				if ((action_include == 1 && Xi[k*VALUES + m] == 0)) {
					(*tm).layer_two_X[k][l] = 0;
					break;
				}
			}
		}
	}

	// Evaluate each clause using layer_two_X

	for (int j = 0; j < CLAUSES; j++) {
		(*tm).clause_output[j] = 1;

		for (int k = 0; k < VARIABLES; k++) {
			for (int l = 0; l < COMPONENTS; l++) {
				int action_include = action((*tm).layer_two_ta_state[j][k][l]);

				if (action_include == 1 && (*tm).layer_two_X[variable][l] == 0) {
					(*tm).clause_output[j] = 0;
					break;
				} 
			}
		}
	}
}

/* Sum up the votes for each class (this is the multiclass version of the Tsetlin Machine) */
static inline int sum_up_class_votes(struct TsetlinMachine *tm)
{
	int class_sum = 0;
	for (int j = 0; j < CLAUSES; j++) {
		class_sum += (*tm).clause_output[j]*(*tm).clause_weight[j];
	}
	
	class_sum = (class_sum > THRESHOLD) ? THRESHOLD : class_sum;
	class_sum = (class_sum < -THRESHOLD) ? -THRESHOLD : class_sum;

	return class_sum;
}

/* Get the state of a specific automaton, indexed by clause, feature, and automaton type (include/include negated). */
int tm_get_state(struct TsetlinMachine *tm, int component, int value)
{
	return (*tm).ta_state[component][value];
}

/* Get the state of a specific automaton, indexed by clause, feature, and automaton type (include/include negated). */
int tm_get_state_layer_two(struct TsetlinMachine *tm, int clause, int variable, int component)
{
	return (*tm).layer_two_ta_state[clause][variable][component];
}

/*************************************************/
/*** Type I Feedback (Combats False Negatives) ***/
/*************************************************/

static inline void type_i_feedback(struct TsetlinMachine *tm, int Xi[], int j, float s)
{
	// Pick random component

	int k = rand() % VARIABLES;

	if ((*tm).clause_output[j] == 0) {
		for (int l = 0; l < VALUES; l++) {
			(*tm).ta_state[(*tm).clause_components[j][k]][l] -= ((*tm).ta_state[(*tm).clause_components[j][k]][l] > 1) && (1.0*rand()/RAND_MAX <= 1.0/s);
		}
	} else if ((*tm).clause_output[j] == 1) {	
		if ((*tm).clause_weight[j] > 0 && (*tm).clause_weight[j] < THRESHOLD) {
			(*tm).clause_weight[j] += 1;
		} else if ((*tm).clause_weight[j] < 0 && (*tm).clause_weight[j] > -THRESHOLD) {
			(*tm).clause_weight[j] -= 1;
		}

		for (int l = 0; l < VALUES; l++) {
			if (Xi[k*VALUES + l] == 1) {
				if (s >= 1.0 || (s < 1.0 && (1.0*rand()/RAND_MAX <= s)))  {
					(*tm).ta_state[(*tm).clause_components[j][k]][l] += ((*tm).ta_state[(*tm).clause_components[j][k]][l] < NUMBER_OF_STATES*2);
				} 
			} else {
				(*tm).ta_state[(*tm).clause_components[j][k]][l] -= ((*tm).ta_state[(*tm).clause_components[j][k]][l] > 1) && (1.0*rand()/RAND_MAX <= 1.0/s);
			}
		}
	}
}


/**************************************************/
/*** Type II Feedback (Combats False Positives) ***/
/**************************************************/

static inline void type_ii_feedback(struct TsetlinMachine *tm, int Xi[], int j) {
	int action_include;


	if ((*tm).clause_output[j] == 1) {
	
		if ((*tm).clause_weight[j] > 0) {
			(*tm).clause_weight[j] -= 1;
			if ((*tm).clause_weight[j] == 0) {
				(*tm).clause_weight[j] = -1;
			}
		} else if ((*tm).clause_weight[j] < 0) {
			(*tm).clause_weight[j] += 1;
			if ((*tm).clause_weight[j] == 0) {
				(*tm).clause_weight[j] = 1;
			}
		}

		int k = rand() % VARIABLES;

		for (int l = 0; l < VALUES; l++) {
			action_include = action((*tm).ta_state[(*tm).clause_components[j][k]][l]);
			(*tm).ta_state[(*tm).clause_components[j][k]][l] += (action_include == 0 && ((*tm).ta_state[(*tm).clause_components[j][k]][l]) < NUMBER_OF_STATES*2) && (Xi[k*VALUES + l] == 0);
		}
	}
}

/******************************************/
/*** Online Training of Tsetlin Machine ***/
/******************************************/

// The Tsetlin Machine can be trained incrementally, one training example at a time.
// Use this method directly for online and incremental training.

void tm_update(struct TsetlinMachine *tm, int Xi[], int target, float s) {
	/*******************************/
	/*** Calculate Clause Output ***/
	/*******************************/

	calculate_clause_output(tm, Xi, UPDATE);

	/***************************/
	/*** Sum up Clause Votes ***/
	/***************************/

	int class_sum = sum_up_class_votes(tm);

	/*************************************/
	/*** Calculate Feedback to Clauses ***/
	/*************************************/

	// Calculate feedback to clauses
	
	// Pick random clause

	int true_clauses[CLAUSES];
	int number_of_true_clauses = 0;
	for (int j = 0; j < CLAUSES; j++) {
		if ((*tm).clause_output[j] == 1) {
			true_clauses[number_of_true_clauses] = j;
			number_of_true_clauses++;
		}	
	}

	int clause;
	if (number_of_true_clauses > 0) {
		clause = true_clauses[rand() % number_of_true_clauses];
	} else {
		clause = rand() % CLAUSES;
	}

	int feedback_to_clause = (2*target-1)*((*tm).clause_weight[clause])*(1.0*rand()/RAND_MAX <= (1.0/(THRESHOLD*2))*(THRESHOLD + (1 - 2*target)*class_sum));
	
	if (feedback_to_clause > 0) {
		type_i_feedback(tm, Xi, clause, s);
	} else if (feedback_to_clause < 0) {
		type_ii_feedback(tm, Xi, clause);
	}
}

int tm_score(struct TsetlinMachine *tm, int Xi[]) {
	/*******************************/
	/*** Calculate Clause Output ***/
	/*******************************/

	calculate_clause_output(tm, Xi, PREDICT);

	/***************************/
	/*** Sum up Clause Votes ***/
	/***************************/

	return sum_up_class_votes(tm);
}

void tm_print(struct TsetlinMachine *tm) {

	for (int j = 0; j < CLAUSES; j++) {
		printf("CLAUSE %d (%+d)\n", j, (*tm).clause_weight[j]);

		for (int k = 0; k < COMPONENTS; k++) {
			printf("COMPONENT %d:", k);

			for (int l = 0; l < VALUES; l++) {
				int action_include = action((*tm).ta_state[k][l]);
				if (action_include) {
					printf(" ¬x%d(%d) ", l, (*tm).ta_state[k][l]);
				}
		}
		printf("\n");
		}
		
	}
}


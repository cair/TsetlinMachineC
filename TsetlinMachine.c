/*

Copyright (c) 2025 Ole-Christoffer Granmo and the University of Agder

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

#include "TsetlinMachine.h"

/**************************************/
/*** The Multiclass Tsetlin Machine ***/
/**************************************/

/*** Initialize Tsetlin Machine ***/
struct TsetlinMachine *CreateTsetlinMachine()
{
	struct TsetlinMachine *tm = (void *)malloc(sizeof(struct TsetlinMachine));

	/* Set up the Tsetlin Machine structure */

	tm_initialize(tm);
	
	return tm;
}


void tm_initialize(struct TsetlinMachine *tm)
{
	for (int v = 0; v < VARIABLES; v++) {
		for (int j = 0; j < CLAUSES; j++) {				
			for (int k = 0; k < FEATURES; k++) {
				if (1.0 * rand()/RAND_MAX <= 0.5) {
					(*tm).ta_state[v][j][k] = NUMBER_OF_STATES;
				} else {
					(*tm).ta_state[v][j][k] = NUMBER_OF_STATES + 1;
				}
			}
		}
	}
}

/* Translates automata state to action */
static inline int action(int state)
{
		return state > NUMBER_OF_STATES;
}

/* Calculate the output of each clause using the actions of each Tsetline Automaton. */
/* Output is stored an internal output array. */

static inline void calculate_clause_output(struct TsetlinMachine *tm, int Xi[])
{
	for (int j = 0; j < CLAUSES; j++) {
		(*tm).joint_clause_output[j] = 1;

		for (int v = 0; v < VARIABLES; v++) {
			(*tm).clause_output[v][j] = 1;

			for (int k = 0; k < FEATURES; k++) {
				int action_include = action((*tm).ta_state[v][j][k]);

				if ((action_include == 1) && (Xi[v*FEATURES + k] == 1)) {
					(*tm).clause_output[v][j] = 0;
					break;
				}
			}

			(*tm).joint_clause_output[j] = (*tm).joint_clause_output[j] && (*tm).clause_output[v][j];
		}
	}
}

/* Sum up the votes for each class (this is the multiclass version of the Tsetlin Machine) */
static inline int sum_up_class_votes(struct TsetlinMachine *tm)
{
	int class_sum = 0;
	for (int j = 0; j < CLAUSES; j++) {
		int sign = 1 - 2 * (j & 1);
		class_sum += (*tm).joint_clause_output[j]*sign;
	}
	
	class_sum = (class_sum > THRESHOLD) ? THRESHOLD : class_sum;
	class_sum = (class_sum < -THRESHOLD) ? -THRESHOLD : class_sum;

	return class_sum;
}

/* Get the state of a specific automaton, indexed by clause, feature, and automaton type (include/include negated). */
int tm_get_state(struct TsetlinMachine *tm, int variable, int clause, int feature)
{
	return (*tm).ta_state[variable][clause][feature];
}

/*************************************************/
/*** Type I Feedback (Combats False Negatives) ***/
/*************************************************/

static inline void type_i_feedback(struct TsetlinMachine *tm, int Xi[], int v, int j, float s)
{
	if ((*tm).joint_clause_output[j] == 0)	{
		for (int k = 0; k < FEATURES; k++) {
			(*tm).ta_state[v][j][k] -= ((*tm).ta_state[v][j][k] > 1) && (1.0*rand()/RAND_MAX <= 1.0/s);
			(*tm).ta_state[v][j][k] -= ((*tm).ta_state[v][j][k] > 1) && (1.0*rand()/RAND_MAX <= 1.0/s);
			(*tm).ta_state[v][j][k] -= ((*tm).ta_state[v][j][k] > 1) && (1.0*rand()/RAND_MAX <= 1.0/s);
			(*tm).ta_state[v][j][k] -= ((*tm).ta_state[v][j][k] > 1) && (1.0*rand()/RAND_MAX <= 1.0/s);
			(*tm).ta_state[v][j][k] -= ((*tm).ta_state[v][j][k] > 1) && (1.0*rand()/RAND_MAX <= 1.0/s);
		}
	} else if ((*tm).joint_clause_output[j] == 1) {	
		for (int k = 0; k < FEATURES; k++) {
			if (Xi[v*FEATURES + k] == 0) {
				(*tm).ta_state[v][j][k] += ((*tm).ta_state[v][j][k] < NUMBER_OF_STATES*2) && (BOOST_TRUE_POSITIVE_FEEDBACK == 1 || 1.0*rand()/RAND_MAX <= (s-1)/s);
			} else {				
				(*tm).ta_state[v][j][k] -= ((*tm).ta_state[v][j][k] > 1) && (1.0*rand()/RAND_MAX <= 1.0/s);
				(*tm).ta_state[v][j][k] -= ((*tm).ta_state[v][j][k] > 1) && (1.0*rand()/RAND_MAX <= 1.0/s);
				(*tm).ta_state[v][j][k] -= ((*tm).ta_state[v][j][k] > 1) && (1.0*rand()/RAND_MAX <= 1.0/s);
				(*tm).ta_state[v][j][k] -= ((*tm).ta_state[v][j][k] > 1) && (1.0*rand()/RAND_MAX <= 1.0/s);
				(*tm).ta_state[v][j][k] -= ((*tm).ta_state[v][j][k] > 1) && (1.0*rand()/RAND_MAX <= 1.0/s);
			}
		}
	}
}


/**************************************************/
/*** Type II Feedback (Combats False Positives) ***/
/**************************************************/

static inline void type_ii_feedback(struct TsetlinMachine *tm, int Xi[], int v, int j) {
	if ((*tm).joint_clause_output[j] == 1) {
		for (int k = 0; k < FEATURES; k++) { 
			(*tm).ta_state[v][j][k] += ((*tm).ta_state[v][j][k] < (NUMBER_OF_STATES + 1)) && (Xi[v*FEATURES + k] == 1);
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

	calculate_clause_output(tm, Xi);

	/***************************/
	/*** Sum up Clause Votes ***/
	/***************************/

	int class_sum = sum_up_class_votes(tm);

	/*************************************/
	/*** Calculate Feedback to Clauses ***/
	/*************************************/

	// Calculate feedback to clauses

	for (int j = 0; j < CLAUSES; j++) {
		(*tm).feedback_to_clauses[j] = (2*target-1)*(1 - 2 * (j & 1))*(1.0*rand()/RAND_MAX <= (1.0/(THRESHOLD*2))*(THRESHOLD + (1 - 2*target)*class_sum));
	}
	
	
	/*********************************/
	/*** Train Individual Automata ***/
	/*********************************/

	for (int j = 0; j < CLAUSES; j++) {
		if ((*tm).feedback_to_clauses[j] > 0) {
			for (int v = 0; v < VARIABLES; v++) {
				type_i_feedback(tm, Xi, v, j, s);
			}
		} else if ((*tm).feedback_to_clauses[j] < 0) {
			for (int v = 0; v < VARIABLES; v++) {
				type_ii_feedback(tm, Xi, v, j);
			}
		}
	}
}

int tm_score(struct TsetlinMachine *tm, int Xi[]) {
	/*******************************/
	/*** Calculate Clause Output ***/
	/*******************************/

	calculate_clause_output(tm, Xi);

	/***************************/
	/*** Sum up Clause Votes ***/
	/***************************/

	return sum_up_class_votes(tm);
}



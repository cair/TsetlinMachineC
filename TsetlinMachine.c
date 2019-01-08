/*

Copyright (c) 2018 Ole-Christoffer Granmo

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
	for (int j = 0; j < CLAUSES; j++) {				
		for (int k = 0; k < FEATURES; k++) {
			if (1.0 * rand()/RAND_MAX <= 0.5) {
				(*tm).ta_state[j][k][0] = NUMBER_OF_STATES;
				(*tm).ta_state[j][k][1] = NUMBER_OF_STATES + 1; 
			} else {
				(*tm).ta_state[j][k][0] = NUMBER_OF_STATES + 1;
				(*tm).ta_state[j][k][1] = NUMBER_OF_STATES; // Deviation, should be random
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

static inline void calculate_clause_output(struct TsetlinMachine *tm, int Xi[], int predict)
{
	int j, k;
	int action_include, action_include_negated;
	int all_exclude;

	for (j = 0; j < CLAUSES; j++) {
		(*tm).clause_output[j] = 1;
		all_exclude = 1;
		for (k = 0; k < FEATURES; k++) {
			action_include = action((*tm).ta_state[j][k][0]);
			action_include_negated = action((*tm).ta_state[j][k][1]);

			all_exclude = all_exclude && !(action_include == 1 || action_include_negated == 1);

			if ((action_include == 1 && Xi[k] == 0) || (action_include_negated == 1 && Xi[k] == 1)) {
				(*tm).clause_output[j] = 0;
				break;
			}
		}

		(*tm).clause_output[j] = (*tm).clause_output[j] && !(predict == PREDICT && all_exclude == 1);
	}
}

/* Sum up the votes for each class (this is the multiclass version of the Tsetlin Machine) */
static inline int sum_up_class_votes(struct TsetlinMachine *tm)
{
	int class_sum = 0;
	for (int j = 0; j < CLAUSES; j++) {
		int sign = 1 - 2 * (j & 1);
		class_sum += (*tm).clause_output[j]*sign;
	}
	
	class_sum = (class_sum > THRESHOLD) ? THRESHOLD : class_sum;
	class_sum = (class_sum < -THRESHOLD) ? -THRESHOLD : class_sum;

	return class_sum;
}

/* Get the state of a specific automaton, indexed by clause, feature, and automaton type (include/include negated). */
int tm_get_state(struct TsetlinMachine *tm, int clause, int feature, int automaton_type)
{
	return (*tm).ta_state[clause][feature][automaton_type];
}

/*************************************************/
/*** Type I Feedback (Combats False Negatives) ***/
/*************************************************/

static inline void type_i_feedback(struct TsetlinMachine *tm, int Xi[], int j, float s)
{
	if ((*tm).clause_output[j] == 0)	{
		for (int k = 0; k < FEATURES; k++) {
			(*tm).ta_state[j][k][0] -= ((*tm).ta_state[j][k][0] > 1) && (1.0*rand()/RAND_MAX <= 1.0/s);
								
			(*tm).ta_state[j][k][1] -= ((*tm).ta_state[j][k][1] > 1) && (1.0*rand()/RAND_MAX <= 1.0/s);
		}
	} else if ((*tm).clause_output[j] == 1) {					
		for (int k = 0; k < FEATURES; k++) {
			if (Xi[k] == 1) {
				(*tm).ta_state[j][k][0] += ((*tm).ta_state[j][k][0] < NUMBER_OF_STATES*2) && (BOOST_TRUE_POSITIVE_FEEDBACK == 1 || 1.0*rand()/RAND_MAX <= (s-1)/s);

				(*tm).ta_state[j][k][1] -= ((*tm).ta_state[j][k][1] > 1) && (1.0*rand()/RAND_MAX <= 1.0/s);
			} else if (Xi[k] == 0) {
				(*tm).ta_state[j][k][1] += ((*tm).ta_state[j][k][1] < NUMBER_OF_STATES*2) && (BOOST_TRUE_POSITIVE_FEEDBACK == 1 || 1.0*rand()/RAND_MAX <= (s-1)/s);
				
				(*tm).ta_state[j][k][0] -= ((*tm).ta_state[j][k][0] > 1) && (1.0*rand()/RAND_MAX <= 1.0/s);
			}
		}
	}
}


/**************************************************/
/*** Type II Feedback (Combats False Positives) ***/
/**************************************************/

static inline void type_ii_feedback(struct TsetlinMachine *tm, int Xi[], int j) {
	int action_include;
	int action_include_negated;

	if ((*tm).clause_output[j] == 1) {
		for (int k = 0; k < FEATURES; k++) { 
			action_include = action((*tm).ta_state[j][k][0]);
			action_include_negated = action((*tm).ta_state[j][k][1]);

			(*tm).ta_state[j][k][0] += (action_include == 0 && (*tm).ta_state[j][k][0] < NUMBER_OF_STATES*2) && (Xi[k] == 0);
			(*tm).ta_state[j][k][1] += (action_include_negated == 0 && (*tm).ta_state[j][k][1] < NUMBER_OF_STATES*2) && (Xi[k] == 1);
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
	for (int j = 0; j < CLAUSES; j++) {
		(*tm).feedback_to_clauses[j] = (2*target-1)*(1 - 2 * (j & 1))*(1.0*rand()/RAND_MAX <= (1.0/(THRESHOLD*2))*(THRESHOLD + (1 - 2*target)*class_sum));
	}
	
	/*********************************/
	/*** Train Individual Automata ***/
	/*********************************/

	for (int j = 0; j < CLAUSES; j++) {
		if ((*tm).feedback_to_clauses[j] > 0) {
			type_i_feedback(tm, Xi, j, s);
		} else if ((*tm).feedback_to_clauses[j] < 0) {
			type_ii_feedback(tm, Xi, j);
		}
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



/*

Copyright (c) 2026 Ole-Christoffer Granmo and the University of Agder

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
		for (int k = 0; k < CLAUSE_COMPONENTS; k++) {				
			for (int l = 0; l < FEATURES; l++) {
				if (1.0 * rand()/RAND_MAX <= 0.5) {
					(*tm).ta_state[j][0][k][l] = NUMBER_OF_STATES;
					(*tm).ta_state[j][0][k][l + FEATURES] = NUMBER_OF_STATES + 1;
				} else {
					(*tm).ta_state[j][0][k][l] = NUMBER_OF_STATES + 1;
					(*tm).ta_state[j][0][k][l + FEATURES] = NUMBER_OF_STATES;
				}
			}
		}
	}

	for (int j = 0; j < CLAUSES; j++) {
		for (int k = 0; k < CLAUSE_COMPONENTS; k++) {				
			for (int l = 0; l < FEATURES; l++) {
				if (1.0 * rand()/RAND_MAX <= 0.5) {
					(*tm).ta_state[j][1][k][l] = NUMBER_OF_STATES;
					(*tm).ta_state[j][1][k][l + FEATURES] = NUMBER_OF_STATES + 1;
				} else {
					(*tm).ta_state[j][1][k][l] = NUMBER_OF_STATES + 1;
					(*tm).ta_state[j][1][k][l + FEATURES] = NUMBER_OF_STATES;
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

static inline void calculate_clause_output(struct TsetlinMachine *tm, int Xi[], int predict)
{
	int action_include;

	for (int j = 0; j < CLAUSES; j++) {
		(*tm).clause_output[j] = 0; // Here, we count how many times the rolled out clauses are True.

		// Go through the clause components, one needs to be True to make the first part of the clause True.
		
		int local_clause_output_1 = 0;
		for (int k = 0; k < CLAUSE_COMPONENTS; k++) {
			(*tm).clause_component_output[j][0][k] = 1; // One False literal makes the clause component False
			for (int l = 0; l < FEATURES / 2; l++) {
				action_include = action((*tm).ta_state[j][0][k][l]);
				if ((action_include == 1 && Xi[l] == 0)) {
					(*tm).clause_component_output[j][0][k] = 0;
					break;
				}

				action_include = action((*tm).ta_state[j][0][k][l + FEATURES]);
				if ((action_include == 1 && Xi[l + FEATURES] == 0)) {
					(*tm).clause_component_output[j][0][k] = 0;
					break;
				}
			}

			local_clause_output_1 += (*tm).clause_component_output[j][0][k]; // Add one vote her if clause component is True.
		}

		int local_clause_output_2 = 0;
		for (int k = 0; k < CLAUSE_COMPONENTS; k++) {
			(*tm).clause_component_output[j][1][k] = 1; // One False literal makes the clause component False
			for (int l = FEATURES / 2; l < FEATURES; l++) {
				action_include = action((*tm).ta_state[j][1][k][l]);
				if ((action_include == 1 && Xi[l] == 0)) {
					(*tm).clause_component_output[j][1][k] = 0;
					break;
				}

				action_include = action((*tm).ta_state[j][1][k][l + FEATURES]);
				if ((action_include == 1 && Xi[l + FEATURES] == 0)) {
					(*tm).clause_component_output[j][1][k] = 0;
					break;
				}
			}

			local_clause_output_2 += (*tm).clause_component_output[j][0][k]; // Add one vote her if clause component is True.
		}

		local_clause_output_2 = 1;
		(*tm).clause_output[j] = local_clause_output_1 * local_clause_output_2;

		if (Xi[FEATURES-1] == 1 && Xi[FEATURES-2] == 1) {
			if (j < 3 * CLAUSES / 4) {
				(*tm).clause_output[j] = 0;
			}
		} else if (Xi[FEATURES-1] == 1 && Xi[FEATURES-2] == 0) {
			if ((j >=  3 * CLAUSES / 4) || (j < 2 * CLAUSES / 4)) {
				(*tm).clause_output[j] = 0;
			}
		} else if (Xi[FEATURES-1] == 0 && Xi[FEATURES-2] == 1) {
			if ((j >=  2 * CLAUSES / 4) || (j <  CLAUSES / 4)) {
				(*tm).clause_output[j] = 0;
			}
		} else {
			if (j >= CLAUSES / 4) {
				(*tm).clause_output[j] = 0;
			}
		}

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
int tm_get_state(struct TsetlinMachine *tm, int clause, int clause_component, int feature)
{
	return (*tm).ta_state[clause][0][clause_component][feature];
}

/*************************************************/
/*** Type I Feedback (Combats False Negatives) ***/
/*************************************************/

static inline void type_i_feedback(struct TsetlinMachine *tm, int Xi[], int j, int v, int k, float s)
{
	if ((*tm).clause_output[j] == 0 || (*tm).clause_component_output[j][v][k] == 0)	{
		for (int l = 0; l < FEATURES / 2; l++) {
			(*tm).ta_state[j][v][k][l] -= ((*tm).ta_state[j][v][k][l] > 1) && (1.0*rand()/RAND_MAX <= 1.0/s);	

			(*tm).ta_state[j][v][k][l + FEATURES] -= ((*tm).ta_state[j][v][k][l + FEATURES] > 1) && (1.0*rand()/RAND_MAX <= 1.0/s);				
		}
	} else {					
		for (int l = 0; l < FEATURES / 2; l++) {
			if (Xi[l] == 1) {
				(*tm).ta_state[j][v][k][l] += ((*tm).ta_state[j][v][k][l] < NUMBER_OF_STATES*2) && (BOOST_TRUE_POSITIVE_FEEDBACK == 1 || 1.0*rand()/RAND_MAX <= (s-1)/s);
			} else {				
				(*tm).ta_state[j][v][k][l] -= ((*tm).ta_state[j][v][k][l] > 1) && (1.0*rand()/RAND_MAX <= 1.0/s);
			}

			if (Xi[l + FEATURES] == 1) {
				(*tm).ta_state[j][v][k][l + FEATURES] += ((*tm).ta_state[j][v][k][l + FEATURES] < NUMBER_OF_STATES*2) && (BOOST_TRUE_POSITIVE_FEEDBACK == 1 || 1.0*rand()/RAND_MAX <= (s-1)/s);
			} else {				
				(*tm).ta_state[j][v][k][l + FEATURES] -= ((*tm).ta_state[j][v][k][l + FEATURES] > 1) && (1.0*rand()/RAND_MAX <= 1.0/s);
			}
		}
	}
}


/**************************************************/
/*** Type II Feedback (Combats False Positives) ***/
/**************************************************/

static inline void type_ii_feedback(struct TsetlinMachine *tm, int Xi[], int j, int v, int k) {
	int action_include;

	if ((*tm).clause_output[j] > 0 && (*tm).clause_component_output[j][v][k] == 1) {
		for (int l = 0; l < (FEATURES / 2); l++) { 
			action_include = action((*tm).ta_state[j][v][k][l]);
			(*tm).ta_state[j][v][k][l] += (action_include == 0 && (*tm).ta_state[j][v][k][l] < NUMBER_OF_STATES*2) && (Xi[l] == 0);

			action_include = action((*tm).ta_state[j][v][k][l + FEATURES]);
			(*tm).ta_state[j][v][k][l + FEATURES] += (action_include == 0 && (*tm).ta_state[j][v][k][l + FEATURES] < NUMBER_OF_STATES*2) && (Xi[l + FEATURES] == 0);
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
		int sign = 1 - 2 * (j & 1);

		for (int k = 0; k < CLAUSE_COMPONENTS; k++) {
			(*tm).feedback_to_components[j][0][k] = sign*(2*target-1)*(1.0*rand()/RAND_MAX <= (1.0/(THRESHOLD*2))*(THRESHOLD + (1 - 2*target)*class_sum));
		}

		for (int k = 0; k < CLAUSE_COMPONENTS; k++) {
			(*tm).feedback_to_components[j][1][k] = sign*(2*target-1)*(1.0*rand()/RAND_MAX <= (1.0/(THRESHOLD*2))*(THRESHOLD + (1 - 2*target)*class_sum));
		}
	}
	
	/*********************************/
	/*** Train Individual Automata ***/
	/*********************************/

	for (int j = 0; j < CLAUSES; j++) {
		int k = rand() % CLAUSE_COMPONENTS;
		if ((*tm).feedback_to_components[j][0][k] > 0) {
			type_i_feedback(tm, Xi, j, 0, k, s);
		} else if ((*tm).feedback_to_components[j][0][k] < 0) {
			type_ii_feedback(tm, Xi, j, 0, k);
		}

		k = rand() % CLAUSE_COMPONENTS;
		if ((*tm).feedback_to_components[j][1][k] > 0) {
			type_i_feedback(tm, Xi, j, 1, k, s);
		} else if ((*tm).feedback_to_components[j][1][k] < 0) {
			type_ii_feedback(tm, Xi, j, 1, k);
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



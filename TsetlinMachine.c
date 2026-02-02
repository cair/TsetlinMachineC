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
	for (int i = 0; i < CLAUSES; i++) {
		for (int j = 0; j < ROOT_GROUPING_FACTOR; j++) {
			for (int k = 0; k < INTERIOR_ALTERNATIVES; k++) {	
				for (int l = 0; l < INTERIOR_GROUPING_FACTOR; l++) {
					for (int m = 0; m < LEAF_ALTERNATIVES; m++) {
						for (int n = 0; n < LEAF_GROUPING_FACTOR; n++) {

							if (1.0 * rand()/RAND_MAX <= 0.5) {
								(*tm).ta_state[i][j][k][l][m][n] = NUMBER_OF_STATES;
								(*tm).ta_state[i][j][k][l][m][n + LEAF_GROUPING_FACTOR] = NUMBER_OF_STATES + 1;
							} else {
								(*tm).ta_state[i][j][k][l][m][n] = NUMBER_OF_STATES + 1;
								(*tm).ta_state[i][j][k][l][m][n + LEAF_GROUPING_FACTOR] = NUMBER_OF_STATES;
							}
						}
					}
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

	for (int i = 0; i < CLAUSES; i++) {
		(*tm).clause_output[i] = 1;

		for (int j = 0; j < ROOT_GROUPING_FACTOR; j++) {
			// Evaluates interior subtrees, adding up votes from each
			(*tm).interior_vote_sums[i][j] = 0;

			for (int k = 0; k < INTERIOR_ALTERNATIVES; k++) {
				// Multiplies vote sums from multiple leaf groups (AND-combination)

				(*tm).interior_vote_products[i][j][k] = 1; // Stores how many class votes you get per interior alternative (product of leaf vote sums)

				for (int l = 0; l < INTERIOR_GROUPING_FACTOR; l++) {
					// Evaluates leaf alternatives (clause components), adding up the votes
					(*tm).leaf_vote_sum[i][j][k][l] = 0; // Stores how many class votes you get per feature group (vote summation over leaf alternatives)

					for (int m = 0; m < LEAF_ALTERNATIVES; m++) {
						// Evaluates clause component on its feature group
						for (int n = 0; n < LEAF_GROUPING_FACTOR; n++) {
							int feature = j * INTERIOR_GROUPING_FACTOR * LEAF_GROUPING_FACTOR + l * LEAF_GROUPING_FACTOR + n;

							action_include = action((*tm).ta_state[i][j][k][l][m][n]);
							if ((action_include == 1 && Xi[feature] == 0)) {
								(*tm).clause_component_output[i][j][k][l][m] = 0;
								break;
							}

							action_include = action((*tm).ta_state[i][j][k][l][m][n + LEAF_GROUPING_FACTOR]);
							if ((action_include == 1 && Xi[feature + FEATURES] == 0)) {
								(*tm).clause_component_output[i][j][k][l][m] = 0;
								break;
							}
						}

						// Adds up clause component votes (OR-alternatives)
						(*tm).leaf_vote_sum[i][j][k][l] += (*tm).clause_component_output[i][j][k][l][m];
					}

					// Multiplies leaf vote sums (AND-grouping). 
					(*tm).interior_vote_products[i][j][k] *= (*tm).leaf_vote_sum[i][j][k][l];
				}

				// Adds up interior leaf vote sum products (OR-alternatives)
				(*tm).interior_vote_sums[i][j] += (*tm).interior_vote_products[i][j][k];
			}

			// Multiplies interior vote sums (AND-grouping). 
			(*tm).clause_output[i] *= (*tm).interior_vote_sums[i][j];
		}
	}
}

/* Sum up the votes for each class (this is the multiclass version of the Tsetlin Machine) */
static inline int sum_up_class_votes(struct TsetlinMachine *tm)
{
	int class_sum = 0;
	for (int i = 0; i < CLAUSES; i++) {
		int sign = 1 - 2 * (i & 1);
		class_sum += (*tm).clause_output[i]*sign;
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

static inline void type_i_feedback(struct TsetlinMachine *tm, int Xi[], int i, int j, int k, int l, int m, float s)
{
	if ((*tm).clause_output[clause] == 0 || (*tm).clause_component_output[i][j][k][l][m] == 0)	{
		for (int n = feature_index; n < feature_index + LEAF_GROUPING_FACTOR; n++) {
			(*tm).ta_state[i][j][k][l][m][n] -= ((*tm).ta_state[cla][j][k][l] > 1) && (1.0*rand()/RAND_MAX <= 1.0/s);	

			(*tm).ta_state[i][j][k][l + FEATURES] -= ((*tm).ta_state[i][j][k][l + FEATURES] > 1) && (1.0*rand()/RAND_MAX <= 1.0/s);				
		}
	} else {
		int feature_index = j * INTERIOR_GROUPING_FACTOR * LEAF_GROUPING_FACTOR + l * LEAF_GROUPING_FACTOR;

		for (int l = (FEATURES / VARIABLES) * j; l < (FEATURES / VARIABLES) * (j + 1); l++) {
			if (Xi[l] == 1) {
				(*tm).ta_state[i][j][k][l] += ((*tm).ta_state[i][j][k][l] < NUMBER_OF_STATES*2) && (BOOST_TRUE_POSITIVE_FEEDBACK == 1 || 1.0*rand()/RAND_MAX <= (s-1)/s);
			} else {				
				(*tm).ta_state[i][j][k][l] -= ((*tm).ta_state[i][j][k][l] > 1) && (1.0*rand()/RAND_MAX <= 1.0/s);
			}

			if (Xi[l + FEATURES] == 1) {
				(*tm).ta_state[i][j][k][l + FEATURES] += ((*tm).ta_state[i][j][k][l + FEATURES] < NUMBER_OF_STATES*2) && (BOOST_TRUE_POSITIVE_FEEDBACK == 1 || 1.0*rand()/RAND_MAX <= (s-1)/s);
			} else {				
				(*tm).ta_state[i][j][k][l + FEATURES] -= ((*tm).ta_state[i][j][k][l + FEATURES] > 1) && (1.0*rand()/RAND_MAX <= 1.0/s);
			}
		}
	}
}


/**************************************************/
/*** Type II Feedback (Combats False Positives) ***/
/**************************************************/

static inline void type_ii_feedback(struct TsetlinMachine *tm, int Xi[], int i, int j, int k) {
	int action_include;

	if ((*tm).clause_output[i] > 0 && (*tm).clause_component_output[i][j][k] == 1) {
		for (int l = (FEATURES / VARIABLES) * j; l < (FEATURES / VARIABLES) * (j + 1); l++) { 
			action_include = action((*tm).ta_state[i][j][k][l]);
			(*tm).ta_state[i][j][k][l] += (action_include == 0 && (*tm).ta_state[i][j][k][l] < NUMBER_OF_STATES*2) && (Xi[l] == 0);

			action_include = action((*tm).ta_state[i][j][k][l + FEATURES]);
			(*tm).ta_state[i][j][k][l + FEATURES] += (action_include == 0 && (*tm).ta_state[i][j][k][l + FEATURES] < NUMBER_OF_STATES*2) && (Xi[l + FEATURES] == 0);
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

	for (int i = 0; i < CLAUSES; i++) {
		int sign = 1 - 2 * (i & 1);

		for (int j = 0; j < VARIABLES; j++) {
			for (int k = 0; k < CLAUSE_COMPONENTS; k++) {
				(*tm).feedback_to_components[i][j][k] = sign*(2*target-1)*(1.0*rand()/RAND_MAX <= (1.0/(THRESHOLD*2))*(THRESHOLD + (1 - 2*target)*class_sum));
			}
		}
	}
	
	/*********************************/
	/*** Train Individual Automata ***/
	/*********************************/

	for (int i = 0; i < CLAUSES; i++) {
		for (int j = 0; j < VARIABLES; j++) {
			int k = rand() % CLAUSE_COMPONENTS;
			if ((*tm).feedback_to_components[i][j][k] > 0) {
				type_i_feedback(tm, Xi, i, j, k, s);
			} else if ((*tm).feedback_to_components[i][j][k] < 0) {
				type_ii_feedback(tm, Xi, i, j, k);
			}
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



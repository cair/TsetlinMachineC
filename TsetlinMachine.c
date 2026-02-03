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
		for (int j = 0; j < ROOT_FACTORS; j++) {
			for (int k = 0; k < INTERIOR_ALTERNATIVES; k++) {	
				for (int l = 0; l < INTERIOR_FACTORS; l++) {
					for (int m = 0; m < LEAF_ALTERNATIVES; m++) {
						for (int n = 0; n < LEAF_FACTORS; n++) {

							if (1.0 * rand()/RAND_MAX <= 0.5) {
								(*tm).ta_state[i][j][k][l][m][n] = NUMBER_OF_STATES;
								(*tm).ta_state[i][j][k][l][m][n + LEAF_FACTORS] = NUMBER_OF_STATES + 1;
							} else {
								(*tm).ta_state[i][j][k][l][m][n] = NUMBER_OF_STATES + 1;
								(*tm).ta_state[i][j][k][l][m][n + LEAF_FACTORS] = NUMBER_OF_STATES;
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

		for (int j = 0; j < ROOT_FACTORS; j++) {
			// Evaluates interior subtrees, adding up votes from each
			(*tm).interior_vote_sums[i][j] = 0;

			for (int k = 0; k < INTERIOR_ALTERNATIVES; k++) {
				// Multiplies vote sums from multiple leaf groups (AND-combination)

				(*tm).interior_vote_products[i][j][k] = 1; // Stores how many class votes you get per interior alternative (product of leaf vote sums)

				for (int l = 0; l < INTERIOR_FACTORS; l++) {
					// Evaluates leaf alternatives (clause components), adding up the votes
					(*tm).leaf_vote_sum[i][j][k][l] = 0; // Stores how many class votes you get per feature group (vote summation over leaf alternatives)

					for (int m = 0; m < LEAF_ALTERNATIVES; m++) {
						// Evaluates clause component on its feature group

						(*tm).clause_component_output[i][j][k][l][m] = 1;
						for (int n = 0; n < LEAF_FACTORS; n++) {
							int feature = j * INTERIOR_FACTORS * LEAF_FACTORS + l * LEAF_FACTORS + n;

							action_include = action((*tm).ta_state[i][j][k][l][m][n]);
							if ((action_include == 1 && Xi[feature] == 0)) {
								(*tm).clause_component_output[i][j][k][l][m] = 0;
								break;
							}

							action_include = action((*tm).ta_state[i][j][k][l][m][n + LEAF_FACTORS]);
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
int tm_get_state(struct TsetlinMachine *tm, int clause, int root_factor, int interior_alternative, int interior_factor, int leaf_alternative, int leaf_factor)
{
	return (*tm).ta_state[clause][root_factor][interior_alternative][interior_factor][leaf_alternative][leaf_factor];
}

/*************************************************/
/*** Type I Feedback (Combats False Negatives) ***/
/*************************************************/

static inline void type_i_feedback(struct TsetlinMachine *tm, int Xi[], int i, int j, int k, int l, int m, float s)
{
	if ((*tm).clause_output[i] == 0 || (*tm).interior_vote_products[i][j][k] == 0 || (*tm).clause_component_output[i][j][k][l][m] == 0)	{
//	if ((*tm).clause_output[i] == 0 || (*tm).clause_component_output[i][j][k][l][m] == 0) {
		for (int n = 0; n < LEAF_FACTORS; n++) {
			(*tm).ta_state[i][j][k][l][m][n] -= ((*tm).ta_state[i][j][k][l][m][n] > 1) && (1.0*rand()/RAND_MAX <= 1.0/s);	

			(*tm).ta_state[i][j][k][l][m][n + LEAF_FACTORS] -= ((*tm).ta_state[i][j][k][l][m][n + LEAF_FACTORS] > 1) && (1.0*rand()/RAND_MAX <= 1.0/s);				
		}
	} else {
		int feature_index = j * INTERIOR_FACTORS * LEAF_FACTORS + l * LEAF_FACTORS;

		for (int n = 0; n < LEAF_FACTORS; n++) {
			if (Xi[feature_index + n] == 1) {
				(*tm).ta_state[i][j][k][l][m][n] += ((*tm).ta_state[i][j][k][l][m][n] < NUMBER_OF_STATES*2) && (BOOST_TRUE_POSITIVE_FEEDBACK == 1 || 1.0*rand()/RAND_MAX <= (s-1)/s);
			} else {				
				(*tm).ta_state[i][j][k][l][m][n] -= ((*tm).ta_state[i][j][k][l][m][n] > 1) && (1.0*rand()/RAND_MAX <= 1.0/s);
			}

			if (Xi[feature_index + n + FEATURES] == 1) {
				(*tm).ta_state[i][j][k][l][m][n + LEAF_FACTORS] += ((*tm).ta_state[i][j][k][l][m][n + LEAF_FACTORS] < NUMBER_OF_STATES*2) && (BOOST_TRUE_POSITIVE_FEEDBACK == 1 || 1.0*rand()/RAND_MAX <= (s-1)/s);
			} else {				
				(*tm).ta_state[i][j][k][l][m][n + LEAF_FACTORS] -= ((*tm).ta_state[i][j][k][l][m][n + LEAF_FACTORS] > 1) && (1.0*rand()/RAND_MAX <= 1.0/s);
			}
		}
	}
}


/**************************************************/
/*** Type II Feedback (Combats False Positives) ***/
/**************************************************/

static inline void type_ii_feedback(struct TsetlinMachine *tm, int Xi[], int i, int j, int k, int l, int m) {
	int action_include;

	if ((*tm).clause_output[i] > 0 && (*tm).interior_vote_products[i][j][k] > 0 && (*tm).clause_component_output[i][j][k][l][m] == 1) {
//	if ((*tm).clause_output[i] > 0 && (*tm).clause_component_output[i][j][k][l][m] == 1) {
		int feature_index = j * INTERIOR_FACTORS * LEAF_FACTORS + l * LEAF_FACTORS;

		for (int n = 0; n < LEAF_FACTORS; n++) {
			action_include = action((*tm).ta_state[i][j][k][l][m][n]);
			(*tm).ta_state[i][j][k][l][m][n] += (action_include == 0 && (*tm).ta_state[i][j][k][l][m][n] < NUMBER_OF_STATES*2) && (Xi[feature_index + n] == 0);

			action_include = action((*tm).ta_state[i][j][k][l][m][n + LEAF_FACTORS]);
			(*tm).ta_state[i][j][k][l][m][n + LEAF_FACTORS] += (action_include == 0 && (*tm).ta_state[i][j][k][l][m][n + LEAF_FACTORS] < NUMBER_OF_STATES*2) && (Xi[feature_index + n + FEATURES] == 0);
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

		for (int j = 0; j < ROOT_FACTORS; j++) {
			for (int k = 0; k < INTERIOR_ALTERNATIVES; k++) {
				for (int l = 0; l < INTERIOR_FACTORS; l++) {
					for (int m = 0; m < LEAF_ALTERNATIVES; m++) {
						(*tm).feedback_to_components[i][j][k][l][m] = sign*(2*target-1)*(1.0*rand()/RAND_MAX <= (1.0/(THRESHOLD*2))*(THRESHOLD + (1 - 2*target)*class_sum));
					}
				}
			}
		}
	}
	
	/*********************************/
	/*** Train Individual Automata ***/
	/*********************************/

	for (int i = 0; i < CLAUSES; i++) {
		for (int j = 0; j < ROOT_FACTORS; j++) {
			//int k = rand() % INTERIOR_ALTERNATIVES; // Pick a random subtree

			for (int k = 0; k < INTERIOR_ALTERNATIVES; k++) {
				for (int l = 0; l < INTERIOR_FACTORS; l++) {
					//int m = rand() % LEAF_ALTERNATIVES; // Pick a random clause component

					for (int m = 0; m < LEAF_ALTERNATIVES; m++) {
						if ((*tm).feedback_to_components[i][j][k][l][m] > 0) {
							type_i_feedback(tm, Xi, i, j, k, l, m, s);
						} else if ((*tm).feedback_to_components[i][j][k][l][m] < 0) {
							type_ii_feedback(tm, Xi, i, j, k, l, m);
						}
					}
				}
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



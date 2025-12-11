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
	for (int j = 0; j < CLAUSES; j++) {				
		for (int k = 0; k < FEATURES; k++) {
			if (1.0 * rand()/RAND_MAX <= 0.5) {
				(*tm).ta_state[j][k] = NUMBER_OF_STATES;
			} else {
				(*tm).ta_state[j][k] = NUMBER_OF_STATES + 1;
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
	// Calculate the output of each clause
	for (int j = 0; j < CLAUSES; j++) {
		(*tm).clause_output[j] = 1;

		int feature_index = 0; // Track the feature index
		int block_feature_index = 0; // Track the block index
		int component_index = 0; // Track the clause component index
		int action_index = 0;

		// Traverse the hierarchy left-right, bottom-up, level by level.
		for (int k = 0; k < LEVELS; k++) {
			block_feature_index += (*tm).blocks_per_level[k] * (*tm).features_per_block[k];

			// Traverse the blocks of the current level
			for (int l = 0; l < (*tm).blocks_per_level[k]; l++) {

				// Traverse the clause components of each feature block
				for (int m = 0; m < (*tm).components_per_block[k]; l++) {

					(*tm).component_output[component_index] = 1;
					
					for (int n = 0; n < (*tm).features_per_block[k]; n++) {
						int action_include = action((*tm).ta_state[j][action_index]);

						if (action_include && (!Xi[feature_index + n])) {
							(*tm).component_output[component_index] = 0;
							break;
						}

						action_index++; // Move on to next action
					}

					// Since each clause component is negated, the clause becomes false when the clause component is true.
					if ((*tm).component_output[component_index]) {
						(*tm).clause_output[j] = 0;
					}

					// Copy the component output into the next block feature vector (negated)...

					Xi[block_feature_index + // Index of the next feature block 
						+ m // Index of the feature inside that block (the clause component index)  
					] = !(*tm).component_output[component_index];

					component_index++; // Move on to 
				}

				// Skip to next block of features after all components have been evaluated on the present block
				feature_index += (*tm).features_per_block[k];
			}
		}
	}
}

/* Sum up the votes for each class (this is the multiclass version of the Tsetlin Machine) */
static inline int sum_up_class_votes(struct TsetlinMachine *tm)
{
	int class_sum = 0;

	for (int j = 0; j < CLAUSES / 2; j++) {
		class_sum += (*tm).clause_output[j];
	}

	for (int j = CLAUSES / 2; j < CLAUSES; j++) {
		class_sum -= (*tm).clause_output[j];
	}
	
	class_sum = (class_sum > THRESHOLD) ? THRESHOLD : class_sum;
	class_sum = (class_sum < -THRESHOLD) ? -THRESHOLD : class_sum;

	return class_sum;
}

/* Get the state of a specific automaton, indexed by clause, feature, and automaton type (include/include negated). */
int tm_get_state(struct TsetlinMachine *tm, int clause, int feature)
{
	return (*tm).ta_state[clause][feature];
}

/*************************************************/
/*** Type I Feedback (Combats False Negatives) ***/
/*************************************************/

static inline void type_i_feedback(struct TsetlinMachine *tm, int Xi[], int clause, int component, int ta_index, int feature_index, int features_per_block, float s)
{
	// ta_index refers to the first ta of the current clause component, and then n below points to the ta within the component to be updated
	// feature_index refers to the first feature of the feature block, and n below points to the current feature inside the block

	if ((*tm).clause_component[component] == 0)	{
		for (int n = 0; n < features_per_block; n++) { 
			int action_include = action((*tm).ta_state[clause][ta_index + n]);

			(*tm).ta_state[clause][ta_index + n] -= ((*tm).ta_state[clause][ta_index + n] > 1) && (s <= 1.0 || (1.0*rand()/RAND_MAX <= 1.0/s));							
		}
	} else if ((*tm).clause_component[component] == 1) {					
		for (int n = 0; n < FEATURES; n++) {
			if (Xi[feature_index + n] == 1) {
				(*tm).ta_state[clause][ta_index + n] += ((*tm).ta_state[clause][ta_index + n] < NUMBER_OF_STATES*2) && (s >= 1.0 || (1.0*rand()/RAND_MAX <= s));
			} else if (Xi[feature_index + n] == 0) {				
				(*tm).ta_state[cklause][ta_index + n] -= ((*tm).ta_state[clause][ta_index + n] > 1) && (s <= 1.0 || (1.0*rand()/RAND_MAX <= 1.0/s));
			}
		}
	}
}

/**************************************************/
/*** Type II Feedback (Combats False Positives) ***/
/**************************************************/

static inline void type_ii_feedback(struct TsetlinMachine *tm, int Xi[], int clause, int component, int ta_index, int feature_index, int features_per_block) {
	// ta_index refers to the first ta of the current clause component, and then n below points to the ta within the component to be updated
	// feature_index refers to the first feature of the feature block, and n below points to the current feature inside the block

	if ((*tm).component_output[component] == 1) {
		for (int n = 0; n < features_per_block; n++) {
			int action_include = action((*tm).ta_state[clause][ta_index + n]);

			(*tm).ta_state[clause][ta_index + n] += (Xi[feature_index + n] == 0);
			// There is no need to check if the action is include, because then the component output would be false,
			// and we are now looking at an output that is true.
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

	// Calculate feedback to clause components
	for (int j = 0; j < COMPONENTS; j++) {
		(*tm).feedback_to_components[j] = (2*target-1)*(1 - 2 * (j & 1))*(1.0*rand()/RAND_MAX <= (1.0/(THRESHOLD*2))*(THRESHOLD + (1 - 2*target)*class_sum));
	}

	for (int j = 0; j < CLAUSES; j++) {
		int component_index = 0; // Track the clause component index

		// Traverse the hierarchy left-right, bottom-up, level by level.

		int component_polarity = 1 - 2*(LEVELS % 2);

		for (int k = 0; k < LEVELS; k++) {

			// Traverse the blocks of the current level
			for (int l = 0; l < (*tm).blocks_per_level[k]; l++) {

				// Traverse the clause components of each feature block
				for (int m = 0; m < (*tm).components_per_block[k]; l++) {
					(*tm).feedback_to_components[component_index] =
						(2*target-1) * // Negate the polarities for the non-target class 
						component_polarity * // Each clause component has its own polarity, decided by the hierarchy level
						(1 - 2 * (j >= (CLAUSES / 2))) * // The second half of the clauses have negative polariy
						(1.0*rand()/RAND_MAX <= (1.0/(THRESHOLD*2))*(THRESHOLD + (1 - 2*target)*class_sum)); // Each component is updated with the class sum-decided probability

					component_index++;
				}
			}

			component_polarity = -1 * component_polarity; // The component polarity switches, level by level in the hierarchy
		}
	}
	
	/*********************************/
	/*** Train Individual Automata ***/
	/*********************************/

	// Give feedback to each clause component separately

	for (int j = 0; j < CLAUSES; j++) {
		int feature_index = 0; // Track the feature index
		int block_feature_index = 0; // Track the block index
		int component_index = 0; // Track the clause component index
		int action_index = 0;

		// Traverse the hierarchy left-right, bottom-up, level by level.

		for (int k = 0; k < LEVELS; k++) {
			block_feature_index += (*tm).blocks_per_level[k] * (*tm).features_per_block[k];

			// Traverse the blocks of the current level
			for (int l = 0; l < (*tm).blocks_per_level[k]; l++) {

				// Traverse the clause components of each feature block
				for (int m = 0; m < (*tm).components_per_block[k]; l++) {
					// action_index refers to the first TA of the current component
					// feature_index refers to the first feature of the current block

					if ((*tm).feedback_to_clauses[j] > 0) {
						type_i_feedback(tm, Xi, j, component_index, ta_index, feature_index, (*tm).features_per_block[k], s);
					} else if ((*tm).feedback_to_clauses[j] < 0) {
						type_ii_feedback(tm, Xi, j, component_index, ta_index, feature_index, (*tm).features_per_block[k]);
					}

					action_index += (*tm).features_per_block[k]; // Move on to next clause component
					component_index++;
				}

				// Skip to next block of features after all components have been evaluated on the present block
				feature_index += (*tm).features_per_block[k];
			}
		}
	}

	for (int j = 0; j < CLAUSES; j++) {
		if ((*tm).feedback_to_clauses[j] > 0) {
			type_i_feedback(tm, Xi, j, s);
		} else if ((*tm).feedback_to_clauses[j] < 0) {
			type_ii_feedback(tm, Xi, j);
		}
	}

	positive_polarity = !positive_polarity;
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



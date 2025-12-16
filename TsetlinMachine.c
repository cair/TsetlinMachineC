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

	(*tm).blocks_per_level[0] = BLOCKS_LVL_1;
	(*tm).blocks_per_level[1] = BLOCKS_LVL_2;
	(*tm).blocks_per_level[2] = BLOCKS_LVL_3;
	
	(*tm).features_per_block[0] = FEATURES_PER_BLOCK_LVL_1;
	(*tm).features_per_block[1] = FEATURES_PER_BLOCK_LVL_2;
	(*tm).features_per_block[2] = FEATURES_PER_BLOCK_LVL_3;

	(*tm).components_per_block[0] = COMPONENTS_PER_BLOCK_LVL_1;
	(*tm).components_per_block[1] = COMPONENTS_PER_BLOCK_LVL_2;
	(*tm).components_per_block[2] = COMPONENTS_PER_BLOCK_LVL_3;

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

static inline void calculate_clause_output(struct TsetlinMachine *tm, int Xi[])
{
	printf("START CALCULATE_CLAUSE_OUTPUT\n");

	// Calculate the output of each clause
	for (int j = 0; j < CLAUSES; j++) {
		printf("CLAUSE %d\n", j);

		int feature_index = 0; // Track the feature index
		int action_index = 0; // Track the action index
		int component_index = 0; // Track the clause component index

		// Traverse the hierarchy left-right, bottom-up, level by level.

		int next_feature_index = (*tm).blocks_per_level[0] * (*tm).features_per_block[0]; // Tracks next feature to be assigned a value
		for (int k = 0; k < LEVELS; k++) {
			printf("LEVEL %d\n", k);

			// Traverse the blocks of the current level
			for (int l = 0; l < (*tm).blocks_per_level[k]; l++) {
				printf("BLOCK %d\n", l);

				// Traverse the clause components of each feature block
				for (int m = 0; m < (*tm).components_per_block[k]; m++) {
					printf("\tCOMPONENT %d\n", m);

					(*tm).component_output[component_index] = 1;
					
					for (int n = 0; n < (*tm).features_per_block[k]; n++) {
						int action_include = action((*tm).ta_state[j][action_index + n]);

						printf("\t\tAction: %d Include: %d Feature %d: %d\n", action_index + n, action_include, feature_index + n, Xi[feature_index + n]);

						if (action_include && (!Xi[feature_index + n])) {
							(*tm).component_output[component_index] = 0;
							break;
						}
					}

					printf("\t\tComponent Output %d\n", (*tm).component_output[component_index]);

					// Copy the component output into the next block feature vector (negated)...
					if (k < LEVELS - 1) {
						Xi[next_feature_index] = !(*tm).component_output[component_index];
						printf("\t\tNext feature %d = %d\n", next_feature_index, !(*tm).component_output[component_index]);
					} else {
						(*tm).clause_output[j] = !(*tm).component_output[component_index];
						printf("\t\tClause Output = %d\n", (*tm).clause_output[j]);
					}

					next_feature_index++;
					action_index += (*tm).features_per_block[k];
					component_index++; // Move on to next component
				}

				// Skip to next block of features after all components have been evaluated on the present block
				feature_index += (*tm).features_per_block[k];
			}
		}

	}

	printf("END CALCULATE_CLAUSE_OUTPUT\n");
}

/* Sum up the votes for each class (this is the multiclass version of the Tsetlin Machine) */
static inline int sum_up_class_votes(struct TsetlinMachine *tm)
{
	printf("START SUM_UP_CLASS_VOTES\n");

	int class_sum = 0;

	for (int j = 0; j < CLAUSES / 2; j++) {
		class_sum += (*tm).clause_output[j];
	}

	for (int j = CLAUSES / 2; j < CLAUSES; j++) {
		class_sum -= (*tm).clause_output[j];
	}
	
	class_sum = (class_sum > THRESHOLD) ? THRESHOLD : class_sum;
	class_sum = (class_sum < -THRESHOLD) ? -THRESHOLD : class_sum;

	printf("END SUM_UP_CLASS_VOTES\n");

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

	printf("START TYPE_I_FEEDBACK\n");

	if ((*tm).component_output[component] == 0) {
		// If clause is False, all positive polarity components are given Type Ib (they are all guided towards match through excluding features)
		for (int n = 0; n < features_per_block; n++) { 
			(*tm).ta_state[clause][ta_index + n] -= ((*tm).ta_state[clause][ta_index + n] > 1) && (s <= 1.0 || (1.0*rand()/RAND_MAX <= 1.0/s));							
		}
	} else if ((*tm).component_output[component] == 1) {					
		for (int n = 0; n < FEATURES; n++) {
			if (Xi[feature_index + n] == 1) {
				(*tm).ta_state[clause][ta_index + n] += ((*tm).ta_state[clause][ta_index + n] < NUMBER_OF_STATES*2) && (s >= 1.0 || (1.0*rand()/RAND_MAX <= s));
			} else if (Xi[feature_index + n] == 0) {				
				(*tm).ta_state[clause][ta_index + n] -= ((*tm).ta_state[clause][ta_index + n] > 1) && (s <= 1.0 || (1.0*rand()/RAND_MAX <= 1.0/s));
			}
		}
	}

	printf("END TYPE_I_FEEDBACK\n");
}

/**************************************************/
/*** Type II Feedback (Combats False Positives) ***/
/**************************************************/

static inline void type_ii_feedback(struct TsetlinMachine *tm, int Xi[], int clause, int component, int ta_index, int feature_index, int features_per_block) {
	// ta_index refers to the first ta of the current clause component, and then n below points to the ta within the component to be updated
	// feature_index refers to the first feature of the feature block, and n below points to the current feature inside the block

	printf("START TYPE_I_FEEDBACK\n");

	if ((*tm).component_output[component] == 1) {
		for (int n = 0; n < features_per_block; n++) {
			(*tm).ta_state[clause][ta_index + n] += (Xi[feature_index + n] == 0);
			// There is no need to check if the action is include, because then the component output would be false,
			// and we are now looking at an output that is true.
		}
	}

	printf("END TYPE_I_FEEDBACK\n");
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
	
	printf("START TM_UPDATE\n");

	calculate_clause_output(tm, Xi);

	/***************************/
	/*** Sum up Clause Votes ***/
	/***************************/

	int class_sum = sum_up_class_votes(tm);

	/*************************************/
	/*** Calculate Feedback to Clauses ***/
	/*************************************/

	printf("START SELECT POLARITY AND FEEDBACK FOR COMPONENTS\n");
	// Calculate feedback to clause components
	for (int j = 0; j < COMPONENTS; j++) {
		(*tm).feedback_to_components[j] = (2*target-1)*(1 - 2 * (j & 1))*(1.0*rand()/RAND_MAX <= (1.0/(THRESHOLD*2))*(THRESHOLD + (1 - 2*target)*class_sum));
	}
	printf("END SELECT POLARITY AND FEEDBACK FOR COMPONENTS\n");

	// Block feedback to component sub-hierarchies on true negative component.

	for (int j = 0; j < CLAUSES; j++) {
		int component_index = 0; // Track the clause component index

		// Traverse the hierarchy left-right, bottom-up, level by level.

		int component_polarity = 1 - 2*(LEVELS % 2);

		for (int k = 0; k < LEVELS; k++) {

			// Traverse the blocks of the current level
			for (int l = 0; l < (*tm).blocks_per_level[k]; l++) {

				// Traverse the clause components of each feature block
				for (int m = 0; m < (*tm).components_per_block[k]; m++) {
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
		int component_index = 0; // Track the clause component index
		int action_index = 0; // Tracks the index of the actions to be updated

		// Traverse the hierarchy left-right, bottom-up, level by level.

		for (int k = 0; k < LEVELS; k++) {
			// Traverse the blocks of the current level
			for (int l = 0; l < (*tm).blocks_per_level[k]; l++) {
				// Traverse the clause components of each feature block
				for (int m = 0; m < (*tm).components_per_block[k]; m++) {
					// action_index refers to the first TA of the current component
					// feature_index refers to the first feature of the current block

					if ((*tm).feedback_to_clauses[j] > 0) {
						type_i_feedback(tm, Xi, j, component_index, action_index, feature_index, (*tm).features_per_block[k], s);
					} else if ((*tm).feedback_to_clauses[j] < 0) {
						type_ii_feedback(tm, Xi, j, component_index, action_index, feature_index, (*tm).features_per_block[k]);
					}

					action_index += (*tm).features_per_block[k]; // Move on to next clause component
					component_index++;
				}

				// Skip to next block of features after all components have been evaluated on the present block
				feature_index += (*tm).features_per_block[k];
			}
		}
	}

	printf("END TM_UPDATE\n");
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



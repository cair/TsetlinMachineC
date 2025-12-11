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

This code implements the Hierarchical Tsetlin Machine

*/


#define LEVELS 3

#define VALUES 10
#define VARIABLES 2

// Each clause is divided into components, a certain number of components per level

#define COMPONENTS_LVL_1 4
#define COMPONENTS_LVL_2 2
#define COMPONENTS_LVL_3 1

// Each level in the hiearchy consists of a number of features

#define FEATURES_LVL_1 (VALUES * VARIABLES)
#define FEATURES_LVL_2 COMPONENTS_LVL_1
#define FEATURES_LVL_3 COMPONENTS_LVL_2

// The total number of features are collected in a flat feature vector

#define FEATURES (FEATURES_LVL_1 + FEATURES_LVL_2 + FEATURES_LVL_3)

// The features of each level is diveded into seperate blocks

#define BLOCKS_LVL_1 2
#define BLOCKS_LVL_2 1
#define	BLOCKS_LVL_3 1

// The features at each hierarchy level are divided uniformly across the blocks

#define FEATURES_PER_BLOCK_LVL_1 (FEATURES_LVL_1 / BLOCKS_LVL_1)
#define FEATURES_PER_BLOCK_LVL_2 (FEATURES_LVL_2 / BLOCKS_LVL_2)
#define FEATURES_PER_BLOCK_LVL_3 (FEATURES_LVL_3 / BLOCKS_LVL_3)

// The clause components are also divided evenly among the blocks, so that each clause component gets its own subset of the features

#define COMPONENTS_PER_BLOCK_LVL_1 (COMPONENTS_LVL_1 / BLOCKS_LVL_1)
#define COMPONENTS_PER_BLOCK_LVL_2 (COMPONENTS_LVL_2 / BLOCKS_LVL_2)
#define COMPONENTS_PER_BLOCK_LVL_3 (COMPONENTS_LVL_3 / BLOCKS_LVL_3)

// The total number of clause components 

#define COMPONENTS (COMPONENTS_LVL_1 + COMPONENTS_LVL_2 + COMPONENTS_LVL_3)

// Standard Tsetlin machine parameters

#define THRESHOLD 15
#define CLAUSES 10
#define NUMBER_OF_STATES 100

// Strategy
// Evaluate clause components from left to right, the last one becomes the clause output, use moving window...

struct TsetlinMachine { 
	int blocks_per_level[] = {BLOCKS_LVL_1, BLOCKS_LVL_2, BLOCKS_LVL_3};
	int features_per_block[] = {FEATURES_PER_BLOCK_LVL_1, FEATURES_PER_BLOCK_LVL_2, FEATURES_PER_BLOCK_LVL_3};
	int components_per_block[] = {COMPONENTS_PER_BLOCK_LVL_1, COMPONENTS_PER_BLOCK_LVL_2, COMPONENTS_PER_BLOCK_LVL_3};

	int ta_state[CLAUSES][FEATURES];

	int component_output[COMPONENTS];

	int feedback_to_components[COMPONENTS];

	int clause_output[CLAUSES];

	int feedback_to_clauses[CLAUSES];
};

struct TsetlinMachine *CreateTsetlinMachine();

void tm_initialize(struct TsetlinMachine *tm);

void tm_update(struct TsetlinMachine *tm, int Xi[], int target, float s);

int tm_score(struct TsetlinMachine *tm, int Xi[]);

int tm_get_state(struct TsetlinMachine *tm, int clause, int feature, int automaton_type);


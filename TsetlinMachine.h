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

#define THRESHOLD 3000
#define LEAF_FACTORS 3
#define INTERIOR_FACTORS 2
#define ROOT_FACTORS 2
#define LEAF_ALTERNATIVES 10
#define INTERIOR_ALTERNATIVES 2
#define CLAUSES 16

#define LITERALS_PER_GROUP (LEAF_FACTORS * 2)

#define FEATURES (ROOT_FACTORS * INTERIOR_FACTORS * LEAF_FACTORS)
#define LITERALS (FEATURES*2)

#define NUMBER_OF_STATES 100
#define BOOST_TRUE_POSITIVE_FEEDBACK 0

#define PREDICT 1
#define UPDATE 0

struct TsetlinMachine {
	int ta_state[CLAUSES][ROOT_FACTORS][INTERIOR_ALTERNATIVES][INTERIOR_FACTORS][LEAF_ALTERNATIVES][LITERALS_PER_GROUP]; // The clause components, unique per clause (later we can introduce sharing)
	int leaf_vote_sum[CLAUSES][ROOT_FACTORS][INTERIOR_ALTERNATIVES][INTERIOR_FACTORS]; // Stores how many class votes you get per feature group (vote summation over leaf alternatives)
	int interior_vote_products[CLAUSES][ROOT_FACTORS][INTERIOR_ALTERNATIVES]; // Stores how many class votes you get per interior alternative (product of leaf vote sums)
	int interior_vote_sums[CLAUSES][ROOT_FACTORS]; // Stores how many class votes you get per interior alternative (product of leaf vote sums)

	int clause_output[CLAUSES]; // Stores product of interior alternative vote sums
	int clause_component_output[CLAUSES][ROOT_FACTORS][INTERIOR_ALTERNATIVES][INTERIOR_FACTORS][LEAF_ALTERNATIVES];

	int feedback_to_components[CLAUSES][ROOT_FACTORS][INTERIOR_ALTERNATIVES][INTERIOR_FACTORS][LEAF_ALTERNATIVES];

	int feedback_to_clauses[CLAUSES]; // Decides which clause to update, but the clause sum is calculated after roll out.
};
 
struct TsetlinMachine *CreateTsetlinMachine();

void tm_initialize(struct TsetlinMachine *tm);

void tm_update(struct TsetlinMachine *tm, int Xi[], int target, float s);

int tm_score(struct TsetlinMachine *tm, int Xi[]);

int tm_get_state(struct TsetlinMachine *tm, int clause, int root_factor, int interior_alternative, int interior_factor, int leaf_alternative, int leaf_factor);

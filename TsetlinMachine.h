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

#define THRESHOLD 15
#define FEATURES 12
#define LITERALS (FEATURES*2)
#define CLAUSES 2
#define CLAUSE_COMPONENTS 5
#define NUMBER_OF_STATES 100
#define BOOST_TRUE_POSITIVE_FEEDBACK 0

#define PREDICT 1
#define UPDATE 0

struct TsetlinMachine { 
	int ta_state[CLAUSES][CLAUSE_COMPONENTS][LITERALS]; // The clause components, unique per clause (later we can introduce sharing)
	int ta_state_layer_two[CLAUSES][2]; // Three automata, one for x_3, one for \lnot x_3, and one for c_1^* OR c_2^*...

	int clause_component_output[CLAUSES][CLAUSE_COMPONENTS]; // Here, we count how many times the rolled out clauses are True.

	int clause_output[CLAUSES]; // Here, we count how many times the rolled out clauses are True.

	int feedback_to_components[CLAUSES][CLAUSE_COMPONENTS];

	int feedback_to_clauses[CLAUSES]; // Decides which clause to update, but the clause sum is calculated after roll out.
};

struct TsetlinMachine *CreateTsetlinMachine();

void tm_initialize(struct TsetlinMachine *tm);

void tm_update(struct TsetlinMachine *tm, int Xi[], int target, float s);

int tm_score(struct TsetlinMachine *tm, int Xi[]);

int tm_get_state(struct TsetlinMachine *tm, int clause, int clause_component, int feature);

import numpy as np


noise = 0.05
number_of_values = 2
number_of_examples = 10000

X_train = np.zeros((number_of_examples, number_of_values*2), dtype=np.uint32)
Y_train = np.zeros(number_of_examples, dtype=np.uint32)
for i in range(number_of_examples):
	value_variable_1 = np.random.randint(number_of_values)
	value_variable_2 = np.random.randint(number_of_values)

	X_train[i, value_variable_1] = 1 
	X_train[i, number_of_values + value_variable_2] = 1
	X_train[i] = 1 - X_train[i]
	if (value_variable_1 % 2) != (value_variable_2 % 2):
		Y_train[i] = 1

Y_train = np.where(np.random.rand(number_of_examples) <= noise, 1-Y_train, Y_train) # Adds noise
np.savetxt("NoisyMultiValuedXORTrainingData.txt", np.append(X_train, Y_train.reshape((number_of_examples, 1)), axis=1), fmt='%d')

X_test = np.zeros((number_of_examples, number_of_values*2), dtype=np.uint32)
Y_test = np.zeros(number_of_examples, dtype=np.uint32)
for i in range(number_of_examples):
	value_variable_1 = np.random.randint(number_of_values)
	value_variable_2 = np.random.randint(number_of_values)

	X_test[i, value_variable_1] = 1
	X_test[i, number_of_values + value_variable_2] = 1
	X_test[i] = 1 - X_test[i]

	if (value_variable_1 % 2) != (value_variable_2 % 2):
		Y_test[i] = 1

np.savetxt("NoisyMultiValuedXORTestingData.txt", np.append(X_test, Y_test.reshape((number_of_examples, 1)), axis=1), fmt='%d')

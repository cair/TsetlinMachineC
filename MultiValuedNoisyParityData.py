import numpy as np


noise = 0.05
number_of_values = 5
number_of_variables = 4
number_of_examples = 20000

X_train = np.zeros((number_of_examples, number_of_values*number_of_variables), dtype=np.uint32)
Y_train = np.zeros(number_of_examples, dtype=np.uint32)
for i in range(number_of_examples):
	true_count = 0
	for j in range(number_of_variables):
		value_variable = np.random.randint(number_of_values-1)
		X_train[i, number_of_values * j + value_variable] = 1
		X_train[i, number_of_values * j + number_of_values - 1] = 1
		true_count += (value_variable % 2)

	X_train[i] = 1 - X_train[i]
	Y_train[i] = true_count % 2		

Y_train = np.where(np.random.rand(number_of_examples) <= noise, 1-Y_train, Y_train) # Adds noise
np.savetxt("NoisyMultiValuedXORTrainingData.txt", np.append(X_train, Y_train.reshape((number_of_examples, 1)), axis=1), fmt='%d')

X_test = np.zeros((number_of_examples, number_of_values*number_of_variables), dtype=np.uint32)
Y_test = np.zeros(number_of_examples, dtype=np.uint32)
for i in range(number_of_examples):
	true_count = 0
	for j in range(number_of_variables):
		value_variable = np.random.randint(number_of_values-1)
		X_test[i, number_of_values * j + value_variable] = 1
		X_test[i, number_of_values * j + number_of_values - 1] = 1
		true_count += (value_variable % 2)
	X_test[i] = 1 - X_test[i]
	Y_test[i] = true_count % 2

np.savetxt("NoisyMultiValuedXORTestingData.txt", np.append(X_test, Y_test.reshape((number_of_examples, 1)), axis=1), fmt='%d')

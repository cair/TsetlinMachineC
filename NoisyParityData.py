import numpy as np


noise = 0.4
number_of_variables = 12
number_of_examples = 20000
parity_values = 3

#X_train = np.zeros((number_of_examples, number_of_variables), dtype=np.uint32)

X_train = np.random.randint(2, size=(number_of_examples, number_of_variables), dtype=np.uint32)
Y_train = np.zeros(number_of_examples, dtype=np.uint32)

for i in range(number_of_examples):
	true_count = 0
	for j in range(number_of_variables):
		X_train[i, j] = np.random.randint(2)
		true_count += X_train[i, j]
	#Y_train[i] = true_count % 2	
	Y_train[i] = (np.sum(X_train[i, 0:parity_values])) % 2

Y_train = np.where(np.random.rand(number_of_examples) <= noise, 1-Y_train, Y_train) # Adds noise
np.savetxt("NoisyParityTrainingData.txt", np.append(X_train, Y_train.reshape((number_of_examples, 1)), axis=1), fmt='%d')

#X_test = np.zeros((number_of_examples, number_of_variables), dtype=np.uint32)
X_test = np.random.randint(2, size=(number_of_examples, number_of_variables), dtype=np.uint32)
Y_test = np.zeros(number_of_examples, dtype=np.uint32)
for i in range(number_of_examples):
	true_count = 0
	for j in range(number_of_variables):
		X_test[i, j] = np.random.randint(2)
		true_count += X_test[i, j]
	#Y_test[i] = true_count % 2	
	Y_test[i] = (np.sum(X_test[i, 0:parity_values])) % 2	

np.savetxt("NoisyParityTestingData.txt", np.append(X_test, Y_test.reshape((number_of_examples, 1)), axis=1), fmt='%d')

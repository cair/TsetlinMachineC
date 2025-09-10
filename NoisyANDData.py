import numpy as np


noise = 0.01
number_of_features = 2
number_of_examples = 5000

X_train = np.random.randint(0, 2, size=(number_of_examples, number_of_features), dtype=np.uint32)
Y_train = np.empty(number_of_examples, dtype=np.uint32)
for i in range(number_of_examples):
	Y_train[i] = np.random.random() <= 0.5
	X_train[i,0] = Y_train[i]
	X_train[i,1] = 1 - Y_train[i]
Y_train = np.where(np.random.rand(number_of_examples) <= noise, 1-Y_train, Y_train) # Adds noise
np.savetxt("ANDsOfORsTrainingData.txt", np.append(X_train, Y_train.reshape((number_of_examples, 1)), axis=1), fmt='%d')

X_test = np.random.randint(0, 2, size=(number_of_examples, number_of_features), dtype=np.uint32)
Y_test = np.empty((number_of_examples), dtype=np.uint32)
for i in range(number_of_examples):
	Y_test[i] = np.random.random() <= 0.5
	X_test[i,0] = Y_test[i]
	X_test[i,1] = 1 - Y_test[i]
np.savetxt("ANDsOfORsTestingData.txt", np.append(X_test, Y_test.reshape((number_of_examples, 1)), axis=1), fmt='%d')

import numpy as np


noise = 0.01
number_of_features = 6
number_of_examples = 20000

X_train = np.zeros((number_of_examples, number_of_features), dtype=np.uint32)
Y_train = np.empty(number_of_examples, dtype=np.uint32)
for i in range(number_of_examples):
	k = np.random.randint(number_of_features)
	X_train[i, k] = 1
	Y_train[i] = k >= (number_of_features // 2)
Y_train = np.where(np.random.rand(number_of_examples) <= noise, 1-Y_train, Y_train) # Adds noise
np.savetxt("ANDsOfORsTrainingData.txt", np.append(X_train, Y_train.reshape((number_of_examples, 1)), axis=1), fmt='%d')


X_test = np.zeros((number_of_examples, number_of_features), dtype=np.uint32)
Y_test = np.empty(number_of_examples, dtype=np.uint32)
for i in range(number_of_examples):
	k = np.random.randint(number_of_features)
	X_test[i, k] = 1
	Y_test[i] = k >= (number_of_features // 2)
np.savetxt("ANDsOfORsTestingData.txt", np.append(X_test, Y_test.reshape((number_of_examples, 1)), axis=1), fmt='%d')

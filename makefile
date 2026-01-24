NoisyParityDemo: MultiClassTsetlinMachine.c MultiClassTsetlinMachine.h TsetlinMachine.c TsetlinMachine.h NoisyParityDemo.c
	gcc -Wall -O3 -ffast-math -o NoisyParityDemo NoisyParityDemo.c MultiClassTsetlinMachine.c TsetlinMachine.c 

clean:
	rm *.o NoisyParityDemo

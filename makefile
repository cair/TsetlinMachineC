NoisyXORDemo: MultiClassTsetlinMachine.c MultiClassTsetlinMachine.h TsetlinMachine.c TsetlinMachine.h NoisyMultiValuedXORDemo.c
	gcc -Wall -O3 -ffast-math -o NoisyXORDemo NoisyXORDemo.c MultiClassTsetlinMachine.c TsetlinMachine.c 

clean:
	rm *.o NoisyMultiValuedXORDemo

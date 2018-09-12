NoisyXORDemo: MultiClassTsetlinMachine.c MultiClassTsetlinMachine.h TsetlinMachine.c TsetlinMachine.h NoisyXORDemo.c
	gcc -Wall -O3 -ffast-math -o NoisyXORDemo NoisyXORDemo.c MultiClassTsetlinMachine.c TsetlinMachine.c 

clean:
	rm *.o NoisyXORDemo

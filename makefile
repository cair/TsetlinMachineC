NoisyXORDemo: MultiClassTsetlinMachine.c MultiClassTsetlinMachine.h TsetlinMachineVariant.c TsetlinMachineVariant.h NoisyXORDemo.c
	gcc -Wall -O3 -ffast-math -o NoisyXORDemo NoisyXORDemo.c MultiClassTsetlinMachine.c TsetlinMachineVariant.c 

clean:
	rm *.o NoisyXORDemo

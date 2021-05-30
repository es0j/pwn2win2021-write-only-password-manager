all:
	gcc -o ../healthcheck/attacker_short attacker.c -no-pie  -O1 -DLAST_INDEX=22 -DBEGIN_INDEX=16
	gcc -o ../healthcheck/attacker attacker.c -no-pie  -O1 
	gcc -o ../healthcheck/testeEficiency testeEficiency.c -no-pie -O0 


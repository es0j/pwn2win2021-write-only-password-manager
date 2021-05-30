#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>


#define TOTAL_RUNS 100
int limit=10;

#include "utils.h"

void unused(){
    asm __volatile__(".rept 0x50;nop;.endr");
}


void checkResults(char *flushAux,int opt){

    unsigned long evictedTime=0;
    unsigned long accessedTime=0;
    int longest=0;
    unsigned long time;

    for (int i=0;i<TOTAL_RUNS;i++){
        if (i%2==0){
            time = probe_no_flush((void *)&limit);
            accessedTime+=time;
            //printf("hit with %lu\n",time);
        }
        else{
            if (opt==0){
                flushLimit_evict(flushAux,((unsigned long)&limit & 0xfff));
            }
            else if (opt==1){
                flush((char *)&limit);
            }
            time = probe_no_flush((void *)&limit);
            evictedTime+=time;
            //printf("miss with %lu\n",time);
            
        }
        asm __volatile__("mfence;lfence");
    }

    printf("acessed Avg = %f \n",(float)accessedTime*2/(float)TOTAL_RUNS);
    printf("evictedTime Avg = %f \n",(float)evictedTime*2/(float)TOTAL_RUNS);
    printf("longesttime = %i\n",longest);


}



int main(){
    
    char *flushAux;
    createDummyFile();
    flushAux= askmemFname((void *)0x6000000,"/tmp/dummy",PROT_READ|PROT_WRITE);
    if(flushAux<0){
        perror("mmap");
    }
	for(int i=0;i<768*2;i++){
		flushAux[4096*i]=i;
	}
    printf("limit = %p\n",&limit);

    printf("method 0 - evict:\n");
    checkResults(flushAux,0);
    printf("method 1 - clflush:\n");

    checkResults(flushAux,1);

    printf("method 2- none\n");

    checkResults(flushAux,2);


}

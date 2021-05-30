
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#define TOTAL_TESTS 50
#define SET_AMMOUNT 768*2
#define TAG_OFFSET_REMAIN 9
#define CACHE_STEP 30
#define CACHE_WAYS 20

#define DUMMY_PAGE_AMMOUNT 768*2

char globalVar[4096]={1,4,7,8,5,9,1};


void createDummyFile(){
	char buf[4096];
	FILE *dummy = fopen("/tmp/dummy","w");
	if (!dummy){
		perror("open file dummy");
	}
	for (int i=0;i<DUMMY_PAGE_AMMOUNT*2;i++){
		buf[0]=(char)i;
		fwrite(buf,4096,1,dummy);
	}
	fclose(dummy);
}


void flush(char *adrs) 
{
	volatile unsigned long time;
	asm __volatile__ (
	" clflush 0(%0) \n"
	" mfence \n"
	" lfence \n"

 : 
 : "c" (adrs)
 : );
}


static inline void flushLimit_evict(char *probe,unsigned long offset){
	volatile char tmp;
	for(int i=0;i<SET_AMMOUNT;i++){
		tmp = *(probe+(4096*i+offset));
	}
	//asm __volatile__("mfence;lfence");
}

//not work
static inline void flushLimit_evict_improved(char *probe,unsigned long offset){
	volatile char tmp;
	static unsigned setVar=0;
	for(int i=0;i<CACHE_WAYS;i++){
		for(int j=0;j<CACHE_STEP;j++){
			setVar=(setVar+1)%TAG_OFFSET_REMAIN;
			tmp = *(probe+(i<<(12+TAG_OFFSET_REMAIN)) + (setVar<<12) +offset);
		}
	}
	//asm __volatile__("mfence;lfence");
}



void *askmemFname(void *addr,char *fname,int protections){
	int fd;
	void *addr1;
	struct stat st;

	stat(fname, &st);

	fd=open(fname, O_RDWR,0755);

	if (fd<0){
		perror("open file");
	}
	addr1=mmap (addr, st.st_size , protections , MAP_SHARED  | MAP_FIXED   , fd,0);

	if( addr1==MAP_FAILED){
		perror("mmap");
	}
	if (addr1!=addr){
		perror("wrong value ");
	}
	return addr1;

}


unsigned long probe(char *adrs) 
{
	volatile unsigned long time;
	asm __volatile__ (
	" mfence \n"
	" lfence \n"
	" rdtsc \n"
	" lfence \n"
	" movl %%eax, %%esi \n"
	" movl (%1), %%eax \n"
	" lfence \n"
	" rdtsc \n"
	" subl %%esi, %%eax \n"
	" clflush 0(%1) \n"

 : "=a" (time)
 : "c" (adrs)
 : "%esi", "%edx");
 return time;
}


unsigned long probe_no_flush(char *adrs) 
{
	volatile unsigned long time;
	asm __volatile__ (
	" mfence \n"
	" lfence \n"
	" rdtsc \n"
	" lfence \n"
	" movl %%eax, %%esi \n"
	" movl (%1), %%eax \n"
	" lfence \n"
	" rdtsc \n"
	" subl %%esi, %%eax \n"


 : "=a" (time)
 : "c" (adrs)
 : "%esi", "%edx");
 return time;
}



int computeThreshold(){

	unsigned long t1,t2;
	unsigned long count=0;
	int recomended;
	double total_access,total_evict;
	volatile int tmp;
	total_access=0;
	total_evict=0;
	for(unsigned i=0;i<(TOTAL_TESTS *2);i++){
		if (i%2==0){
			tmp = globalVar[44];
		}
		t1=probe((void *)&globalVar[44]);
		count++;
		if (i%2==0){
			//printf("time w acess: %lu\n",t1);
			total_access+=(double)t1;

		}
		else{
			//printf("time no acess: %lu\n",t1);
			total_evict+=(double)t1;
		}
		//ensures that clflush will be called before entering in a new loop test
		asm __volatile__("mfence\nlfence\n");
	
	}
	total_access=total_access/TOTAL_TESTS;
	total_evict=total_evict/TOTAL_TESTS;
	printf("avg cached=%i\n",(int)total_access);
	printf("avg evicted=%i\n",(int)total_evict);
	recomended = (int)(total_evict*0.5+total_access*0.5);
	printf("recomended threshold =%i\n",recomended);
	return recomended;
}

#include <cstdlib>
#include <iostream>
#include <sys/time.h>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ITER   10
#define MAX_N 256*1024*1024 
#define STEPS_N 64*1024*1024 
#define MB    (1024*1024)

#define CACHE_LN_SZ 64

// LLC Parameters assumed
#define START_SIZE 1*MB
#define STOP_SIZE  16*MB


using namespace std;
char array[MAX_N];
unsigned int memrefs;
volatile void* global;


/////////////////////////////////////////////////////////
// Provides elapsed Time between t1 and t2 in milli sec
/////////////////////////////////////////////////////////

double elapsedTime(timeval t1, timeval t2){
  double delta;
  delta = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
  delta += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
  return delta; 
}

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

double DummyTest(void)
{    
  timeval t1, t2;
  int ii, iterid;

  // start timer
  gettimeofday(&t1, NULL);

  for(iterid=0;iterid<ITER;iterid++){
    for(ii=0; ii< MAX_N; ii++){
      array[ii] += rand();
    }
  }

  // stop timer
  gettimeofday(&t2, NULL);
 
  return elapsedTime(t1,t2);
}


/////////////////////////////////////////////////////////
// Change this, including input parameters
/////////////////////////////////////////////////////////


void fill_data(char *src, unsigned int size){

  unsigned int index = 0;
  if(!src){
    perror("wrong args");
    exit(-1);
  }
  
  for (index = 0; index < size; index++) {
	src[index] = 'a';
  }
}


void remove_frm_cache(unsigned int arg){
   
   unsigned int index = 0;
   char *dummy = (char *)malloc(arg);	
   for (index = 0; index < arg; index++) {
        dummy[index] = 'a';
  }
}

clock_t LineSizeTest(register unsigned int Stride)
{    
  double retval=0, prev=0;
  timeval t1,t2;
  char dummy; 
  unsigned int j=8; 
  int diff =0;
  int max =-999,maxidx=0,index = 0;

  for (j=4; j <= 1024; j=j<<1) {

	  Stride = j;
         // start timer
	  gettimeofday(&t1, NULL);

	  for (int times = 0; times < ITER; times++)
	  for (unsigned int i=0; i < STEPS_N; i= i++){
    	      array[i*Stride & STEPS_N-1] *=100;
	   }
	 // stop timer
	  gettimeofday(&t2, NULL);
	  retval = elapsedTime(t1,t2);

      if(index > 0){ 
	   	diff = retval - prev;
		if(max < diff) {
			max= diff;
		    maxidx =j;
		}
      }
	  cout << "CacheLineTest "<< retval <<" stride "<<j<< " diffs: "<<diff<<endl;
	  prev = retval;
	  index++;
   }
  
   cout << "CacheLine size " <<maxidx<<" B"<<endl;

  return retval; 
}




/////////////////////////////////////////////////////////
// Change this, including input parameters
/////////////////////////////////////////////////////////

//alternative methid using memcopy. is not consistent hence not using.
double CacheSizeTest(unsigned int arg, unsigned int repeat, register char *src, register char *dest)
{    
  double retval;
  timeval t1,t2;
  //void *src = array; // malloc(arg);
  //void *dest = malloc(arg);

  //fill_data((char *)src, arg);
  remove_frm_cache(arg);

  // start timer
  gettimeofday(&t1, NULL);

  while (repeat){
   memcpy(dest,src,arg);	
   repeat--;
  }

 // stop timer
  gettimeofday(&t2, NULL);

  retval = elapsedTime(t1,t2);

  return retval; 
}

unsigned long long LLCCacheSizeTest(unsigned int arg)
{    

  unsigned long long retval,prev;
  timeval t1,t2;
  int times = 0;
  char dummy;
  struct timespec tps, tpe;
  unsigned int Stride = 0;
  int diff =0;
  int max =-999,index = 0;
  int maxidx = 0;
  double change_per[10];

  //start timer
  //for (unsigned int j=START_SIZE; j<= STOP_SIZE; j=j+(512*1024)) {
  unsigned int j = 512*1024;

  while( j < STOP_SIZE) {

  //char *lclarry = new char[j];
  //for (unsigned int idx=0; idx<=j; idx++)
  //	lclarry[idx]= 1;

  Stride = j-1;
  unsigned int tmp,val;
  //clock_gettime(CLOCK_REALTIME, &tps);
  gettimeofday(&t1, NULL);

   for (times = 0; times < ITER; times++)
   for (unsigned int i = 0; i < STEPS_N; i++) {
	
       //First read into cache	
       //make sure to access diff cache line
       array[(i*CACHE_LN_SZ) & Stride] *= 100;
       //array[(i*cache_line) % Stride] *= 100;
       //then write to same cache line
   }
    //clock_gettime(CLOCK_REALTIME, &tpe);
    //retval = ((tpe.tv_sec-tps.tv_sec)*1000000000  + tpe.tv_nsec-tps.tv_nsec)/1000;
    gettimeofday(&t2, NULL);
    retval = elapsedTime(t1,t2);

    if(index > 0){ 
   	diff = retval - prev;
	if(max < diff) {
	    max= diff;
	    maxidx =j;
	}
      }
        cout << "LLCCacheSizeTest "<< retval <<" stride "<<j<< " diffs: "<<diff<<endl;
	prev = retval;

	if(index == 0) j = MB;
	//else j = j + (512*1024);
	else j = j + (MB);
        index++;
  }

   cout<<"Effective LLC Cache Size "<<maxidx/MB<<" MB"<<endl;
  // stop timer
  //gettimeofday(&t2, NULL);
  //retval = elapsedTime(t1,t2);
  return retval;
}

/////////////////////////////////////////////////////////
// Change this, including input parameters
/////////////////////////////////////////////////////////

struct node {
   int val;
   struct node *next;
   struct node *prev; 
   //pad to make each node a cache line sz;
   char dummy[CACHE_LN_SZ - 2 * sizeof(struct node*) - sizeof(int)]; 
};


struct node* delete_node( struct node *j) {

	if(!j) return NULL;
			
    j->prev->next = j->next;
    j->next->prev = j->prev;
    
    //ok I am free
    return j;
}

struct node* insert_node( struct node *i, struct node *j) {

	if(!j) return NULL;
			
    i->next->prev = j;
    j->next = i->next;
    j->prev = i;
    i->next = j;
    
    return i;
}

double MemoryTimingTest(void)
{    
  double retval;
  unsigned int j = 0;
  int sequential = 0;
  timeval t1,t2;
  struct timespec tps, tpe;
  int index = 0;
  unsigned long avg_memref[4],avg=0;


   for(unsigned int i=2*1024; i<16*1024*1024; i=i<<1) {

		unsigned long int ws = i;
  		struct node *arr = (struct node *)malloc(sizeof(struct node) * ws); 
		//first link all the node continuos
		for(unsigned int idx =1; idx < ws-1; idx++){
		     arr[idx].val = 1;
		     arr[idx].prev = &arr[idx-1];
		     arr[idx].next = &arr[idx+1];
		 }
		 //set the boundary values
		  arr[0].prev = &arr[ws-1];
		  arr[0].next = &arr[1];
		  arr[0].val =1;
		  arr[ws-1].prev= &arr[ws-2];
		  arr[ws-1].next = &arr[0];
		  arr[ws-1].val = 1;
		  //fill random
		  srand ( time(NULL) );

		  if(!sequential){
		   //Now start linking random nodes 
		   //delete old node links and then create
		   //new links
		   for(unsigned int idx =0; idx < ws; idx++) {
	       //generate a random id
    		   j = rand()% (ws-1);
		      //delete j;
		      delete_node(&arr[j]);
		      //insert between i and its next
		      insert_node(&arr[idx],&arr[j]);
		   }
		  }

		  //Now visit/iterate the linked list
		  //sum the values
		  register struct node *tmp = &arr[0];     
		  register unsigned int val =0;
		  register unsigned int idx =0;

		  clock_gettime(CLOCK_REALTIME, &tps);
		  //for(int it=0; it<ITER;it++)
		   gettimeofday(&t1, NULL);

		   for(idx =0; idx < STEPS_N; idx++){
     			tmp = tmp->next;
			    ++tmp->val;
		   }
		   clock_gettime(CLOCK_REALTIME, &tpe);
		   retval = (tpe.tv_sec-tps.tv_sec)*1000000000  + tpe.tv_nsec-tps.tv_nsec;
		   memrefs = STEPS_N;
	       avg_memref[index] = retval/memrefs;
	       cout << "MemTiming test "<<avg_memref[index]<<" wss: "<<i*64<<endl;
           index++;
   }

   cout<<endl;
   cout<<"memory access latency: "<<avg_memref[index-1]<<" ns"<<endl;
   //gettimeofday(&t2, NULL);
   //retval = elapsedTime(t1,t2);
   /*printf("%lu s, %lu ns, %lu\n", tpe.tv_sec-tps.tv_sec,
 				  tpe.tv_nsec-tps.tv_nsec,
				  retval);*/
  return retval; 
}



/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

int main(int argc, char *argv[]){

	int opt = 0;

	if(argc < 2) {	
	
		fprintf(stdout,"Enter 1 for Cache line size test, 2 for LLCCachesizetes,"
						"3 for Memtiming test, 4 for all \n");
		exit(0);
	}
  	
	 opt = atoi(argv[1]); 
	  
	  if(opt == 1 || opt == 4){
		  LineSizeTest(0);
		  sleep(1);
		  cout<<".........."<<endl;
		  cout<<""<<endl;
	 }

	if(opt == 2 || opt == 4){
	  	LLCCacheSizeTest(0);
	    sleep(1);
	    cout<<".........."<<endl;
        cout<<""<<endl;
	}

	if(opt == 3 || opt == 4){
	  MemoryTimingTest();
	}
	return 0;
}

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

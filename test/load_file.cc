
/*
 *  malloc-test
 *  cel - Thu Jan  7 15:49:16 EST 1999
 *
 *  Benchmark libc's malloc, and check how well it
 *  can handle malloc requests from multiple threads.
 *
 *  Syntax:
 *  malloc-test [ size [ iterations [ thread count ]]]
 *
 */

//#define ENABLE_MPI_RANKS
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <assert.h>
#include <dirent.h>
#include <errno.h>

#include <nv_map.h>
#include <c_io.h>
#include <util_func.h>

#define USECSPERSEC 1000000
#define pthread_attr_default NULL
#define MAX_THREADS 2
#define BASE_PROC_ID 1000

//#define _USE_BASIC_MMAP

unsigned int procid;

struct filestruct{
  char fname[256];
  unsigned int size;
};

struct filestruct *filelist=NULL;
static int g_idx=0;

static char* LoadDirFile(size_t *size, char *filename, char *read_dir) {

	size_t bytes = 0;
	FILE *fp = NULL;
	char filearr[512], *input;
	struct stat file_status;
	char *nvptr = NULL;
	size_t fsize = 0;

	//fprintf(stderr,"loading filename %s\n",filename);
	if(strlen(filename) < 4)
		return NULL;

	bzero(filearr, 512);
	strcpy(filearr,read_dir);
	strcat(filearr,"/");
	strcat(filearr,filename);

	fp = fopen(filearr, "r");
	if(fp == NULL)
		return NULL;

	if(fstat(fileno(fp), &file_status) != 0){
		perror("ERROR");
	}

	fsize = file_status.st_size;
	input = (char *)malloc(fsize);
	bytes = fread(input, 1,fsize, fp); 

	if(!bytes)
	{
		fprintf(stderr,"filename %s, bytes %u \n",filename, bytes);
		assert(bytes);
	}
	

	nvptr = (char *)(char *)nvalloc_((size_t)bytes, filename, 0);
	assert(nvptr);
	memcpy(nvptr, input, bytes);
	strcpy(filelist[g_idx].fname, filename);
	filelist[g_idx].size = bytes;
	g_idx++;

	nvcommitsz(filename, bytes);


#ifdef _USE_BASIC_MMAP
	 mmap_free(filename, nvptr);
#endif
	fclose(fp);
	return input;
}

static void load_dir(char *read_dir) {

	size_t datasize =0;
	DIR *mydir = NULL;
	struct dirent *entry = NULL;

	mydir = opendir(read_dir);
	assert(mydir);
	entry = readdir(mydir);

	while(entry)
	{
		char *input = NULL;
		if(strlen(entry->d_name) < 4)
			goto next;

		input = (char *)LoadDirFile(&datasize,  entry->d_name, read_dir);
		if(!input)
			goto next;
		else
			free(input);	

		next:
		entry = readdir(mydir);
	}
	nvcommitsz((char*)"filelist", g_idx*sizeof(int));
	//fprintf(stderr," load_dir commiting %u\n", g_idx*sizeof(int));
}

char * extract_filename(char *str)
{
	int     ch = '\\';
	size_t  len;
	char   *pdest;
	char   *inpfile = NULL;

	pdest = strrchr(str, ch);
	if(pdest == NULL )
	{
		printf( "Result:\t%c not found\n", ch );
		pdest = str;  // The whole name is a file in current path?
	}
	else
	{
		pdest++; // Skip the backslash itself.
	}
	len = strlen(pdest);
	inpfile = (char *)malloc(len+1);
	strncpy(inpfile, pdest, len+1);
	return inpfile;
}


int read_objects(){

	size_t nvmwriteoff=0, fsize=0;
	int count=0, idx=0;

	filelist = (struct filestruct *)nvread_len("filelist", 0, &nvmwriteoff);
	assert(filelist);
	assert(nvmwriteoff);

	count = nvmwriteoff/sizeof(int);

	for(idx=0; idx < count; idx++){
		void *nvptr=NULL;
	    nvptr = nvread_len(filelist[idx].fname,0,&fsize);
	    assert(nvptr);
	}
	return 0;
}



int main(int argc, char *argv[])
{
	unsigned i;
	unsigned thread_count = 1;
	pthread_t thread[MAX_THREADS];
	size_t size = 0,rdsz=0;
	struct stat st;
	FILE *fp1 = NULL;
	char *fname = NULL;
	void *dram_ptr = NULL;
	void* nvm_in_ptr = NULL;
#if 1
      nvinit_(600); 
#endif

	if(argc < 2){
		perror("Usage: arg0: appname, arg1: filepath"
				"Optional arg2: exclude filepath arg3: load entir dir, "
				"arg4: read/write mode\n");
		exit(0);
	}

	if(argc > 2 &&  argv[2]){
		fname= extract_filename(argv[1]);
	}else{
		fname = argv[1];
	}

	if(argc > 4 &&  argv[4]){

		/*read mode*/
		if(atoi(argv[4]) == 1){
			 read_objects();
			 return 0;
		}
	}

	filelist = (struct filestruct *)nvalloc_(10000*sizeof(struct filestruct),(char*)"filelist", 0);
	assert(filelist);

	if(atoi(argv[3]) == 1){
		load_dir((char *)argv[1]);
	}else {

		fp1 = fopen((char *)argv[1],"r");
		assert(fp1);
		stat((char *)argv[1], &st);
		size =  st.st_size;
		dram_ptr = malloc(size);
		assert(dram_ptr);

		nvm_in_ptr = nvalloc_(size, (char *)fname, 0);
		assert(nvm_in_ptr);

		rdsz = fread ( dram_ptr, 1, st.st_size, fp1);
		//fprintf(stdout,"rdsz %zu actual sz: %zu \n",rdsz, st.st_size);
		assert(rdsz);
		memcpy(nvm_in_ptr, dram_ptr, rdsz);
	}

	exit(0);
}


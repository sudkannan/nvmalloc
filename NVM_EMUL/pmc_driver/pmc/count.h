void cpuid();
long long rdpmc(int counter); 
int rdpmc32(int counter); 
int pmc_init(int *eventId, int count);

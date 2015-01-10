#include <iostream>
#include <time.h>
#include <cstdio>

clock_t whack_cache(const int sz)
{
    char* buf = new char[sz];

    clock_t start = clock();

    for (unsigned int i = 0; i < 64 * 1024 * 1024; i++)
        ++buf[(i * 64) % sz]; // writing in increments hopefully means we write to a new cache-line every time

    clock_t elapsed = clock() - start;

    delete [] buf;
    return elapsed;
}

int main()
{
    std::cout << "writing timing results to \"results.csv\"" << std::endl;

    FILE* f = fopen("results.csv", "w");
    if (!f)
        return 1;

    for (int sz = 1024; sz <= 16 * 1024 * 1024; sz = sz + 1024 * 1024)
    {
        fprintf(f, "%d, %lu\n", sz / 1024, whack_cache(sz));
        std::cout << ".";
        fflush(stdout);
    }

    fclose(f);

    std::cout << "done" << std::endl;
    return 0;
}

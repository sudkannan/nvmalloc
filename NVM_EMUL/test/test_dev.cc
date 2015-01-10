#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include "gtest/gtest.h"
#include "pmalloc.h"

int main(int argc, char** argv)
{
//    ::testing::InitGoogleTest(&argc, argv);
//    return RUN_ALL_TESTS();
    printf("PID: %d\n", getpid());
    printf("malloc: %p\n", malloc(8));
    printf("malloc: %p\n", malloc(8));
    printf("pmalloc: %p\n", pmalloc(8));
}


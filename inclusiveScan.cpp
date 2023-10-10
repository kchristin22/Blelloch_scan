#include <iostream>
#include <cstdlib>
#include <cmath>
#include <atomic>
#include <thread>
#include <chrono>
#include <vector>
#include <fstream>

#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench.h"

#define SIZE 8

void fnSum(uint16_t *array, size_t start, size_t end)
{
    array[end] += array[start];
}

void fnTree(uint16_t *array, size_t start)
{

    uint8_t nthreads = std::thread::hardware_concurrency();
    std::vector<std::thread> threadObj;
    threadObj.reserve(nthreads);
    size_t threadsToCreate, chunks = 1, chunkSize = start; // the length of the array range we're dealing with here is equal to start(2^i)
    size_t current_pos, previous_pos;                      // previous_pos is the position of the nearest item from the left
                                                           // that includes the sum of all its previous items and itself

    for (size_t l = 1; l < ((size_t)log2(start) + 1); l++)
    {
        threadsToCreate = (size_t)pow(2, l - 1);

        if (threadsToCreate >= nthreads)
        { // check if the number of threads needed surpasses the allocated no. of threads
            size_t chunks = threadsToCreate / nthreads;
            if ((threadsToCreate - (chunks * nthreads)) != 0)
            {                // check if the number of threads needed is not a multiple of the no. of hw threads
                chunks += 1; // add an extra chunk to account for the remaining threads
            }
            chunkSize = start / chunks;
        }
        for (size_t chunk = 0; chunk < chunks; chunk++)
        {
            for (size_t k = 1 + chunk * chunkSize; k < ((1 << l) - (chunkSize * (chunks - chunk - 1))); k += 2) // check report for an explanation on the limit of k
            {
                current_pos = start - 1 + k * (1 << ((size_t)log2(start) - l)); // check report for an explanation on the formula
                previous_pos = start - 1 + (k - 1) * (1 << ((size_t)log2(start) - l));
                threadObj.emplace_back(std::thread(fnSum, array, previous_pos, current_pos));
            }
            for (std::thread &t : threadObj)
            {
                if (t.joinable())
                {
                    t.join();
                }
            }
        }
    }
}

void reduce(uint16_t *array, size_t size)
{
    uint8_t nthreads = std::thread::hardware_concurrency();
    std::vector<std::thread> threadObj;
    threadObj.reserve(nthreads); // reserve space for the threads equal to the no. of hw threads
    size_t threadsToCreate, chunks = 1, chunkSize = size;

    for (size_t k = 0; k < (size_t)log2(size); k++)
    {
        threadsToCreate = size / ((size_t)pow(2, k + 1));

        if (threadsToCreate >= nthreads)
        { // check if the number of threads needed surpasses the allocated no. of threads
            size_t chunks = threadsToCreate / nthreads;
            if ((threadsToCreate - (chunks * nthreads)) != 0)
            {                // check if the number of threads needed is not a multiple of the no. of hw threads
                chunks += 1; // add an extra chunk to account for the remaining threads
            }
            chunkSize = size / chunks;
        }
        for (size_t threadGroup = 0; threadGroup < chunks; threadGroup++)
        {
            for (size_t i = ((1 << k) - 1 + threadGroup * chunkSize); i < (size - (1 << k) + 1 - (chunkSize * (chunks - threadGroup - 1))); i += (1 << (k + 1)))
            // subtracting (chunkSize*(chunks-threadGroup-1)) to account for the chunk's limit at each iteration
            // subtracting ((1 << k) - 1) to check if the end index is not out of bounds
            {
                threadObj.emplace_back(std::thread(fnSum, array, i, (i + (1 << k))));
            }
            for (std::thread &t : threadObj)
            { // join the parent thread before switching to the next chunk or to next level of the tree
                if (t.joinable())
                {
                    t.join();
                }
            }
        }
    }
}

void increase(uint16_t *array, size_t size)
{
    uint8_t nthreads = std::thread::hardware_concurrency();
    std::vector<std::thread> threadObj;
    threadObj.reserve(nthreads);

    for (size_t i = 1; i < (size_t)log2(size); i++) // start from 1 as the first element is already scanned
    {                                               // divide the array to ranges of [2^i, 2^(i+1))
        threadObj.emplace_back(std::thread(fnTree, array, (1 << i)));
    }

    for (std::thread &t : threadObj)
    { // join the parent thread before exiting
        if (t.joinable())
        {
            t.join();
        }
    }
}

void scan(uint16_t *array, size_t size)
{
    reduce(array, size);   // first stage of the algorithm, same with blelloch's
    increase(array, size); // second stage of the algorithm
}

// check for overflows

int main()
{

    uint16_t *array = (uint16_t *)malloc(SIZE * sizeof(uint16_t));

    // srand(time(NULL));

    // printf("The array in question is: ");
    for (size_t i = 0; i < SIZE; i++)
    {
        array[i] = rand() % 100;
        // printf("%d ", array[i]);
    }
    // printf("\n");
    // auto start = std::chrono::steady_clock::now();
    // scan(array, SIZE);
    // auto end = std::chrono::steady_clock::now();
    // auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    // printf("Time elapsed: %ld\n", duration.count());

    // printf("Final:");
    // for (size_t i=0; i<SIZE; i++){
    //     printf(" %d", array[i]);
    // }
    // printf("\n");

    std::fstream file("output_inclusive.json", std::ios::out);

    ankerl::nanobench::Bench()
        .minEpochIterations(100)
        .epochs(30)
        .run("scan", [&]
             { scan(array, SIZE); })
        .render(
            ankerl::nanobench::templates::pyperf(),
            file);

    free(array);
    return 0;
}

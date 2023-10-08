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

void fnSweep(uint16_t *array, size_t left, size_t right)
{
    uint16_t temp = array[left];
    array[left] = array[right];
    array[right] += temp;
}

void fnShift(uint16_t *array, uint16_t *auxArray, size_t left, size_t right)
{
    auxArray[left] = array[right];
}

void fnEquate(uint16_t *array, uint16_t *auxArray, size_t pos)
{
    array[pos] = auxArray[pos];
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

void downSweep(uint16_t *array, size_t size)
{
    array[(1 << (int)log2(size)) - 1] = 0; // initiate the pivot element of the sweep
    uint8_t nthreads = std::thread::hardware_concurrency();
    std::vector<std::thread> threadObj;
    threadObj.reserve(nthreads); // reserve space for the threads equal to the no. of hw threads
    size_t threadsToCreate, chunks, chunkSize;

    for (int k = ((int)log2(size) - 1); k >= 0; k--)
    {
        threadsToCreate = size / ((size_t)pow(2, k + 1));
        chunks = 1;
        chunkSize = size;

        if (threadsToCreate >= nthreads)
        { // check if the number of threads needed surpasses the allocated no. of threads
            size_t chunks = threadsToCreate / nthreads;
            if ((threadsToCreate - (chunks * nthreads)) != 0)
            {                // check if the number of threads needed is not a multiple of the no. of hw threads
                chunks += 1; // add an extra chunk to account for the remaining threads
            }
            chunkSize = size / chunks;
        }

        for (size_t chunk = 0; chunk < chunks; chunk++)
        {
            for (size_t i = ((1 << k) - 1 + chunk * chunkSize); i < (size - (1 << k) + 1 - (chunkSize * (chunks - chunk - 1))); i += (1 << (k + 1)))
            {                                                                           // subtracting (chunkSize*(chunks-chunk-1)) to account for the chunk's limit
                threadObj.emplace_back(std::thread(fnSweep, array, i, (i + (1 << k)))); // i + (1 << k) = 2^k  - 1 + 2^k = 2^(k+1) - 1
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

void leftShift(uint16_t *array, size_t size, uint16_t lastItem)
{
    uint8_t nthreads = std::thread::hardware_concurrency();
    uint8_t threadsToCreate = size / 2; // the number of threads required is constant for all levels of the tree
    std::vector<std::thread> threadObj;
    threadObj.reserve(nthreads);
    size_t chunks = 1, chunkSize = size;

    uint16_t *auxArray = (uint16_t *)malloc(size * sizeof(uint16_t)); // auxiliary array to be used to store the shifted array temporarily
                                                                      // so the shifting can be done in parallel

    if (threadsToCreate >= nthreads)
    { // check if the number of threads needed surpasses the allocated no. of threads
        size_t chunks = threadsToCreate / nthreads;
        if ((threadsToCreate - (chunks * nthreads)) != 0)
        {                // check if the number of threads needed is not a multiple of the no. of hw threads
            chunks += 1; // add an extra chunk to account for the remaining threads
        }
        chunkSize = size / chunks;
    }

    for (size_t chunk = 0; chunk < chunks; chunk++)
    {
        for (size_t i = chunk * chunkSize; i < (size - 1 - (chunkSize * (chunks - chunk - 1))); i += 2)
        {                                                                              // subtracting 1 from size since the increment is by 2
            threadObj.emplace_back(std::thread(fnShift, array, auxArray, i, (i + 1))); // even items
        }

        for (std::thread &t : threadObj)
        {
            if (t.joinable())
            {
                t.join();
            }
        }

        for (size_t i = 1 + chunk * chunkSize; i < (size - 1 - (chunkSize * (chunks - chunk - 1))); i += 2)
        {
            threadObj.emplace_back(std::thread(fnShift, array, auxArray, i, (i + 1))); // odd items
        }

        for (std::thread &t : threadObj)
        {
            if (t.joinable())
            {
                t.join();
            }
        }
    }

    // copying auxiliar array back to the original array
    threadsToCreate = size; // each item will be copied by a single thread
    chunks = 1;
    chunkSize = size;

    if (threadsToCreate >= nthreads)
    { // check if the number of threads needed surpasses the allocated no. of threads
        size_t chunks = threadsToCreate / nthreads;
        if ((threadsToCreate - (chunks * nthreads)) != 0)
        {                // check if the number of threads needed is not a multiple of the no. of hw threads
            chunks += 1; // add an extra chunk to account for the remaining threads
        }
        chunkSize = size / chunks;
    }

    for (size_t chunk = 0; chunk < chunks; chunk++)
    {
        for (size_t i = chunk * chunkSize; i < (size - 1 - (chunkSize * (chunks - chunk - 1))); i++)
        {
            threadObj.emplace_back(std::thread(fnEquate, array, auxArray, i));
        }

        for (std::thread &t : threadObj)
        {
            if (t.joinable())
            {
                t.join();
            }
        }
    }

    array[size - 1] = lastItem; // the last item that was overwritten by the pivot element of the sweep

    free(auxArray); // deallocate the memory used by the auxiliary array
}

void blellochScan(uint16_t *array, size_t size)
{
    reduce(array, size);                 // first stage of the algorithm
    uint16_t lastItem = array[size - 1]; // save the calculated value of the last item to re-assign it later
    downSweep(array, size);              // second stage of the algorithm
    leftShift(array, size, lastItem);    // use left shift to convert result to inclusive scan's output
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
    // blellochScan(array, SIZE);
    // auto end = std::chrono::steady_clock::now();
    // auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    // printf("Time elapsed: %ld\n", duration.count());

    // printf("Final:");
    // for (size_t i=0; i<SIZE; i++){
    //     printf(" %d", array[i]);
    // }
    // printf("\n");

    std::fstream file("output_blelloch.json", std::ios::out);

    ankerl::nanobench::Bench()
        .minEpochIterations(100)
        .epochs(30)
        .run("blelloch", [&]
             { blellochScan(array, SIZE); })
        .render(
            ankerl::nanobench::templates::pyperf(),
            file);

    free(array);
    return 0;
}

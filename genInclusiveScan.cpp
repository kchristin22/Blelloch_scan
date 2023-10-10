#include <iostream>
#include <cstdlib>
#include <cmath>
#include <atomic>
#include <thread>
#include <chrono>
#include <vector>

#define SIZE 14

void fnSum(uint16_t *array, size_t start, size_t end)
{
    array[end] += array[start];
}

void fnTree(uint16_t *array, size_t size, size_t start)
{

    uint8_t nthreads = std::thread::hardware_concurrency();
    std::vector<std::thread> threadObj;
    threadObj.reserve(nthreads);
    size_t threadsToCreate, chunks = 1, chunkSize = start; // the length of the array range we're dealing with here is equal to start(2^i)
    size_t current_pos, previous_pos, end;                 // previous_pos is the position of the nearest item from the left
                                                           // that includes the sum of all its previous items and itself

    size_t pow2size = 1 << (size_t)log2(size);

    if (size % 2 != 0)
    {           // check if the size of the array is an odd number
        size--; // perform the second stage for the range [0, size - 1]
    }

    if (size - pow2size > 0 && start == pow2size && (size - start) % 2 != 0)
    {                   // size != 2^k, we're in the last range of the array
                        // and the first index of the iteration is not odd
        end = 2 + size; // add 2 to the end index so the first index of the iteration will end up to the spot (size - start) / 2 + 1
    }
    else
        end = 2 * start; // end = 2^(i+1)

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
                current_pos = start - 1 + k * (end - start) / (1 << l); // check report for an explanation on the formula
                previous_pos = start - 1 + (k - 1) * (end - start) / (1 << l);
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

    if (size % 2 != 0)
    {
        array[size - 1] += array[size - 2];
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

    size_t log2size = (size_t)log2(size);

    size_t limit = (size - (1 << log2size)) > 0 ? log2size + 1 : log2size; // check if the size of the array is not a power of 2

    for (size_t i = 1; i < limit; i++) // start from 1 as the first element is already scanned
    {                                  // divide the array to ranges of [2^i, 2^(i+1))
        threadObj.emplace_back(std::thread(fnTree, array, size, (1 << i)));
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

    printf("The array in question is: ");
    for (size_t i = 0; i < SIZE; i++)
    {
        array[i] = rand() % 100;
        printf("%d ", array[i]);
    }
    printf("\n");
    auto start = std::chrono::steady_clock::now();
    scan(array, SIZE);
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    printf("Time elapsed: %ld\n", duration.count());

    printf("Final:");
    for (size_t i = 0; i < SIZE; i++)
    {
        printf(" %d", array[i]);
    }
    printf("\n");

    free(array);
    return 0;
}
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <atomic>
#include <thread>
#include <chrono>
#include <vector>

#define SIZE 8

void fnSum(uint16_t* array, size_t start, size_t end){
    array[end] += array[start];
}

void fnTree(uint16_t* array, size_t start){

    uint8_t nthreads = std::thread::hardware_concurrency();
    std::vector<std::thread> threadObj;
    threadObj.reserve(nthreads);
    size_t threadsToCreate, chunks = 1, chunkSize = start;
    size_t current_pos, previous_pos;

    for(size_t l= 1; l < ((size_t)log2(start) + 1); l++){
        threadsToCreate = (size_t)pow(2, l - 1);

        if(threadsToCreate >= nthreads){
            if (threadsToCreate % nthreads == 0){
                chunks = threadsToCreate / nthreads;
            }
            else{
                chunks = threadsToCreate / nthreads + 1;
            }
            chunkSize = start/chunks;
        }
        for (size_t chunk=0; chunk < chunks; chunk++){
            for (size_t k = 1 + chunk*chunkSize; k < ((1 << l) - (chunkSize*(chunks-chunk-1))); k+=2)
            {
                current_pos = start - 1 + k * (1 << ((size_t)log2(start) - l));
                previous_pos = start - 1 + (k - 1) * (1 << ((size_t)log2(start) - l));
                threadObj.emplace_back(std::thread(fnSum, array,previous_pos,current_pos));
            }
            for (std::thread& t : threadObj) {
                if (t.joinable()) {
                    t.join();
                }
            }
            
        }

        
    }
}


void reduce(uint16_t* array, size_t size){
    uint8_t nthreads = std::thread::hardware_concurrency();
    std::vector<std::thread> threadObj;
    threadObj.reserve(nthreads);
    size_t threadsToCreate, chunks = 1, chunkSize = size;

    for(size_t k=0; k < (size_t)log2(size); k++){
        threadsToCreate = size / ((size_t)pow(2, k + 1));

        if(threadsToCreate >= nthreads){
            if (threadsToCreate % nthreads == 0){
                chunks = threadsToCreate / nthreads;
            }
            else{
                chunks = threadsToCreate / nthreads + 1;
            }
            chunkSize = size/chunks;
        }
        for (size_t threadGroup=0; threadGroup < chunks; threadGroup++){
            for (size_t i = ((1 << k) -1 + threadGroup*chunkSize); i < (size - (1 << k) + 1 - (chunkSize*(chunks-threadGroup-1))); i+= (1 << (k+1)))
            {
                threadObj.emplace_back(std::thread(fnSum, array,i,(i + (1 << k))));
            }
            for (std::thread& t : threadObj) {
                if (t.joinable()) {
                    t.join();
                }
            }
            
        }

        
    }
}

void increase(uint16_t* array, size_t size){
    uint8_t nthreads = std::thread::hardware_concurrency();
    std::vector<std::thread> threadObj;
    threadObj.reserve(nthreads);

    for (size_t i = 1; i < (size_t)log2(size); i++){
        threadObj.emplace_back(std::thread(fnTree, array, (1 << i)));
    }

    for (std::thread& t : threadObj) {
        if (t.joinable()) {
            t.join();
        }
    }
 
}


void scan(uint16_t* array, size_t size){
    reduce(array, size);
    increase(array, size);
}


// check for overflows

int main(){

    uint16_t* array = (uint16_t*)malloc(SIZE*sizeof(uint16_t));

    // srand(time(NULL));

    printf("The array in question is: ");
    for (size_t i=0; i<SIZE; i++){
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
    for (size_t i=0; i<SIZE; i++){
        printf(" %d", array[i]);
    }
    printf("\n");

    free(array);
    return 0;
}

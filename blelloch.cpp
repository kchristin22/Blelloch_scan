#include <iostream>
#include <cstdlib>
#include <cmath>
#include <atomic>
#include <thread>
#include <chrono>
#include <vector>

#define SIZE 10

void fnSum(uint16_t* array, size_t start, size_t end){

    // std::atomic<uint16_t*> atomicArray[2]; // check notation
    // atomicArray[0] = &array[start];
    // atomicArray[1] = &array[end];
    
    // uint16_t* currentPtr = atomicArray[0].load(std::memory_order_acquire);
    // uint16_t* nextPtr = atomicArray[1].load(std::memory_order_acquire);
    // uint16_t* sum = new uint16_t(*currentPtr + *nextPtr);
    // atomicArray[1].store(sum, std::memory_order_release);
    // array[end] = *atomicArray[1];
    // delete sum;

    array[end] += array[start];
}

// for(size_t k=2; k<=pow(2,log2(end-start)); k*=2){
        // for(size_t i=start; i<end/(size_t)pow(2,k+1); i+=(size_t)pow(2,k+1)){

void reduce(uint16_t* array, size_t size){
    uint8_t nthreads = std::thread::hardware_concurrency();
    std::vector<std::thread> threadObj;
    threadObj.reserve(nthreads);
    for(size_t k=0; k<(size_t)log2(size); k++){
        size_t threadsToCreate = size / ((size_t)pow(2, k + 1));
        size_t chunks = 1;
        size_t chunkSize=size;
        if(threadsToCreate >= nthreads){
            chunks = threadsToCreate / nthreads + 1;
            chunkSize = size/chunks;
        }
        for (size_t threadGroup=0; threadGroup < chunks; threadGroup++){
            // printf("k: %ld\n",k);
            for (size_t i = ((1 << k) -1 + threadGroup*chunkSize); i < (size - (1 << k) + 1 - (chunkSize*(chunks-threadGroup-1))); i+= (1 << (k+1)))
            {
                // printf("max: %ld ", (size - (1 << k) + 1 - (chunkSize*(chunks-threadGroup-1))));
                // printf("start: %ld, end: %ld \n", i, (i + (1 << k)));
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

void blellochScan(uint16_t* array, size_t size){
    reduce(array, size);
    //upSweep(array, size);
}


// check for overflows

int main(){

    uint16_t* array = (uint16_t*)malloc(SIZE*sizeof(uint16_t));

    srand(time(NULL));

    printf("The array in question is: ");
    for (size_t i=0; i<SIZE; i++){
        array[i] = rand() % 100;
        printf("%d ", array[i]);
    }
    printf("\n");
    auto start = std::chrono::steady_clock::now();
    blellochScan(array, SIZE);
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


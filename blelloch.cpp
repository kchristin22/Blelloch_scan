#include <iostream>
#include <cstdlib>
#include <cmath>
#include <atomic>
#include <thread>
#include <chrono>
#include <vector>

#define SIZE 100000

void fnSum(uint16_t* array, size_t start, size_t end){
    array[end] += array[start];
}

// for(size_t k=2; k<=pow(2,log2(end-start)); k*=2){
        // for(size_t i=start; i<end/(size_t)pow(2,k+1); i+=(size_t)pow(2,k+1)){

void reduce(uint16_t* array, size_t size){
    uint8_t nthreads = std::thread::hardware_concurrency();
    std::vector<std::thread> threadObj;
    threadObj.reserve(nthreads);
    size_t threadsToCreate, chunks = 1, chunkSize = size;

    for(size_t k=0; k< (size_t)log2(size); k++){
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

void downSweep(uint16_t* array, size_t size){
    array[size-1] = 0;
    uint8_t nthreads = std::thread::hardware_concurrency();
    std::vector<std::thread> threadObj;
    threadObj.reserve(nthreads);
    size_t threadsToCreate, chunks, chunkSize;

    for (size_t k = 0; k < (size_t)log2(size); k++){
        threadsToCreate = size / ((size_t)pow(2, k + 1));
        chunks = 1;
        chunkSize = size;

        if(threadsToCreate >= nthreads){
            if (threadsToCreate % nthreads == 0){
                chunks = threadsToCreate / nthreads;
            }
            else{
                chunks = threadsToCreate / nthreads + 1;
            }
            chunkSize = size/chunks;
        }

        for (size_t chunk = 0; chunk < chunks; chunk++){
            
        }
    }
}

// a[n − 1] ← 0 % Set the identity
// for d from (lg n) − 1 downto 0
// in parallel for i from 0 to n − 1 by 2d+1
// t ← a[i + 2d − 1] % Save in temporary
// a[i + 2d − 1] ← a[i + 2d+1 − 1] % Set left child
// a[i + 2d+1 − 1] ← t + a[i + 2d+1 − 1] % Set right child


void blellochScan(uint16_t* array, size_t size){
    reduce(array, size);
    // downSweep(array, size);
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


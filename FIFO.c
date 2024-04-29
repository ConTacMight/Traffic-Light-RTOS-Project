// FIFO.c
// Runs on TM4C123

#include <stdint.h>
#include "FIFO.h" 

#define FSIZE 10    // can be any size
uint32_t PutI;      // index of where to put next
uint32_t GetI;      // index of where to get next
uint32_t Fifo[FSIZE][512]; //array of sectors for buffering
int32_t CurrentSize;// 0 means FIFO empty, FSIZE means full
uint32_t LostData;  // number of lost pieces of data

void OS_FIFO_Init(void) {
	//***IMPLEMENT THIS***
	PutI = GetI = CurrentSize = LostData = 0;
}

// ******** OS_FIFO_Put ************
// Put an entry in the FIFO.  
// Exactly one event thread puts,
// do not block or spin if full
// Inputs:  data to be stored
// Outputs: 0 if successful, -1 if the FIFO is full
int OS_FIFO_Put(uint32_t data[512]) {
	//***IMPLEMENT THIS***
	if (CurrentSize >= FSIZE) {
		LostData++; // FIFO is full, data is lost
		return -1; // Indicate failure
	}
	for (int i = 0; i < 512; i++) {
		Fifo[PutI][512] = data[i];
	}
	PutI = (PutI + 1) % FSIZE; // Update PutI
	CurrentSize++; // Increment current size

	return 0;   // success
}

// ******** OS_FIFO_Get ************
// Get an entry from the FIFO.   
// Exactly one main thread get,
// do block if empty
// Inputs:  none
// Outputs: data retrieved
uint32_t OS_FIFO_Get(void) {
	uint32_t data;
	//***IMPLEMENT THIS***
	if (CurrentSize == 0) {
		return 0; // FIFO is empty
	}
	data = Fifo[GetI]; // Get data from FIFO
	GetI = (GetI + 1) % FSIZE; // Update GetI
	CurrentSize--; // Decrement current size
	return data;
}

void FIFO_Thread(void) {
    int32_t status;

    // Read data from FIFO
    status = OS_FIFO_Get(); 
    if(status == 0) {
        // Mount directory if not already loaded
        MountDirectory();
        // Append data to a file using eFile_edit
        status = OS_File_Append(3, data); 
        
    } else {
        return -1;
    }
}

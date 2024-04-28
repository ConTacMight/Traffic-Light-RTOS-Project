// eFile_edit.c
// Runs on either TM4C123 or MSP432
// High-level implementation of the file system implementation.
// based off of Daniel and Jonathan Valvano
// 4/24/2024
#include <stdint.h>
#include "FIFO.h"//********************************************************************************

uint8_t Directory[256], FAT[256];
int32_t bDirectoryLoaded = 0; // 0 means disk on ROM is complete, 1 means RAM version active

// Return the larger of two integers.
int16_t max(int16_t a, int16_t b) {
	if (a > b) {
		return a;
	}
	return b;
}
//*****MountDirectory******
// if directory and FAT are not loaded in RAM,
// bring it into RAM from disk
void MountDirectory(void) {
	// if bDirectoryLoaded is 0, 
	//    read disk sector 255 and populate Directory and FAT
	//    set bDirectoryLoaded=1
	// if bDirectoryLoaded is 1, simply return
	// **write this function**
	if (!bDirectoryLoaded) { // if bDirectoryLoaded is not 1 (0 in fact)
		//eDisk_ReadSector(&Buff[0], 255); // read disk sector 255 ***************************************************************************************
		uint8_t *buffer = OS_FIFO_Get();
		for (int i = 0; i < 256; i++) { // populate Directory and FAT
			Directory[i] = buffer[i];
			FAT[i] = buffer[256 + i];
		}
		bDirectoryLoaded = 1; // set bDirectoryLoaded=1
	}
}

// Return the index of the last sector in the file
// associated with a given starting sector.
// Note: This function will loop forever without returning
// if the file has no end (i.e. the FAT is corrupted).
uint8_t lastsector(uint8_t start) {
	// **write this function**
	uint8_t n = 0;
	if (start == 255) {
		return 255;
	}
	else {
		while (n != 255) {
			n = FAT[start];
			if (n == 255) {
				return start; // last sector in a file
			}
			else {
				start = n; // point to next sector in list
			}
		}
	}
	return 255; // replace this line
}

// Return the index of the first free sector.
// Note: This function will loop forever without returning
// if a file has no end or if (Directory[255] != 255)
// (i.e. the FAT is corrupted).
uint8_t findfreesector(void) {
	// **write this function**
	int16_t e = -1;
	uint8_t i = 0;
	int16_t ls = (int16_t)lastsector(Directory[i]);
	while (ls != 255) {
		e = max(e, ls);
		i = i + 1;
		ls = lastsector(Directory[i]);
	}
	return (e + 1); // replace this line
}

//********OS_File_Append*************
// Save 512 bytes into the file
// Inputs:  num, 8-bit file number, 0 to 254
//          buf, pointer to 512 bytes of data
// Outputs: 0 if successful
// Errors:  255 on failure or disk full
uint8_t OS_File_Append(uint8_t num, uint8_t buf[512]) {
	// **write this function**
	MountDirectory(); // bring DIR and FAT from ROM to RAM if needed
	uint8_t free = findfreesector();
	
	if (free == 255) {
		return 255; // disk full or failure
	}
	int status = OS_File_Put(buf);
	if (status == -1) {
		return 255;
	}
	appendfat(num, free);

	return 0;
}

//********OS_File_Read*************
// Read 512 bytes from the file
// Inputs:  num, 8-bit file number, 0 to 254
//          location, logical address, 0 to 254
//          buf, pointer to 512 empty spaces in RAM
// Outputs: 0 if successful
// Errors:  255 on failure because no data
uint8_t OS_File_Read(uint8_t num, uint8_t loc,
	uint8_t buf[512]) {
	// **write this function**
	// Find the starting sector of the file
	uint8_t place = Directory[num];
	if (place == 255) {
		return 255;
	}
	// Move to the specified logical address
	uint8_t m = 0;
	while (m < loc && FAT[place] != 255) {
		place = FAT[place];
		m++;
	}
	// Read sector data from the FIFO buffer
	for (int i = 0; i < 512; i++) {
		buf[i] = Fifo[place][i];
	}
	return 0;
}

//********OS_File_Flush*************
// Update working buffers onto the disk
// Power can be removed after calling flush
// Inputs:  none
// Outputs: 0 if success
// Errors:  255 on disk write failure
uint8_t OS_File_Flush(void) {
	// **write this function**

	//copy directory and FAT to buffer
	uint8_t buff[512];
	for (int i = 0; i < 256; i++) {
		buff[i] = Directory[i];
		buff[256 + i] = FAT[i];
	}
	
	//write buffer data to FIFO
	unint8_t status = OS_FIFO_Put(buff);
	if (status == -1) {
		return 255;
	}
}

//********OS_File_Format*************
// Erase all files and all data
// Inputs:  none
// Outputs: 0 if success
// Errors:  255 on disk write failure
uint8_t OS_File_Format(void) {
	//erase all files and data in directory and FAT
	for (int i = 0; i < 256; i++) {
		Directory[i] = 255;
		FAT[i] = 0;
	}

	uint8_t status = OS_File_Flush();
	if (status == 255) {
		return 255;
	}
	bDirectoryLoaded = 0;
	return 0;
}
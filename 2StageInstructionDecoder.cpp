#pragma warning(disable : 4996) //IDE command to disable warnings about fopen and fscan being unsafe

#include <bitset>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>

#define HALT_OPCODE 0x19
#define SIZE_OF_MEMORY 65536

using namespace std;

void fetchNextInstruction(void);
void executeInstruction(void);
void readFileToMemory(const char* filename);
void fileOut(void);

unsigned char memory[SIZE_OF_MEMORY];
unsigned char ACC = 0;
unsigned char IR = 0;
unsigned int MAR = 0;
unsigned int PC = 0;
unsigned int RestorePC;	//For Endian Manipulations

void fetchNextInstruction(void) {
	IR = memory[PC];
	RestorePC = PC; //see execution function
	PC++; //move past opcode

	//move the PC in preparation for program execution
	//always 8 bit opcode
	//either 8 or 16 following bits for data or an address

    //Caleb - fetch math and logical
	// Mathematical or Logical Operation
	if (IR & 0x80)
	{
		bool twoByteFlag = false;
		switch ((IR & 0x0f) >> 2) { //0000 XX00 //Shift right so numbers 
		case 0: break; //Indirect (MAR used as pointer)
		case 1: break; //Accumulator ACC
		case 2: twoByteFlag = true; break; //Adress register MAR
		case 3: PC += 2; break; //Memory
		}

		switch (IR & 0x03) { //0000 00XX
		case 0: break; //Indirect (MAR used as pointer)
		case 1: break; //Accumulator ACC
		case 2: PC++; if (twoByteFlag) { PC++;} break; //Constant
		case 3: PC += 2; break; //Memory
		}
	}
	
	//Regan - fetch jump,branch and memory  
	else if ((IR & 0xf0) == 0) // 0000 XXXX //Memory Operation
    	{ 
            if((IR & 0x04) == 0) //0000 0100 //Accumulator 0
                {
                    switch (IR & 0x03)
                        {
                                //Operand is used as address //000
                    			//Register Accumulator
                    		case 0: PC += 2; break;
                    
                    			//Operand is used as a constant //001
                    			//Register Accumulator
                    		case 1: PC++; break;
                    
                    			//Indirect (MAR used as a pointer)	//010
                    			//Register Accumulator
                    		case 2: break;
                        }
                }
                
            else if((IR & 0x04) == 4) //0000 0100 //MAR 1
                {
                    switch (IR & 0x03)  
                        {
                                //Operand is used as address //000
                    			//Register Accumulator
                    		case 0: PC += 2; break;
                    
                    			//Operand is used as a constant //001
                    			//Register Accumulator
                    		case 1: PC += 2; break;
                    
                    			//Indirect (MAR used as a pointer)	//010
                    			//Register Accumulator
                    		case 2: break;
                        }
                }
    	}

	else if ((IR & 0xf8) == 0X10) //1111 1000 Mask //0001 0000 Check 5 MSBs for branch/jump
		PC += 2;
	else
    	{
    
    	}

}

void executeInstruction(void)
{
	int address;

	if ((IR & 0x80) == 0x80)
	{
		char destinationOperator;
		char sourceOperator;
		char result;
		bool twoByteFlag = false; //used to determine size of constant operator
                                //constant size in dependant on the source
        //Caleb - math ad logical execution

		switch (IR & 0x0c) { //0000 1100 //Destination
			//adjust magnitude of case for 3rd and 4th bit
		case 0x00: //00 //Indirect (MAR used as pointer)
			destinationOperator = memory[MAR];
			break;
		case 0x04: //0100 //Accumulator ACC
			destinationOperator = ACC;
			break;
		case 0x08: //1000 //Address register MAR
			destinationOperator = MAR;
			twoByteFlag = true;
			break;
		case 0x0c: //1100 // Memory
			destinationOperator = memory[((memory[RestorePC + 1] << 8) + memory[RestorePC + 2])];
			break;
		default: printf("Unknown destination\n"); break;
		}

		//get second operator
		switch (IR & 0x03) { //0000 0011 //Source
		case 0: //00 //Indirect (MAR used as pointer)
			sourceOperator = memory[MAR]; 
			break;
		case 1: //01 //Accumulator ACC
			sourceOperator = ACC; 
			break;
		case 2: //10 //Constant
			sourceOperator = memory[RestorePC + 1]; 
			if (twoByteFlag) {
				sourceOperator <<= 8;
				sourceOperator += memory[RestorePC + 2];
			}
			break;
		case 3: //11 //Memory
			sourceOperator = memory[((memory[RestorePC + 1] << 8) + memory[RestorePC + 2])];
			break;
		default: printf("Unknown source\n"); break;
		}

		//perform operation
		switch (IR & 0x70) { //0111 0000
		case 0x00:
			result = destinationOperator & sourceOperator; //AND operation
			break;
		case 0x10:
			result = destinationOperator | sourceOperator; //OR operation
			break;
		case 0x20:
			result = destinationOperator ^ sourceOperator; //XOR operation
			break;
		case 0x30:
			result = destinationOperator + sourceOperator; //ADD operation
			break;
		case 0x40:
			result = destinationOperator - sourceOperator; //SUB operation
			break;
		case 0x50:
			result = sourceOperator++; //INC operation
			break;
		case 0x60:
			result = sourceOperator--; //DEC operation
			break;
		case 0x70:
			result = ~sourceOperator; //NOT operation
			break;
		default: printf("Unknown function\n"); break;
		}

		//save operation result to destination location
		switch (IR & 0x0c) { //0000 1100 //Destination
		case 0x00: //00 //Indirect (MAR used as pointer)
			memory[MAR] = result;
			break;
		case 0x04: //0100 //Accumulator ACC
			ACC = result;
			break;
		case 0x08: //1000 //Address register MAR
			MAR = result;
			break;
		case 0x0c: //1100 // Memory
			memory[(memory[RestorePC + 1] << 8) + memory[RestorePC + 2]] = result;
			break;
		}
	}

	else if ((IR & 0xf0) == 0) //1111 0000 Mask //4 MSBs Memory Op
	{
		if ((IR & 0x08) == 0) //0000 1000 Mask  // Store 0 //meaning memory[] = Value
		{
			if ((IR & 0x04) == 0) //0000 0100 //0 Accumulator
    			{  
    				switch (IR & 0x03) //0000 0011 Mask //2 LSBs
        				{
            					//ACC (an 8-bit register) 
            					
            					//00 //Operand is used as address
            					//memory[x] returns an 8 bit number
            						//Decode one 8 bit, shift magnitude to MSB, add second 8 bit number
                				case 0:memory[((memory[RestorePC + 1] << 8) + memory[RestorePC + 2])] = ACC;
                					break;
                
                					//01 // Operand is used as a Constant 	
                				case 1: break;
                
                					//10 //Indirect (MAR used as a pointer)
                				case 2: memory[MAR] = ACC; break;
                
                					//Non-Conditions like 11 
                				default:break;
        				}
    			}
			
			else if((IR & 0x04) == 4) //1 Index register MAR
    			{
    				switch (IR & 0x03) //0000 0011 Mask 
        				{
        					//Teacher Notes:
        						//MAR is a sixteen bit register.
        
        						//The "<<8" shifts the most significant byte 8 places to the left leaving zeros in the low byte for the second byte of the operand to be added.
        								//e.i shift, add method
        
        					//Process:
        						//00 //Operand is used as address
        						// store MAR in address operand
        						//We can only fill one 8 bit section of the memory at a time
        							//MAR is 16 bits
        								//grab MSB bits, store, than grab LSBs, store
        									//increment RestorePC accordingly, starting location is op code
        											//get 16 bits address AND move to next sequential address to store full MAR value
        													//Cast with a Mask to ensure only 8 bits are stored
            				case 0:
            					memory[((memory[RestorePC + 1] << 8) + memory[RestorePC + 2])] = (MAR >> 8) & 0xff;
            					memory[((memory[RestorePC + 1] << 8) + memory[RestorePC + 2]) + 1] = MAR & 0xff;
            					break;
            
            					//01 // Operand is used as a Constant	
            				case 1: break;
            
            					//10 //Indirect (MAR used as a pointer)
            				case 2:
            					//Register is a addresss point and the stored values
            						//no data conflict should occur
            							//MAR doesnt change
            					memory[MAR] = (MAR >> 8) & 0xff;
            					memory[MAR + 1] = MAR & 0xff;
            					break;
            
            				default: break;
        				}
    			}
		    }
		    
		else if((IR & 0x08) == 8) //Load 1
		{ 
			if ((IR & 0x04) == 0) //register Accumulator //0
    			{
    				switch (IR & 0x03) //0000 0011 Mask
        				{
            					//00 //Operand is used as address
            				case 0: ACC = memory[((memory[RestorePC + 1] << 8) + memory[RestorePC + 2])]; break;
            
            					//01 //Operand is used as a constant 	
            				case 1: ACC = memory[RestorePC + 1]; break;
            
            					//10 //Indirect (MAR used as a pointer)	
            				case 2:ACC = memory[MAR]; break;
            
            				default: break;
        				}
    			}
    			
			else if((IR & 0x04) == 4)  //register Adress MAR //1
    			{
    				int RestoreMAR = MAR;
    				//prevent overwrite if indirect destination and source address point
    
    				switch (IR & 0x03)
        				{
            					//00 //Operand is used as address
            				case 0:
            					MAR = memory[((memory[RestorePC + 1] << 8) + memory[RestorePC + 2])];
            					MAR = MAR << 8;
            					MAR = MAR + memory[((memory[RestorePC + 1] << 8) + memory[RestorePC + 2]) + 1];
            					break;
            
            					//01 //Operand is used as a constant	
            				case 1:
            					MAR = memory[RestorePC + 1];
            					MAR = MAR << 8;
            					MAR = MAR + memory[RestorePC + 2];
            					break;
            
            					//10 //Indirect (MAR used as pointer)	
            				case 2:
            					//uniqie issue, the MAR is being loaded using a value pointed at by the MAR
            						//this could cause a sequence conflict if we dont use a RestoreMAR variable to maintain the correct MAR for memory pointing
            					MAR = memory[RestoreMAR];
            					MAR = MAR << 8;
            					MAR = MAR + memory[RestoreMAR + 1];
            					break;
            
            					//Non-conditions like 11
            				default: break;
        				}
    			}
		}
	}

	else if ((IR & 0xF8) == 0x10) //1111 1000 Mask //0001 0000 Result meaning Branch/Jump
    	{
    		address = (memory[RestorePC + 1] << 8) + memory[RestorePC + 2]; //16 bits
    
    		switch (IR & 0x07) //0000 0111 Mask
        		{
            			//000 //BRA (Unconditional branch/branch always)
            		case 0:
            			PC = address; break;
            
            			//001 //BRZ (Branch if ACC = 0)	
            		case 1:
            			if (ACC == 0)
            				PC = address;
            			break;
            
            			//010 // BNE (Branch if ACC != 0)	
            		case 2:
            			if (ACC != 0)
            				PC = address;
            			break;
            
            			//011 // BLT (Branch if ACC < 0)	
            		case 3:
            			if (ACC < 0)
            				PC = address;
            			break;
            
            			//100 // BLE (Branch if ACC <= 0) 	
            		case 4:
            			if (ACC <= 0)
            				PC = address;
            			break;
            
            			// BGT (Branch if ACC > 0)	
            		case 5:
            			if (ACC > 0)
            				PC = address;
            			break;
            
            			// BGE (Branch if ACC >= 0)	
            		case 6:
            			if (ACC >= 0)
            				PC = address;
            			break;
            
            		default:break;
        		}
    	}
}

//Mai - All File I/O and Main

void readFileToMemory(const char* filename) { // pass in the mem_in.txt file as a parameter and read/store its contents to an array

	int hex; // variable to store hexadecimal values read from the file
	int i = 0; // var to iterate through memory array

	FILE* fptr = fopen(filename, "r"); // open the mem_in.txt file for reading

	if (fptr == NULL) { // terminate if file is not successfully opened 
		printf("Error opening file.");
		exit(1);
	}

	while (fscanf(fptr, "%02x", &hex) != EOF) { // while the file has not reached the end read the hex values 
		memory[i++] = (unsigned char)hex; // convert the hex values to unsigned char and store them in 'memory' array
	}

	fclose(fptr); // close file

}

// write the contents of the 'memory' array to a file
void fileOut() {
	FILE* foutptr = fopen("mem_out.txt", "w"); // create/open the mem_out.txt file for writing

	if (foutptr == NULL) { // terminate if file is not successfully opened 
		printf("Error opening file out.");
		exit(1);
	}
    
	for (int i = 0; i < SIZE_OF_MEMORY; i++) { // iterate through the 'memory' array and write each byte as a hex value to the file
		fprintf(foutptr, " %02x ", memory[i]);
        
	}
    
	fclose(foutptr); // close file 
}

int main(int argc, char* argv[]) {
	if (argc != 2) { // check if correct number of arguments are provided
		printf("Usage: %s <filename>\n", argv[0]);
		return 0;
	}

	readFileToMemory(argv[1]);

	while (memory[PC] != HALT_OPCODE) {
		fetchNextInstruction();
		executeInstruction();
	}

    fileOut();
	return 0;
}

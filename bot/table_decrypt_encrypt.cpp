#include <Windows.h>
#include <stdlib.h>
#include <iostream>
#include <vector>

#include "DeTable.h"
#include "EnTable.h"
#include "table_decrypt_encrypt.h"

using namespace std;

int recv_key = 0;
int send_key = 0;
int send_key_engine = 0;


// given the key and data, decrypt using DeTable
void table_decode(int key, vector<char> *packet){
	vector<char>::iterator data = packet->begin() + 2;
	vector<char>::iterator data_end = packet->end();
	register const char* tablePtr = (char*)(DeTable[key]);

	while( data != data_end ){
		*data = tablePtr[(unsigned char)*data];
		++data;
	}
}

// given the key and data, enccrypt using EnTable
void table_encode(int key, vector<char> *packet){
	vector<char>::iterator data = packet->begin() + 2;
	vector<char>::iterator data_end = packet->end();
	register const char* tablePtr = (char*)(EnTable[key]);

	while (data != data_end){
		*data = tablePtr[(unsigned char)*data];
		++data;
	}
}

#include <windows.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <conio.h>
#include <stdio.h>

#include "payload.h"

using namespace std;

struct S_payload {
	DWORD size;

	DWORD entrypoint_virtual;
	DWORD entrypoint_file;

	DWORD backup_virtual;
	DWORD backup_file;
	DWORD backup_size_available;

	//DWORD payload_virtual;
	DWORD payload_file;
	DWORD payload_size_available;

	DWORD popad_retn_virtual;
};

const char* error_messages[] =
{
	"NO ERROR",
	"Couldn't open file",
	"Couldn't file map file",
	"Couldn't file map view file",
	"No DOS-Header found",
	"No NT-Header found",
	"No Entrypoint found",
	"Reloc in use, but not segment found",
	"This file got patched already !"
};

int get_dll_payload_data(char* dll_name, S_payload* payload_data){
	HANDLE hFile, hMap, hMapView;
	PIMAGE_DOS_HEADER pDosHeader;
	PIMAGE_NT_HEADERS pImageHeader;

	hFile = CreateFileA(dll_name, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile == INVALID_HANDLE_VALUE){
		return 1;
	}

	hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if(!hMap){
		return 2;
	}

	hMapView = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
	if(!hMapView){
		return 3;
	}

	//check if patched already !
	DWORD file_size = GetFileSize(hFile, 0);
	for(DWORD i = 0; i < file_size-strlen("kalmagic.dll"); ++i)
	{
		char* ptr = (char*)hMapView + i;
		if (strcmp(ptr, "kalmagic.dll") == 0)
		{
			return 8;
		}
	}

	pDosHeader = (PIMAGE_DOS_HEADER)hMapView;
	if(pDosHeader->e_magic != IMAGE_DOS_SIGNATURE){
		return 4;
	}

	pImageHeader = (PIMAGE_NT_HEADERS)((char*)pDosHeader + pDosHeader->e_lfanew);
	if(pImageHeader->Signature != IMAGE_NT_SIGNATURE){
		return 5;
	}
   
	int   best_size    = 0;
	char* best_virtual_ptr = 0;
	char* best_file_ptr = 0;
	PIMAGE_SECTION_HEADER pSections = (PIMAGE_SECTION_HEADER)((BYTE*)pImageHeader + sizeof(IMAGE_NT_HEADERS)/* + (pImageHeader->FileHeader.NumberOfSections - 1) * sizeof(IMAGE_SECTION_HEADER)*/);
	for(int i = 0; i < pImageHeader->FileHeader.NumberOfSections; ++i) {
		PIMAGE_SECTION_HEADER ptr = pSections+i;
		if (ptr->Characteristics & IMAGE_SCN_MEM_WRITE) {
			//READ and WRITE able as we need it ;)
			char* raw_data_start = (char*)hMapView + ptr->PointerToRawData;
			char* raw_data_end   = (char*)hMapView + ptr->PointerToRawData + ptr->SizeOfRawData;
			char* raw_data = raw_data_start;
			
			while(raw_data != raw_data_end)
			{
				char* raw_data_zero_start = raw_data;
				if ((raw_data++)[0] == 0)
				{
					while(raw_data != raw_data_end)
					{
						if ((raw_data++)[0] != 0)
						{
							//end of 0 reached
							int size = std::distance(raw_data_zero_start, raw_data-1);
							if (size > best_size)
							{
								best_virtual_ptr = (char*)((DWORD)ptr->VirtualAddress + ((DWORD)raw_data_zero_start - (DWORD)hMapView - (DWORD)ptr->PointerToRawData));
								best_file_ptr    = raw_data_zero_start;
								best_size        = size;
							}
						}
					}
				}
			}
		}
	}

	payload_data->entrypoint_file = 0; //(void*)(pImageHeader->OptionalHeader.ImageBase + pImageHeader->OptionalHeader.AddressOfEntryPoint);
	payload_data->entrypoint_virtual = pImageHeader->OptionalHeader.AddressOfEntryPoint;
	payload_data->backup_virtual = (DWORD)best_virtual_ptr;
	payload_data->backup_file    = (DWORD)(best_file_ptr - (char*)hMapView);
	payload_data->backup_size_available = best_size;
	payload_data->popad_retn_virtual = 0;

	//get file offset of entrypoint !
	PIMAGE_SECTION_HEADER code_section;
	DWORD want = pImageHeader->OptionalHeader.AddressOfEntryPoint;
	for(int i = 0; i < pImageHeader->FileHeader.NumberOfSections; ++i) {
		PIMAGE_SECTION_HEADER ptr = pSections+i;
		if ((ptr->VirtualAddress <= want) && (ptr->VirtualAddress+ptr->SizeOfRawData > want))
		{
			code_section = ptr;
			//found it
			DWORD offset = want;
			offset -= ptr->VirtualAddress;
			offset += ptr->PointerToRawData;
			payload_data->entrypoint_file = offset;
			break;
		}
	}

	if (payload_data->entrypoint_file == 0)
	{
		return 6;
	}

	//check reloc !
	vector<DWORD> rebases;
	vector<DWORD> rebases_virtual;
	if (pImageHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress != 0)
	{
		//got reloc ! be careful !
		IMAGE_BASE_RELOCATION *reloc_table =  0;
		for(int i = 0; i < pImageHeader->FileHeader.NumberOfSections; ++i) {
			PIMAGE_SECTION_HEADER ptr = pSections+i;
			if (ptr->VirtualAddress == pImageHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress)
			{
				reloc_table = (IMAGE_BASE_RELOCATION*)((BYTE*)hMapView + ptr->PointerToRawData);
				break;
			}
		}
		if (reloc_table == 0)
		{
			//couldn't find section !
			return 7;
		}

		PIMAGE_BASE_RELOCATION relocation = (PIMAGE_BASE_RELOCATION) (reloc_table);
        for (; relocation->VirtualAddress > 0; ) {
            unsigned short *relInfo = (unsigned short *)((unsigned char *)relocation + sizeof(IMAGE_BASE_RELOCATION));
            for (DWORD i=0; i<((relocation->SizeOfBlock-sizeof(IMAGE_BASE_RELOCATION)) / 2); i++, relInfo++) {
                //DWORD *patchAddrHL;
                unsigned type, offset; //Was int ? why ?

                // the upper 4 bits define the type of relocation
                type = *relInfo >> 12;
                // the lower 12 bits define the offset
                offset = *relInfo & 0xfff;

				if (type == IMAGE_REL_BASED_HIGHLOW)
				{
					DWORD file_offset = 0;
					DWORD virtual_offset = 0;
					for(int j = 0; j < pImageHeader->FileHeader.NumberOfSections; ++j) {
						PIMAGE_SECTION_HEADER ptr = pSections+j;
						if (ptr->Characteristics & IMAGE_SCN_MEM_EXECUTE)
						{
							//NEEDS TO BE EXECUTE ABLE :)
							if ((ptr->VirtualAddress <= want) && (ptr->VirtualAddress+ptr->SizeOfRawData > want))
							{
								if ((relocation->VirtualAddress >= ptr->VirtualAddress) && (relocation->VirtualAddress < ptr->VirtualAddress+ptr->SizeOfRawData))
								{
									//NEEDS TO BE IN SAME SEGMENT/SECTION AS ENTRYPOINT !
									file_offset = relocation->VirtualAddress - ptr->VirtualAddress + offset + ptr->PointerToRawData;
									virtual_offset = relocation->VirtualAddress + offset;

									rebases.push_back(ptr->PointerToRawData);
									rebases_virtual.push_back(ptr->VirtualAddress);
									rebases.push_back(ptr->PointerToRawData + ptr->SizeOfRawData);
									rebases_virtual.push_back(ptr->VirtualAddress + ptr->SizeOfRawData);
								}
							}
						}
					}
                
					if (file_offset != 0)
					{
						rebases.push_back(file_offset);
						rebases_virtual.push_back(virtual_offset);
					}
				}
            }
            // advance to next relocation block
            relocation = (PIMAGE_BASE_RELOCATION) (((char *) relocation) + relocation->SizeOfBlock);
        }

		//FIND LARGEST AREA IN RELOCS
		sort(rebases.begin(), rebases.end());
		rebases.resize( distance(rebases.begin(), unique(rebases.begin(), rebases.end())) );
		sort(rebases_virtual.begin(), rebases_virtual.end());
		rebases_virtual.resize( distance(rebases_virtual.begin(), unique(rebases_virtual.begin(), rebases_virtual.end())) );
		DWORD from = 0; DWORD size = 0;
		DWORD from_virtual = 0;
		if (rebases.size() > 1)
		{
			for(size_t i = 0; i < rebases.size()-1; ++i)
			{
				if (rebases[i+1] - rebases[i]+sizeof(void*) > size)
				{
					from_virtual = rebases_virtual[i]+sizeof(void*);
					//check important segments of memory !.. fuck this
					bool okay = true;
					for(size_t j = 0; okay && (j <= 14); ++j)
					{
						if ((from_virtual >= pImageHeader->OptionalHeader.DataDirectory[j].VirtualAddress) && 
							(from_virtual <  pImageHeader->OptionalHeader.DataDirectory[j].VirtualAddress+pImageHeader->OptionalHeader.DataDirectory[j].Size))
						{
							okay = false;
						}
					}
					if (okay)
					{
						from =  rebases[i]+sizeof(void*);		
						size = rebases[i+1] - rebases[i]+sizeof(void*);
					}
				}
			}
		}

		payload_data->payload_file = from;
		payload_data->payload_size_available = size;
	}

	//check for popad retn => 58 61 C3
	for(size_t i = 0; i < code_section->SizeOfRawData-3; ++i)
	{
		char* data_ptr = (char*)hMapView + code_section->PointerToRawData + i;
		if ((data_ptr[0] == 0x58) && (data_ptr[1] == 0x61) && (data_ptr[2] == 0xC3))
		{
			//check if rebased !
			size_t j = 0;
			for(;j < rebases.size(); ++j)
			{
				if ((rebases[j] <= code_section->VirtualAddress+j) && (rebases[j]+4 > code_section->VirtualAddress+j))
				{
					//within rebase
					break;
				}
			}
			if (j == rebases.size())
			{
				//found good one !
				payload_data->popad_retn_virtual = code_section->VirtualAddress+j;
				break;
			}
		}
	}

	if (payload_data->popad_retn_virtual == 0)
	{
		//couldn't find popad retn !
		//return EXIT_FAILURE;
	}

	UnmapViewOfFile(hMapView);
	CloseHandle(hMap);
	CloseHandle(hFile);

	return 0;
}

int get_payload_size()
{
	int size = 0;
	void** ptr = (void**)get_payload();
	while(*ptr != (void*)0x90909090) //4 times nop
	{
		ptr = (void**)((char*)ptr + 1);
		size += 1;
	}
	return size;
}

void replace(char* payload, size_t size, void* from, void* to)
{
	for(size_t i = 0; i < size-sizeof(void*); ++i)
	{
		void** ptr = (void**)(payload+i);
		if (*ptr == from)
		{
			*ptr = to;
		}
	}
}

int main(int argc, char * argv[]){
	if (argc != 2)
	{
		cout << "Usage: " << argv[0] << " <DLLNAME>" << endl;
		system("PAUSE");
		return EXIT_FAILURE;
	}

	cout << "Extract the data we need ... from " << argv[1] << endl;
	S_payload payload_data;
	memset(&payload_data, 0, sizeof(S_payload));
	payload_data.size = get_payload_size();

	int res = get_dll_payload_data(argv[1], &payload_data);
	if (res != 0)
	{
		cout << error_messages[res] << endl;
		return EXIT_FAILURE;
	}

	if (payload_data.popad_retn_virtual == 0)
	{
		cout << "No stack-fixer found, but it's no problem we will use the stack itself" << endl;
	}
	else
	{
		cout << "Found a stack-fixer at " << (void*)payload_data.popad_retn_virtual << " but we wont use it..." << endl;
	}
	
	if (payload_data.size+5 > payload_data.backup_size_available) {
		cout << "Could not get enough space for payload" << endl;
		system("PAUSE");
		return EXIT_FAILURE;
	}

	if (payload_data.payload_file != 0)
	{
		if (payload_data.size+5 > payload_data.payload_size_available) {
			cout << "Could not get enough space for payload" << endl;
			system("PAUSE");
			return EXIT_FAILURE;
		}
	}
	else
	{
		payload_data.payload_file = payload_data.entrypoint_file + 5;
		payload_data.payload_size_available = payload_data.size + 5;
	}

	char* payload = new char[payload_data.size+4];     //+4 makes pointer reading more easy ;)
	memset(payload, 0x90, payload_data.size+4);        //fill with nop
	memcpy(payload, get_payload(), payload_data.size); //copy our data
	//search for 0x11111111 => is our size
	//search for 0x22222222 => is our realative, by entrypoint, offset to backupdata !
	replace(payload, payload_data.size, (void*)0x11111111, (void*)payload_data.size);
	replace(payload, payload_data.size, (void*)0x22222222, (void*)(payload_data.backup_virtual - payload_data.entrypoint_virtual));

	cout << "Patch the file.." << endl;
	fstream file(argv[1], ios::binary|ios::in|ios::out);
	if (file.is_open())
	{
		char  backup_entry[5];
		char* backup = new char[payload_data.size];

		//make a backup of call
		file.seekg(payload_data.entrypoint_file, ios_base::beg);
		file.read(backup_entry, 5);
		//memset(backup_entry, 0xCC, 5); //for testing
		file.seekp(payload_data.backup_file, ios_base::beg);
		file.write(backup_entry, 5);

		#pragma pack(1)
		struct s_JMP
		{
			unsigned char opcode;
			int  offset;
		} entry;

		entry.opcode = 0xE8;
		entry.offset = payload_data.payload_file - payload_data.entrypoint_file - sizeof(s_JMP);

		file.seekp(payload_data.entrypoint_file, ios_base::beg);
		file.write((char*)&entry, sizeof(s_JMP));

		//make a backup
		file.seekg(payload_data.payload_file, ios_base::beg);
		file.read(backup, payload_data.size);
		//memset(backup, 0xCC, payload_data.size); //for testing
		file.seekp(payload_data.backup_file+5, ios_base::beg);
		file.write(backup, payload_data.size);

		//write code
		file.seekp(payload_data.payload_file, ios_base::beg);
		file.write(payload, payload_data.size);

		file.close();
	}
	else
	{
		cout << "Could not open file " << endl;
		system("PAUSE");
		return EXIT_FAILURE;
	}

	cout << "FINISH" << endl;
	system("PAUSE");
	return EXIT_SUCCESS;
}

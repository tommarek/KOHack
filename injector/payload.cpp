#include <Windows.h>
#include "payload.h"

#define KAL_RELEASE
__declspec(dllexport, naked) void* payload_asm()
{
	__asm
	{
		call getOffsetHelper
		//__asm __emit 0xCC //int 3, if you use it don't for get "sub ebx, 7" some lines below !!
		//there are still 1 parameters on the stack !
		pushad //store registers eax, ecx, edx, ebx, esp, ebp, esi, edi => esp changed by 8*4
		call offsetCheck
offsetCheck:

		pop  ebx
		sub  ebx, 6 //-6 because of pushad !, -7 because of int 3

		mov  eax, [esp+0x20] //should be before first push
		sub  eax, 5 //-5 because of call
		mov  [esp+0x20], eax //store right value of stack

		//push code for stack fix:
		push  0x90C36158 //pop eax, popad, retn, nop
		mov  ecx, esp    //offset of the pushed code

		//setup stack
		mov  ebp, esp     //should be already the same but lets do it

		push eax          //[ebp-0x04]  entrypoint offset
		push 0x00000000   //[ebp-0x08]  kalmagic.dll
		add  eax, 0x22222222
		push eax          //[ebp-0x0C]  backup offset
		push 0x11111111   //[ebp-0x10]  payload size
		push 0x00000000   //[ebp-0x14]  old-virtual-protection-entry
		push 0x00000000   //[ebp-0x18]  old-virtual-protection-payload
		push ebx          //[ebp-0x1C]  payload offset
		push 0x00000000   //[ebp-0x20]  VirtualProtect
		push 0x00000000   //[ebp-0x24]  ZeroMemory
		push 0x00000000   //[ebp-0x28]  RtlMoveMemory
		push 0x00000000   //[ebp-0x2C]  Kernel32
		push 0x00000000   //[ebp-0x30]  NtDll
		push ecx          //[ebp-0x34]  Stackfix code offset (on stack)

		call byPassFunctions
		__asm _emit 'k' __asm _emit  'o' __asm _emit  'k' __asm _emit  'u' __asm _emit  'm' __asm  _emit  'a'__asm _emit  'g' __asm _emit  'i' __asm _emit  'c' __asm _emit  '.' __asm _emit  'd' __asm _emit  'l' __asm _emit  'l' __asm _emit  '\0'
//LIBRARY START
/*
getFunctionSimple does this now !!

getKernel32Adr:
		//load address of Kernel32
		mov     eax, fs:[ 0x30 ]
		mov     eax, [eax+0Ch]
		mov     eax, [eax+14h]
		mov     eax, [eax]
		mov     eax, [eax]
		mov     eax, [eax+10h]
		//adress of kernel32 on EAX now !
		retn
getNtDllAdr:
		mov     eax, fs:[0x30]   // PEB address
        mov     eax, [eax+0x0c]  // PEB->Ldr
        mov     eax, [eax+0x1c]  // Ldr.InInitializationOrderModuleList (ntdll)
        mov     eax, [eax+0x08]  // ntdll base address
		retn
*/
getFunctionSimple:
		//__asm __emit 0xCC
		pop     edx
		pop     ecx //second parameter
		pop     ebx //first  parameter
		push    edx //back jmp

		mov     edx, fs:[0x30]   // PEB address
        mov     edx, [edx+0x0c]  // PEB->Ldr
        mov     edx, [edx+0x1c]  // Ldr.InInitializationOrderModuleList (ntdll)

getFunctionSimple_start:
		cmp     edx, 0
		jz      getFunctionSimple_end

		mov     eax, [edx+0x08]
		cmp     eax, 0
		jz      getFunctionSimple_end

		push    edx
		push    ebx
		push    ecx

		push    ebx
		push    ecx

		call    getFunction

		pop     ecx
		pop     ebx
		pop     edx

		cmp     eax, 0
		jnz     getFunctionSimple_end //found function

		//try next module;
		mov     edx, [edx]

		jmp getFunctionSimple_start
getFunctionSimple_end:
		retn
getFunction:
		mov     edx, eax
		mov     eax, [edx+3Ch]
		mov     eax, [eax+edx+78h]
		add     eax, edx
		mov     ecx, [eax+20h]
		mov     esi, [eax+18h]
		push    esi
		lea     edi, [ecx+edx]
		lea     ecx, [ecx+esi*4]
		add     ecx, edx
loc_40120E:
		cmp     edi, ecx
		jz      short loc_40122C_not_found
		mov     esi, [edi]
		add     edi, 4
		mov     ebx, dword ptr [esp+8]
		cmp     dword ptr [esi+edx], ebx//64616F4Ch
		jnz     short loc_40120E
		mov     ebx, dword ptr [esp+12]
		cmp     dword ptr [esi+edx+0Ah], ebx//41784579h
		jnz     short loc_40120E
		//okay:
		pop     ebx
		mov     esi, [eax+24h]
		mov     eax, [eax+1Ch]
		sub     ecx, edi //end-ptr
		sar     ecx, 2 //end sar 2 ? => div by 4 !
		sub     ebx, ecx
		lea     ecx, [esi+ebx*2]
		movzx   ecx, word ptr [ecx+edx] //this is
		lea     eax, [eax+ecx*4] //this is distance*4
		mov     eax, [eax+edx-4]   //?? + distance-4
		add     eax, edx
loc_40122C:
		retn    8 //remove 4 values from stack..
loc_40122C_not_found:
		mov     eax, 0
		pop     ebx
		jmp     loc_40122C
//LIBRARY END
		
byPassFunctions:
		pop eax

		mov [ebp-0x08], eax //absolute adress of "kalmagic.dll"
		
		//1. get NtDll
		/*call    getNtDllAdr
		mov     [ebp-0x30], eax */

		//2. get Kernel32
		/*call    getKernel32Adr
		mov     [ebp-0x2C], eax*/

		//3. load dll
		push    41784579h
		push    64616F4Ch   //as test	
		call    getFunctionSimple //call    getFunction //search for LoadLibraryExA in Kernel32
		
		push    0
		push    0
		push    [ebp-0x08]
		call    eax //LoadLibraryExA("kalmagic.dll", 0, 0);

		//4. get Virtual protect
		//mov     eax, [ebp-0x2C]
		push    74636574h
		push    74726956h
		call    getFunctionSimple //call    getFunction //search for VirtualProtect in Kernel32
		mov     [ebp-0x20], eax

		//5. get RtlMoveMemory
		//mov     eax, [ebp-0x30]
		push    0x0079726F
		push    0x4D6C7452
		call    getFunctionSimple //call    getFunction
		mov     [ebp-0x28], eax

		//6. get RtlZeroMemory
		//mov     eax, [ebp-0x30]
		push    0x0079726F
		push    0x5A6C7452
		call    getFunctionSimple //call    getFunction
		mov     [ebp-0x24], eax

		/** START RESTORING DATA **/

		//6.5. virtualProtect on stack, make our code working >.>, i wish i wouldn't need to do this
		lea     ebx, [ebp-0x14] //lets use this temp..
		push    ebx         //adress of "old protection"
		push    0x40        //new protection -> PAGE_EXECUTE_READWRITE , maybe we need writecopy!
		push    0x04        //size 
		push    [ebp-0x34]  //lpAddress
		call    [ebp-0x20]  //VirtualProtect(....)

		//7. VirtualProtect on entry
		lea     ebx, [ebp-0x14]
		push    ebx         //adress of "old protection"
		push    0x40        //new protection -> PAGE_EXECUTE_READWRITE , maybe we need writecopy!
		push    0x5         //size 
		push    [ebp-0x04]  //lpAddress
		call    [ebp-0x20]  //VirtualProtect(....)

		//8. virtualProtect on backup
		lea     ebx, [ebp-0x18] 
		push    ebx         //adress of "old protection"
		push    0x40        //new protection -> PAGE_EXECUTE_READWRITE , maybe we need writecopy!
		push    [ebp-0x10]  //size 
		push    [ebp-0x1C]  //lpAddress
		call    [ebp-0x20]  //VirtualProtect(....)

		//14. jmp back to our stack fix !
		//push    [ebp-0x34]

		//13. virtualprotect reverse 
		push    0           //adress of "old protection", is not allowed to be 0 !
		push    [ebp-0x18]  //new protection -> PAGE_EXECUTE_READWRITE , maybe we need writecopy!
		push    [ebp-0x10]  //size 
		push    [ebp-0x1C]  //lpAddress*/
		push    [ebp-0x34]  //stack fix from 14.

		//12. virtualprotect reverse entry
		push    0           //adress of "old protection", is not allowed to be 0 !
		push    [ebp-0x14]  //new protection -> PAGE_EXECUTE_READWRITE , maybe we need writecopy!
		push    0x5         //size 
		push    [ebp-0x04]  //lpAddress
		push    [ebp-0x20]  //VirtualProtect(....) from 13.*/

		//11. zero backup
		mov     ebx, [ebp-0x10] //payload size
		add     ebx, 5          //size of backup + 5
		push    ebx
		push    [ebp-0x0C]      //backup offset
		push    [ebp-0x20]      //VirtualProtect(....) from 12.
		
		//10. step copy backup
		push    [ebp-0x10]   //payload size
		mov     ebx, [ebp-0x0C]
		add     ebx, 5
		push    ebx //adress of backup + 5
		push    [ebp-0x1C]  //payload offset
		push    [ebp-0x24]  //RtlZeroMemory from 11.

		//9. step copy entry
		push    0x05        //size
		push    [ebp-0x0C]  //backup
		push    [ebp-0x04]  //entry
		push    [ebp-0x28]  //RtlMoveMemory from 10.

		push    [ebp-0x28]  //RtlMoveMemory from 9.

		/*** FIX STACK START ***/
		//Remove custom pushed data !
		lea      eax, [ebp-0x38] //start , end = esp !
		lea      ebx, [ebp-0x04]
stack_fix_loop_start:
		// *ebx = *eax
		mov      ecx, [eax]
		mov      [ebx], ecx

		//check eax == esp
		cmp      eax, esp
		jz       stack_fix_loop_finish

		sub      eax, 0x04
		sub      ebx, 0x04
		jmp stack_fix_loop_start

stack_fix_loop_finish:
		add      esp, 0x34
		/*** FIX STACK END   ***/

		retn    //calls 9, 10, 11, 12, 13, 14
								
		//4 nopes end of function ! ;)
		__asm _emit 0x90
		__asm _emit 0x90
		__asm _emit 0x90
		__asm _emit 0x90
getOffsetHelper:
		pop eax
		retn
	}
}

void* get_payload()
{
	void* addr;
	__asm
	{
		call payload_asm
		mov addr, eax
	}
	return addr;
}

static void* result;
__declspec(naked) HANDLE payload_asm_test()
{
	__asm
	{
		pushad
		call payload_asm
		mov result, eax
		popad
		mov eax, result
		retn
	}
}

#include <stdio.h>
#include <algorithm>
__declspec(dllexport) HANDLE payload_c()
{
	//1. get kernel32 handle !
	HANDLE kernel32 = 0;	
	 __asm {
		mov eax, fs:[ 0x30 ]       // get a pointer to the PEB
		mov eax, [ eax + 0x0C ]    // get PEB->Ldr
		mov eax, [ eax + 0x14 ]    // get PEB->Ldr.InMemoryOrderModuleList.Flink (1st entry)
		mov eax, [ eax ]           // get the next entry (2nd entry)
		mov eax, [ eax ]           // get the next entry (3rd entry)
		mov eax, [ eax + 0x10 ]    // get the 3rd entries base address (kernel32.dll)
        mov kernel32, eax
    }

	//2. find load library
	unsigned long modb          = (unsigned long)kernel32;
    IMAGE_EXPORT_DIRECTORY *ied = (IMAGE_EXPORT_DIRECTORY *)(modb+((IMAGE_NT_HEADERS *)(modb+((IMAGE_DOS_HEADER *)modb)->e_lfanew))->OptionalHeader.DataDirectory->VirtualAddress);

	//check name
	//printf("NAME: %s\n", ied->Name + modb); //i think this should print name

	typedef void*  (WINAPI *LType)(const char* name, void*, void*);
	//LType load_library_addr = 0;

	const char** ptr =     (const char **)(ied->AddressOfNames+modb+0*sizeof(void *));
	const char** ptr_end = (const char **)(ied->AddressOfNames+modb+ied->NumberOfNames*sizeof(void *));
	while(ptr != ptr_end)
	{
		const char* nn = *ptr + modb;
		ptr += 1;

		int A = *((const int*)nn);
		int B = *((const int*)(nn+0x0A));
		int A_want = 0x64616F4C; 
		int B_want = 0x41784579;
        if((A == A_want) && (B == B_want)) //check for loadlibraryexA
		{
			//printf("%s\n", nn);
			return (HANDLE)((unsigned long)*(void **)(ied->AddressOfFunctions+modb+*(unsigned short *)(ied->AddressOfNameOrdinals+modb+(ied->NumberOfNames-std::distance(ptr, ptr_end))*sizeof(unsigned short))*sizeof(void *)-sizeof(void*))+modb);
        }
		/*printf("%s\n",nn);
		if (!strcmp(nn, "LoadLibraryExA"))
		{
			return 0;
		}*/
	}

	return 0;
}

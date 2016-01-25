#include <Winsock2.h>
#include <Windows.h>
#include <iostream>
#include <vector>
#include <mutex>
#include <iomanip>

#include "console.h"
#include "table_decrypt_encrypt.h"
#include "process_packet.h"
#include "aes.h"
#include "enviroment.h"
#include "send_my_packet.h"
#include "packets.h"
#include "handle_recieved_packets.h"

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

mutex mtx;

Bot* my_bot = Bot::instance();

// typedefs of original send and recv functions
typedef int(__stdcall f_orig_recv)(SOCKET, char*, int, int);
f_orig_recv *orig_recv = NULL;
typedef int(__stdcall f_orig_send)(SOCKET, char*, int, int);
f_orig_recv *orig_send = NULL;

int g_socket;
int g_flags;

namespace thread
{
	template<typename T> void critical(T f)
	{
		mtx.lock();
		try
		{
			f();
		}
		catch (...)
		{
			mtx.unlock();
			throw;
		}
		mtx.unlock();
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// SEND
/////////////////////////////////////////////////////////////////////////////////////
int WINAPI my_send(SOCKET socket, vector<char> *packet, int flags){
	int r, r_real = 0;
	thread::critical([&](){
		if (aes_set){
			encrypt_send_packet_aes(packet);
		}
		table_encode(send_key, packet);

		send_key = (send_key + 1) % 64;

		// send it
		do {
			if (packet->size() - r_real == 0){
				break;
			}
			r = orig_send(socket, packet->data() + r_real, packet->size() - r_real, flags);
			r_real += r;
		} while (r >= 0);
	});

	if (r < 0) {
		return r;
	}
	return r_real;
}


vector<char> cache_send;
int WINAPI my_send_hook(SOCKET socket, char* packet, int size, int flags){
	g_socket = socket;
	g_flags = flags;

	cache_send.insert(cache_send.end(), packet, packet + size);

	reader r(&cache_send.front());
	short packet_size;
	r.read(packet_size); // read the size
	if (packet_size <= cache_send.size()) {
		int return_val = process_send_packet(&cache_send);
		if (return_val != packet_size) {
			cout << "We are fucked" << endl;
			Sleep(10000);
			return -1;
		}
		send_key_engine = (send_key_engine + 1) % 64;
	}
	return size;
}


/////////////////////////////////////////////////////////////////////////////////////
// RECV
/////////////////////////////////////////////////////////////////////////////////////
vector<char> recv_cache;
vector<char> recv_for_thread;
int WINAPI my_recv(SOCKET socket, char *buffer, int length, int flags){
	int ret = orig_recv(socket, buffer, length, flags);
	if (ret > 0){
		// insert newly recieved data to the buffer
		recv_cache.insert(recv_cache.end(), buffer, buffer + ret);

		//check if packet is full
		if (recv_cache.size() > 2)
		{
			reader r(recv_cache.data());
			short packet_size;
			r.read(packet_size); // read the size
			if (packet_size <= recv_cache.size()) {
				//we got a full packet (or more !)
				vector<char> packet(recv_cache.begin(), recv_cache.begin() + packet_size);
				recv_cache.erase(recv_cache.begin(), recv_cache.begin() + packet_size);
				// DECRYPT
				table_decode(recv_key, &packet);
				if (aes_set){
					decrypt_recv_packet_aes(&packet);
				}
				// TYPE
				r = reader(packet.data() + 2);
				char type;
				r.read(type);
				if (type == S2C_CODE) {
					thread::critical([&](){
						//PORCESS 0x2A quickly here !
						handle_welcome_packet(&packet, r);
					});
				}
				else thread::critical([&]()
				{
					//send to thread for procession
					recv_for_thread.insert(recv_for_thread.begin(), packet.begin(), packet.end());
				});
			}
		}
	}

	return ret;
}

/////////////////////////////////////////////////////////////////////////////////////
// HOOK
/////////////////////////////////////////////////////////////////////////////////////

bool bDataCompare(const BYTE* pData, const BYTE* bMask, const char* szMask){
	for (; *szMask; ++szMask, ++pData, ++bMask)
	if (*szMask == 'x' && *pData != *bMask)
		return false;
	return (*szMask) == NULL;
}

DWORD dwFindPattern(DWORD dwAddress, DWORD dwLen, BYTE *bMask, char * szMask){
	for (DWORD i = 0; i<dwLen; i++){
		if (bDataCompare((BYTE*)(dwAddress + i), bMask, szMask)){
			return (DWORD)(dwAddress + i);
		}
	}
	return NULL;
}

void detour_send(){
	// my_recv address
	DWORD my_send_addr = (DWORD)&my_send_hook;

	// get address of the original_recv_pointer
	DWORD ptr_to_orig_send_ptr_addr;
	DWORD orig_send_ptr_addr;
	DWORD orig_send_addr;
	DWORD lookup_start_addr = 0x401000;
	DWORD real_send_address = (DWORD)GetProcAddress(GetModuleHandle(L"ws2_32.dll"), "send");
	while (1){
		ptr_to_orig_send_ptr_addr = dwFindPattern(lookup_start_addr, 0x2bc000, (BYTE*)"\x55\x8B\xEC\x83\xEC\x08\x89\x4D\xF8\x8B\x45\x14\x50\x8B\x4D\x10\x51\x8B\x55\x0C\x52\x8B\x45\x08\x50\xFF\x15", "xxxxxxxxxxxxxxxxxxxxxxxxxxx");
		ptr_to_orig_send_ptr_addr += 27; //add length of the pattern

		// address of the orig_recv containing pointer to the ws2_32.recv function
		orig_send_ptr_addr = *(DWORD*)ptr_to_orig_send_ptr_addr;

		// store orig_recv_address
		orig_send_addr = *(DWORD*)orig_send_ptr_addr;

		if (orig_send_addr == real_send_address){
			break;
		}
		lookup_start_addr = ptr_to_orig_send_ptr_addr + 27;
	}

	// overwrite orig_recv address to my_recv ptr
	DWORD OldProtection;
	VirtualProtect((LPVOID)orig_send_ptr_addr, sizeof(DWORD), PAGE_READWRITE, &OldProtection);
	*(DWORD*)orig_send_ptr_addr = my_send_addr;
	VirtualProtect((LPVOID)orig_send_ptr_addr, sizeof(DWORD), OldProtection, &OldProtection);

	// create pointer to orig_recv
	orig_send = (f_orig_send *)orig_send_addr;

	cout << "SEND_DETOUR: initialised" << endl;

}

void detour_recv(){
	// my_recv address
	DWORD my_recv_addr = (DWORD)&my_recv;

	// get address of the original_recv_pointer
	DWORD ptr_to_orig_recv_ptr_addr;
	DWORD orig_recv_ptr_addr;
	DWORD orig_recv_addr;
	DWORD lookup_start_addr = 0x401000;
	DWORD real_recv_address = (DWORD)GetProcAddress(GetModuleHandle(L"ws2_32.dll"), "recv");
	while (1){
		ptr_to_orig_recv_ptr_addr = dwFindPattern(lookup_start_addr, 0x2bc000, (BYTE*)"\x55\x8B\xEC\x83\xEC\x08\x89\x4D\xF8\x8B\x45\x14\x50\x8B\x4D\x10\x51\x8B\x55\x0C\x52\x8B\x45\x08\x50\xFF\x15", "xxxxxxxxxxxxxxxxxxxxxxxxxxx");
		ptr_to_orig_recv_ptr_addr += 27; //add length of the pattern

		// address of the orig_recv containing pointer to the ws2_32.recv function
		orig_recv_ptr_addr = *(DWORD*)ptr_to_orig_recv_ptr_addr;

		// store orig_recv_address
		orig_recv_addr = *(DWORD*)orig_recv_ptr_addr;

		if (orig_recv_addr == real_recv_address){
			break;
		}
		lookup_start_addr = ptr_to_orig_recv_ptr_addr + 27;
	}

	// overwrite orig_recv address to my_recv ptr
	DWORD OldProtection;
	VirtualProtect((LPVOID)orig_recv_ptr_addr, sizeof(DWORD), PAGE_READWRITE, &OldProtection);
	*(DWORD*)orig_recv_ptr_addr = my_recv_addr;
	VirtualProtect((LPVOID)orig_recv_ptr_addr, sizeof(DWORD), OldProtection, &OldProtection);

	// create pointer to orig_recv
	orig_recv = (f_orig_recv *)orig_recv_addr;

	cout << "RECV_DETOUR: initialised" << endl;
}

DWORD WINAPI MedThread(LPVOID) {
	byte skill;
	for (;;){
		// this should be done regardles of the state
		if (my_bot->state != E_STATE_STOPPED){
			if (my_bot->HP <= 1400){
				if (GetTickCount() - my_bot->last_med_time >= 100){
					cout << "USED MEDICINE.." << endl;
					use_medium_medicine();
					my_bot->last_med_time = GetTickCount();
				}
				if (my_bot->HP <= 0){
					my_bot->state = E_STATE_STOPPED;
					cout << "DEATH!!!!!!!!! STOPPING!!!!!!!!!" << endl;
				}
			}
			// if attacking for too long, find another enemy..
			if (GetTickCount() - my_bot->mob_target_time >= 60000){
				my_bot->state = E_STATE_NOENEMY;
				if (my_bot->target_ID != -1 && my_bot->state != E_STATE_NOENEMY){
					cout << "attacking the enemy ID:" << my_bot->target_ID << " for too long .. possibly a bug -> removing from enemy list..." << endl;
					my_bot->monsters.erase(my_bot->target_ID);
				}
			}
			skill = my_bot->get_buff_skill();
			if (skill){
				send_buff_skill(skill);
				my_bot->skills[skill].used();
				cout << "USED BUFF SKILL ID:" << skill << endl;
			}
		}

		Sleep(50);
	}
	return 1;
}

DWORD WINAPI MainThread(LPVOID) {
	Sleep(1000); // wait for the Armadillo to unpack

	init_console();
	detour_recv();
	detour_send();

	// async wait for received packets
	vector< vector<char> > packet;
	vector< vector<char> > packet_backup;
	int size;
	for (;;){
		thread::critical([&](){
			//extract as much as possible !
			do
			{
				size = check_for_packet(&recv_for_thread);
				if (size != -1){
					packet.push_back(extract_packet(&recv_for_thread, size));
				}
			} while (size != -1);
		});

		//if packet was red, process:
		//process as much as possible !
		while (!packet.empty()){
			process_recv_packet(&(packet.front()));
			packet.erase(packet.begin());
		}

		// do the cleanup before everything runs :)
		my_bot->cleanup();

		// bot stuff	
		if (my_bot->state == E_STATE_NOENEMY){
			cout << dec << "bot: HP: " << my_bot->HP << "/" << my_bot->max_HP << "; MP: " << my_bot->MP << "/" << my_bot->max_MP << ", ";
			cout << my_bot->players.size() << " players around" << endl;
			my_bot->target_ID = my_bot->get_centered_monster_id(128.0);

			if (my_bot->target_ID){
				cout << "selected enemy: " << my_bot->target_ID << endl;
				my_bot->state = E_STATE_MOVING;
				cout << "moving move" << endl;
				// set mob target time
				my_bot->mob_target_time = GetTickCount();
			}

			if (!my_bot->is_target_valid(my_bot->target_ID)){
				my_bot->state = E_STATE_NOENEMY;
			}

		}
		else if (my_bot->state == E_STATE_MOVING){
			if (!my_bot->is_target_valid(my_bot->target_ID)){
				cout << "moving validation failed" << endl;
				my_bot->state = E_STATE_NOENEMY;
			}
			else{
				auto mob = my_bot->monsters.find(my_bot->target_ID);
				if (mob != my_bot->monsters.end()){
					if (my_bot->nobody_is_around()){
						send_fast_move_to_loc(mob->second.X, mob->second.Y, 50);
					}
					else{
						send_pl_move_to_loc(mob->second.X, mob->second.Y, 50);
					}
				}
			}
		}
		else if (my_bot->state == E_STATE_MOVE_STOPPED){
			if (!my_bot->is_target_valid(my_bot->target_ID)){
				cout << "move stopped validation failed" << endl;
				my_bot->state = E_STATE_NOENEMY;
			}
			else if (my_bot->get_dist_to_monster(my_bot->target_ID) <= 100){
				cout << "move stopped strating to attack" << endl;
				my_bot->state = E_STATE_ATTACKING;
			}
			else{
				cout << "move stopped, enemy too far, starting move again" << endl;
				my_bot->state = E_STATE_MOVING;
			}
		}
		else if (my_bot->state == E_STATE_ATTACKING){
			if (!my_bot->is_target_valid(my_bot->target_ID)){
				cout << "attacking validation failed" << endl;
				my_bot->state = E_STATE_NOENEMY;
			}
			else if (my_bot->get_dist_to_monster(my_bot->target_ID) > 100){
				cout << "attacking target too far, moving" << endl;
				my_bot->state = E_STATE_MOVING;
			}
			// fire!!! if nobody is around use multiple spells
			else if (my_bot->nobody_is_around()){
				if (my_bot->skills[23].ready()){
					send_pl_skill(my_bot->target_ID, my_bot->skills[23].animation_time, 23);
					my_bot->skills[23].used();
					//cout << "sent lightning blow attack on monster ID: " << my_bot->target_ID << ", dist: " << my_bot->get_dist_to_monster(my_bot->target_ID) << endl;
				}
				else if (my_bot->skills[3].ready()){
					send_pl_skill(my_bot->target_ID, my_bot->skills[4].animation_time, 3);
					my_bot->skills[3].used();
					//cout << "sent ice attack on monster ID: " << my_bot->target_ID << ", dist: " << my_bot->get_dist_to_monster(my_bot->target_ID) << endl;
				}
				else if (my_bot->skills[2].ready()){
					send_pl_skill(my_bot->target_ID, my_bot->skills[2].animation_time, 2);
					my_bot->skills[2].used();
					//cout << "sent fire attack on monster ID: " << my_bot->target_ID << ", dist: " << my_bot->get_dist_to_monster(my_bot->target_ID) << endl;
				}
				else if (my_bot->skills[4].ready()){
					send_pl_skill(my_bot->target_ID, my_bot->skills[4].animation_time, 4);
					my_bot->skills[4].used();
					//cout << "sent lightning attack on monster ID: " << my_bot->target_ID << ", dist: " << my_bot->get_dist_to_monster(my_bot->target_ID) << endl;
				}
			}
			else {
				if (my_bot->skills[23].ready()){
					send_pl_skill(my_bot->target_ID, my_bot->skills[23].animation_time, 23);
					my_bot->skills[23].used();
					//cout << "sent lightning attack on monster ID: " << my_bot->target_ID << ", dist: " << my_bot->get_dist_to_monster(my_bot->target_ID) << endl;
				}
			}
		}
		else if (my_bot->state == E_STATE_KILLED_BEHEADABLE_MONSTER){
			if (!my_bot->is_target_valid(my_bot->target_ID)){
				cout << "killed beheadable monster failed" << endl;
				my_bot->state = E_STATE_NOENEMY;
			}
			if (my_bot->nobody_is_around()){
				send_pl_skill_no_pre(my_bot->target_ID, 1);
			}
			else {
				Sleep(100);
				send_pl_skill_no_pre(my_bot->target_ID, 1);
			}
			my_bot->state = E_STATE_MONSTER_BEHEADED;
			cout << "monster beheaded" << endl;
		}
		else if (my_bot->state == E_STATE_KILLED_MONSTER){
			if (my_bot->nobody_is_around()){
				send_fast_pick_all_items();
			}
			else {
				Sleep(100);
				send_pl_pick_all_items();
			}

			cout << "monster killed! :)" << endl;
			my_bot->state = E_STATE_NOENEMY;
		}


		//anti 100% cpu usage
		Sleep(1);
	}
	return 1;
}

BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	if (fdwReason == DLL_PROCESS_ATTACH) {
		Sleep(1000);
		MessageBoxA(NULL, "Attached...", "BOT", 0);
		CreateThread(NULL, 0, MainThread, NULL, 0, NULL);
		CreateThread(NULL, 0, MedThread, NULL, 0, NULL);
	}
	else if (fdwReason == DLL_PROCESS_DETACH){
		MessageBoxA(NULL, "Detached...", "BOT", 0);
	}

	else{
		//cout << "nothing..." << endl;
	}
	return 1;
}
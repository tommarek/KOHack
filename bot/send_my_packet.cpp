#include <Windows.h>
#include <vector>
#include <iostream>

#include "enviroment.h"
#include "main.h"
#include "packets.h"
#include "process_packet.h"
#include "send_my_packet.h"

using namespace std;

// login info - will be moved elsewhere
string out_login("XXXXXXXXX");
string out_pass("XXXXXXXXX");
string out_pass2("XXXXXXXXX");
// end of login info

int send_login_packet(){
	vector<char> out_packet;
	byte out_type;
	WORD out_size = 0;

	out_type = C2S_LOGIN;
	write(out_packet, out_size);
	write(out_packet, out_type);
	write(out_packet, U);
	write(out_packet, out_login);
	write(out_packet, out_pass);
	update_packet_size(out_packet);

	// send login packet
	return my_send(g_socket, &out_packet, g_flags);
}

int send_2nd_password(){
	byte out_type = 0x75; // C2S_PLAYER_SUMMON
	byte param = 0; // pin_ok .. other types may be pin_no and pin_set
	WORD out_size = 0;

	vector<char> out_packet;
	write(out_packet, out_size);
	write(out_packet, out_type);
	write(out_packet, U);
	write(out_packet, param);
	write(out_packet, out_pass2);
	update_packet_size(out_packet);

	// send 2nd pass packet
	return my_send(g_socket, &out_packet, g_flags);
}

int send_character_select(int id){
	byte out_type = C2S_LOADPLAYER;
	DWORD p1 = 0, p2 = 1;
	WORD out_size = 0;

	vector<char> out_packet;
	write(out_packet, out_size);
	write(out_packet, out_type);
	write(out_packet, U);
	write(out_packet, id);
	write(out_packet, p1);
	write(out_packet, p2);
	update_packet_size(out_packet);

	// send character select packet
	return my_send(g_socket, &out_packet, g_flags);
}

// ATTACK
void _send_attack_monster(int monster_id){
	Bot* my_bot = Bot::instance();
	byte out_type = C2S_ATTACK, something1 = 1;
	WORD out_size = 0;

	vector<char> out_packet;
	write(out_packet, out_size);
	write(out_packet, out_type);
	write(out_packet, U);
	write(out_packet, something1);
	write(out_packet, monster_id);
	write(out_packet, my_bot->Z);
	update_packet_size(out_packet);

	my_send(g_socket, &out_packet, g_flags);
}

void send_attack_all_monsters_attacking_me(){
	Bot* my_bot = Bot::instance();
	if (GetTickCount() - my_bot->last_attack_time > 600){
		for each(auto monster in my_bot->monsters){
			if (monster.second.target_ID == my_bot->ID){
				_send_attack_monster(monster.second.target_ID);
			}
		}
		my_bot->last_attack_time = GetTickCount();
	}
}

void send_pl_attack_monster(int monster_id){
	Bot* my_bot = Bot::instance();
	if (GetTickCount() - my_bot->last_attack_time > 600){
		_send_attack_monster(monster_id);
		my_bot->last_attack_time = GetTickCount();
	}
}

void _send_preskill(int monster_id, int skill_id){
	vector<char> out_packet;
	WORD out_size = 0;
	byte out_type;

	out_size = 0;
	out_type = C2S_PRESKILL;
	write(out_packet, out_size);
	write(out_packet, out_type);
	write(out_packet, U);
	write(out_packet, (byte)skill_id);
	write(out_packet, monster_id);
	update_packet_size(out_packet);
	my_send(g_socket, &out_packet, g_flags);
}

void _send_skill(int monster_id, int skill_id){
	vector<char> out_packet;
	WORD out_size = 0;
	byte out_type;

	out_type = C2S_SKILL;
	out_size = 0;
	write(out_packet, out_size);
	write(out_packet, out_type);
	write(out_packet, U);
	write(out_packet, (byte)skill_id);
	write(out_packet, (byte)1);
	write(out_packet, monster_id);
	update_packet_size(out_packet);
	my_send(g_socket, &out_packet, g_flags);
}

void send_pl_skill(int monster_id, int animation_time, int skill_id){
	_send_preskill(monster_id, skill_id);
	Sleep(animation_time);
	_send_skill(monster_id, skill_id);
}

void send_pl_skill_no_pre(int monster_id, int skill_id){
	_send_skill(monster_id, skill_id);
}

void send_fast_skill(int monster_id, int skill_id){
	_send_skill(monster_id, skill_id);
}

void send_buff_skill(int skill_id){
	vector<char> out_packet;
	WORD out_size = 0;
	byte out_type;

	out_type = C2S_SKILL;
	out_size = 0;
	write(out_packet, out_size);
	write(out_packet, out_type);
	write(out_packet, U);
	write(out_packet, (byte)skill_id);
	update_packet_size(out_packet);
	my_send(g_socket, &out_packet, g_flags);
}

// USE ITEM
void send_use_item(int item_id){
	Bot* my_bot = Bot::instance();
	byte out_type = C2S_USEITEM;
	WORD out_size = 0;
	vector<char> out_packet;
	write(out_packet, out_size);
	write(out_packet, out_type);
	write(out_packet, U);
	write(out_packet, item_id);
	my_send(g_socket, &out_packet, g_flags);
}

// PICK
void send_pick_item(int id, int X, int Y){
	Bot* my_bot = Bot::instance();
	byte out_type = C2S_PICKUPITEM;
	WORD out_size = 0;
	
	vector<char> out_packet;
	write(out_packet, out_size);
	write(out_packet, out_type);
	write(out_packet, U);
	write(out_packet, (DWORD)id);
	write(out_packet, (DWORD)X / 32);
	write(out_packet, (DWORD)Y / 32);
	update_packet_size(out_packet);
	my_send(g_socket, &out_packet, g_flags);
}

void send_fast_pick_all_items(){
	Bot* my_bot = Bot::instance();
	for each(auto item in my_bot->items){
		double distance = my_bot->get_distance(item.second.X, item.second.Y);
		// move to the item if it is too far
		if (distance < 300){
			send_pick_item(item.second.ID, item.second.X, item.second.Y);
		}
	}
}

void send_pl_pick_all_items(){
	Bot* my_bot = Bot::instance();
	for each(auto item in my_bot->items){
		double distance = my_bot->get_distance(item.second.X, item.second.Y);
		if (distance < 300){
			// pick the item
			send_pick_item(item.second.ID, item.second.X, item.second.Y);
			// sleep so it looks more like a player
			Sleep(500);
		}
	}
}

// REST
void _send_rest(byte param){
	byte out_type = C2S_REST;
	WORD out_size = 0;

	vector<char> out_packet;
	write(out_packet, out_size);
	write(out_packet, out_type);
	write(out_packet, U);
	write(out_packet, (byte)param);
	update_packet_size(out_packet);
	my_send(g_socket, &out_packet, g_flags);
}

void send_rest_start(){
	_send_rest(1);
}

void send_rest_stop(){
	_send_rest(0);
}

// USE ITEM
void use_item(int item_id){
	byte out_type = C2S_USEITEM;
	WORD out_size = 0;

	vector<char> out_packet;
	write(out_packet, out_size);
	write(out_packet, out_type);
	write(out_packet, U);
	write(out_packet, (DWORD)item_id);
	update_packet_size(out_packet);
	my_send(g_socket, &out_packet, g_flags);	
}

void use_medium_medicine(){
	use_item(0x8d7c3443);
}

// MOVEMENT
void _send_move_to_loc(int X, int Y, int tick_dist, int ending_distance){
	Bot* my_bot = Bot::instance();
	int dX, dY, dZ;

	// packet fields
	byte out_type;
	WORD out_size = 0;
	vector<char> out_packet;

	// calculate the direction vector
	int dir_vec[] = { X - my_bot->X, Y - my_bot->Y };

	// calculate distances to target and real stopping point
	double dist_to_target = my_bot->get_distance(X, Y);
	double dist_to_end = dist_to_target - ending_distance;

	if (dist_to_end > tick_dist){
		dist_to_end = tick_dist;
		out_type = C2S_MOVE_ON;
	}
	else {
		out_type = C2S_MOVE_END;
		my_bot->state = E_STATE_MOVE_STOPPED;
	}

	dX = (int)((dir_vec[0] / dist_to_target) * dist_to_end);
	dY = (int)((dir_vec[1] / dist_to_target) * dist_to_end);

	// check if dX and dY are not too high
	if (dX < -60) dX = -60;
	else if (dX > 60) dX = 60;
	if (dY < -60) dY = -60;
	else if (dY > 60) dY = 60;

	// update bot X and Y coords
	my_bot->X += dX;
	my_bot->Y += dY;

	// calculate Z coordinate
	int new_Z = my_bot->heightmap.get_height(my_bot->X, my_bot->Y);
	dZ = new_Z - my_bot->Z;
	if (dZ < -60) dZ = -60;
	else if (dZ > 60) dZ = 60;
	my_bot->Z += dZ;

	write(out_packet, out_size);
	write(out_packet, out_type);
	write(out_packet, U);
	write(out_packet, (signed char)dX);
	write(out_packet, (signed char)dY);
	write(out_packet, (signed char)dZ);
	update_packet_size(out_packet);

	my_send(g_socket, &out_packet, g_flags);
}

void send_pl_move_to_loc(int X, int Y, int ending_distance){
	Bot* my_bot = Bot::instance();
	if (GetTickCount() - my_bot->last_move_time > 400){ // only send move packet once in 400 ms
		_send_move_to_loc(X, Y, 15, ending_distance); // just 15 meters per packet
		my_bot->last_move_time = GetTickCount();
	}
}

void send_fast_move_to_loc(int X, int Y, int ending_distance){
	Bot* my_bot = Bot::instance();
	if (GetTickCount() - my_bot->last_move_time > 200){ // only send move packet once in 100 ms
		_send_move_to_loc(X, Y, 40, ending_distance); // just 15 meters per packet
		my_bot->last_move_time = GetTickCount();
	}
}

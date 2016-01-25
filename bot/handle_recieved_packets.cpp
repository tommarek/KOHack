#include <Windows.h>
#include <vector>
#include <array>
#include <iostream>

#include "enviroment.h"
#include "process_packet.h"
#include "main.h"
#include "aes.h"
#include "send_my_packet.h"

#include "handle_recieved_packets.h"

using namespace std;


void handle_welcome_packet(vector<char> *packet, reader r){ // 0x2A
	// read te packet
	DWORD protocol_version, timestamp, time_start, game_system, game_event, unknown, appender;
	char table_key, server, age, country;
	array<char, 52> crypted_key;
	r.read(protocol_version);
	r.read(table_key);
	r.read(timestamp);
	r.read(time_start);
	r.read(game_system);
	r.read(game_event);
	r.read(unknown);
	r.read(server);
	r.read(age);
	r.read(country);
	r.read(crypted_key);
	r.read(appender);

	// set table key
	recv_key = (int)table_key;
	send_key = (int)table_key;
	send_key_engine = (int)table_key;

	// generate AES keys
	aes_generate_keys(crypted_key);

	// update u
	set_U(appender);
}


// APPEAR / DISAPPEAR
void handle_monster_appeared_packet(vector<char> *packet, reader r){ // 0x33
	Bot* my_bot = Bot::instance();
	WORD wCodeNo;
	DWORD ID, X, Y, Dir, HP, max_HP;

	r.read(wCodeNo);
	r.read(ID);
	r.read(X);
	r.read(Y);
	r.read(Dir);
	r.read(HP);
	r.read(max_HP);
	if (HP == 0) { // wooden box
		return;
	}
	else {
		Monster monster(ID, X, Y, 0, HP, max_HP);
		my_bot->monsters[ID] = monster;
	}
}

void handle_monster_disappeared_packet(vector<char> *packet, reader r){ // 0x38
	Bot* my_bot = Bot::instance();
	DWORD ID;
	r.read(ID);
	my_bot->monsters.erase(ID);
}

void handle_player_appeared_packet(vector<char> *packet, reader r){ // 0x32
	Bot* my_bot = Bot::instance();
	DWORD ID, X, Y, Z, dir;
	string name;
	byte job;
	r.read(ID);
	r.read(name);
	r.read(job);
	r.read(X);
	r.read(Y);
	r.read(Z);
	r.read(dir);
	// my bot appeared
	if (my_bot->ID == -1){
		my_bot->ID = ID;
		my_bot->X = X;
		my_bot->Y = Y;
		my_bot->Z = Z;
		cout << dec << "my bot ID: " << ID << ", coords: [" << X << ", " << Y << ", " << Z << "], dir =" << dir << endl;
	}
	// other players appeared
	else{
		Player player(ID, X, Y, Z, name);
		my_bot->players[ID] = player;
		//cout << "player appeared" << endl;
	}
}

void handle_player_disappeared_packet(vector<char> *packet, reader r){ // 0x37
	Bot* my_bot = Bot::instance();
	DWORD ID;
	r.read(ID);
	my_bot->players.erase(ID);
	//cout << "player disappeared" << endl;
}

void handle_item_appeared_packet(vector<char> *packet, reader r){ // 0x32
	Bot* my_bot = Bot::instance();
	DWORD ID, X, Y, nNum;
	WORD wItemIndex;
	r.read(wItemIndex);
	r.read(ID);
	r.read(X);
	r.read(Y);
	r.read(nNum);
	Item item(ID, X, Y);
	my_bot->items[ID] = item;
}

void handle_item_disappeared_packet(vector<char> *packet, reader r){ // 0x37
	Bot* my_bot = Bot::instance();
	DWORD ID;
	r.read(ID);
	my_bot->items.erase(ID);
}

// movement
void handle_move_monster(vector<char> *packet, reader r){
	Bot* my_bot = Bot::instance();
	DWORD ID;
	signed char x, y, state;
	r.read(ID);
	r.read(x);
	r.read(y);
	r.read(state);

	// warining
	if (my_bot->monsters.count(ID) == 0){
		cout << "WARNING: moving monster " << ID << "but there is no such monster" << endl;
	}
	else{
		my_bot->monsters[ID].X += x;
		my_bot->monsters[ID].Y += y;
	}
}

void handle_move_player(vector<char> *packet, reader r){
	Bot* my_bot = Bot::instance();
	DWORD ID;
	signed char x, y, state;
	r.read(ID);
	r.read(x);
	r.read(y);
	r.read(state);

	// warining
	if (my_bot->players.count(ID) == 0){
		cout << "WARNING: moving player " << ID << "but there is no such player" << endl;
	}
	else {
		my_bot->players[ID].X += x;
		my_bot->players[ID].Y += y;
	}
}

// HP/MP update
void process_update_property(vector<char> *packet, reader r){ // 0x43
	Bot* my_bot = Bot::instance();
	byte type;
	WORD value1, value2;

	r.read(type);

	if (type == 5){ // cur_HP, max_HP, 
		r.read(value1);
		r.read(value2);
		my_bot->HP = value1;
		my_bot->max_HP = value2;
	}
	else if (type == 6){ // cur_MP, max_MP
		r.read(value1);
		r.read(value2);
		my_bot->MP = value1;
		my_bot->max_MP = value2;
	}
	else if (type == 7){ // cur_HP
		r.read(value1);
		my_bot->HP = value1;
	}
	else if (type == 8){ // cur_MP
		r.read(value1);
		my_bot->MP = value1;
	}
}

void handle_action_packet(vector<char> *packet, reader r){ // 0x3d
	Bot* my_bot = Bot::instance();
	DWORD ID;
	byte action;
	r.read(ID);
	r.read(action);

	// change state and remove from monster list
	if (action == 0x08){ // beheadable state
		if (ID == my_bot->target_ID){
			my_bot->state = E_STATE_KILLED_BEHEADABLE_MONSTER;
			cout << "got packet with behead of my target" << endl;
		}
	}
	else if (action == 0x09){ // normal death -> monster is removed from the list
		if (ID == my_bot->target_ID){
			my_bot->state = E_STATE_KILLED_MONSTER;
		}
		// remove the monster from the list
		my_bot->monsters.erase(ID);
	}
	else if (action == 0x0a){ // after beheading disappear packet
		if (ID == my_bot->target_ID){
			my_bot->state = E_STATE_KILLED_MONSTER;
		}
		// remove the monster from the list
		my_bot->monsters.erase(ID);
	}
}

// handle other players/monsters attacking
void handle_attack_packet(vector<char> *packet, reader r){ // 0x3e
	Bot* my_bot = Bot::instance();
	DWORD attacker, target;
	WORD wCurHP, wCurHP_ADD;

	r.read(attacker);
	r.read(target);

	// if mob is an attacker
	auto mob = my_bot->monsters.find(attacker);
	if (mob != my_bot->monsters.end()){
		mob->second.target_ID = target;
	}
	// if mob is attacked
	mob = my_bot->monsters.find(target);
	if (mob != my_bot->monsters.end()){
		mob->second.target_ID = attacker;
	}

	// hp collection if mob is attacking me
	if (target == my_bot->ID){
		r.read(wCurHP);
		r.read(wCurHP_ADD);

		my_bot->HP -= wCurHP;
		my_bot->HP += wCurHP_ADD;
	}

}

// TELEPORT PACKET
void handle_teleport_packet(vector<char> *packet, reader r){
	Bot* my_bot = Bot::instance();
	byte byMap, cheat;
	DWORD X, Y, nY;

	r.read(byMap);
	r.read(X);
	r.read(Y);
	r.read(nY);
	r.read(cheat);

	cout << "got teleport packet!!!!!!!!!!!! bot is stopping!!!!!!!!!!!!" << endl;
	my_bot->X = X;
	my_bot->Y = Y;
	my_bot->state = E_STATE_STOPPED;

}


// administrative :-)
void handle_u_update(vector<char> *packet, reader r){ // 0x6b
	DWORD appender;
	r.read(appender);
	set_U(appender);
}

void handle_first_pass_ok(vector<char> *packet, reader r){ // 0x2B
	send_2nd_password();
}

void handle_second_pass_ok(vector<char> *packet, reader r){ // 0x2B
	cout << "2nd pass - OK - choose character" << endl;

}
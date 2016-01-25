#include <Windows.h>
#include <vector>
#include <array>
#include <iostream>
#include <sstream>
#include <string>

#include "enviroment.h"
#include "process_packet.h"
#include "main.h"
#include "aes.h"
#include "handle_recieved_packets.h"
#include "send_my_packet.h"

using namespace std;

void handle_check_version_packet(vector<char> packet, reader r){
	// init maps in bot
	Bot* my_bot = Bot::instance();
	my_bot->setup();
	
	//read U
	r.read(U);

	// send login
	send_login_packet();
	//cout << "sent login packet" << endl;
}

void handle_load_player(vector<char> packet, reader r){
	DWORD player_ID, p1, p2;
	r.read(U);
	r.read(player_ID);
	r.read(p1);
	r.read(p2);
	//cout << "Selected player ID = " << player_ID << endl;
}

void handle_attack(vector<char> packet, reader r){
	DWORD monster_ID, p2;
	byte p1;
	r.read(U);
	r.read(p1);
	r.read(monster_ID);
	r.read(p2);
	cout << dec << "sent: Attack, p1=" << (int)p1 << ", monster_id=" << (int)monster_ID << ", p3=" << (int)p2 << "time: " << GetTickCount() << endl;
}

void handle_preskill(vector<char> packet, reader r){
	DWORD monster_id;
	byte skill_id;
	r.read(U);
	r.read(skill_id);
	r.read(monster_id);
	cout << dec << "sent: Preskill with skill_id = " << (int)skill_id << ", monster_id = " << (int)monster_id << "time: " << GetTickCount() << endl;

}

void handle_skill_attack(vector<char> packet, reader r){
	DWORD monster_ID;
	byte skill_ID, p1;
	r.read(U);
	r.read(skill_ID);
	r.read(p1);
	r.read(monster_ID);
	cout << dec << "sent: Attack with Skill: skill_id=" << (int)skill_ID << ", p1 = " << (int)p1 << ", monster_id = " << (int)monster_ID << "time: " << GetTickCount() << endl;
}

void handle_move_packet(vector<char> packet, reader r){
	Bot* my_bot = Bot::instance();
	signed char dX, dY, dZ;
	r.read(U);
	r.read(dX);
	r.read(dY);
	r.read(dZ);
	my_bot->X += dX;
	my_bot->Y += dY;
	my_bot->Z += dZ;
	cout << "sent move packet" << endl;
}

void handle_move_end_packet(vector<char> packet, reader r){
	Bot* my_bot = Bot::instance();
	signed char dX, dY, dZ;
	r.read(U);
	r.read(dX);
	r.read(dY);
	r.read(dZ);
	my_bot->X += dX;
	my_bot->Y += dY;
	my_bot->Z += dZ;
	cout << "sent move end packet" << endl;
}


void handle_rest_packet(vector<char> packet, reader r){
	byte param;

	r.read(U);
	r.read(param);
	
	cout << "Rest packet sent with param: " << dec << (int)param << endl;
}

void handle_use_item_packet(vector<char> packet, reader r){
	DWORD param;

	r.read(U);
	r.read(param);

	cout << "used item ID: " << hex << (int)param << endl;
}


void handle_sent_chat_message(vector<char> packet, reader r){
	Bot* my_bot = Bot::instance();
	string message;

	r.read(U);
	r.read(message);

	// create stream with message
	stringstream stream;
	stream.str(message);

	// process
	string command;
	stream >> command;
	if (command == "/start"){
		my_bot->state = E_STATE_NOENEMY;

		// store star coords to save starting coords
		my_bot->start_X = my_bot->X;
		my_bot->start_Y = my_bot->Y;

		//GET FIRST PARAMETER
		//int whatever;
		//stream >> whatever;
		//cout << "Started with parameter " << whatever << endl;
	}
	else if (command == "/stop"){
		my_bot->state = E_STATE_STOPPED;
	}
	else if (command == "/monsters"){
		cout << "MONSTERS:" << endl;
		for each(auto mob in my_bot->monsters){
			cout << "Mob ID:" << mob.second.ID << ", coords: [" << mob.second.X << ", " << mob.second.Y << "]" << endl;
		}
	}
	else if (command == "/players"){
		cout << "PLAYERS:" << endl;
		for each(auto player in my_bot->players){
			cout << "Player ID:" << player.second.ID << ", coords: [" << player.second.X << ", " << player.second.Y << "] name: " << player.second.name << endl;
		}
	}
	else if (command == "/set"){
		string command2;
		stream >> command2;

		if (command2 == "center"){
			// store star coords to save starting coords
			my_bot->start_X = my_bot->X;
			my_bot->start_Y = my_bot->Y;

			cout << "NEW CENTER SET TO bot current position: [" << my_bot->X << ", " << my_bot->Y << "]" << endl;
		}
	}
	else if (command == "/pick"){
		send_fast_pick_all_items();
	}
}


void handle_other_sent_packet(vector<char> packet, reader r){
	r.read(U);
}
#include <Windows.h>
#include <list>
#include <fstream>
#include <iostream>
#include <map>
#include <array>
#include <cmath>

#include "enviroment.h"

using namespace std;

/////////////////////////////////////////////////////////////////////////////////////
// Skill class 
/////////////////////////////////////////////////////////////////////////////////////
Skill::Skill(){};
Skill::Skill(byte skill_ID, int skill_cooldown, int skill_anim_time, int skill_mana, int skill_type){
	ID = skill_ID;
	cooldown = skill_cooldown; // [ms] server skill cooldown
	animation_time = skill_anim_time; // [ms] length of a skill animation
	mana_cost = skill_mana;
	type = skill_type;

	last_time_used = 0;
}

bool Skill::ready(){
	int now = GetTickCount();

	if (now - last_time_used > cooldown){
		return true;
	}
	return false;
}


void Skill::used(){
	last_time_used = GetTickCount();
}


/////////////////////////////////////////////////////////////////////////////////////
// Map class 
/////////////////////////////////////////////////////////////////////////////////////
Map::Map(){
}

int Map::get_height(int player_x, int player_y)
{
	string file = "kalonline.hmap";

	static int c_x = -1;
	static int c_y = -1;
	static array<unsigned short, 257 * 257> c_data;

	int map_x = player_x / 32 / 256;
	int map_y = player_y / 32 / 256;

	if ((c_x != map_x) || (c_y != map_y)) {
		ifstream f(file.c_str(), ios::in | ios::binary);
		if (f.is_open()) {
			while (f.good()) {
				f.read((char*)&c_x, sizeof(c_x));
				f.read((char*)&c_y, sizeof(c_y));
				f.read((char*)c_data.data(), c_data.size()*sizeof(unsigned short));

				if ((c_x == map_x) && (c_y == map_y))  break;
			}
		}
		else {
			cout << "File not found" << endl;
			return -1;
		}
	}

	if ((c_x == map_x) && (c_y == map_y)) {
		player_x -= map_x * 32 * 256;
		player_y -= map_y * 32 * 256;

		double x = player_x / 32.0;
		double y = player_y / 32.0;

		double ceil_x = ceil(x);
		double floor_x = floor(x);
		double pos_x = x - floor_x;

		double ceil_y = ceil(y);
		double floor_y = floor(y);
		double pos_y = y - floor_y;

		int pixel[2][2];
		try	{
			pixel[0][0] = c_data.at((int)floor_x + (int)floor_y * 257) * 10;
			pixel[0][1] = c_data.at((int)floor_x + (int)ceil_y * 257) * 10;
			pixel[1][0] = c_data.at((int)ceil_x + (int)floor_y * 257) * 10;
			pixel[1][1] = c_data.at((int)ceil_x + (int)ceil_y * 257) * 10;
		}
		catch (...)
		{
			cout << "Map logic error.." << endl;
			throw string("Map logic error..");
		}
		
		// interpolation in X
		double temp_1 = linear_interpolation(pixel[0][0], pixel[1][0], pos_x);
		double temp_2 = linear_interpolation(pixel[0][1], pixel[1][1], pos_x);

		// interpolation in Y
		double height = linear_interpolation(temp_1, temp_2, pos_y);
		return (int)height;
	}
	else {
		cout << "Map-cell not found" << endl;
		return -1;
	}

	return -1;
}

double Map::linear_interpolation(double p1, double p2, double position){
	return p1 * (1 - position) + p2 * position;
}


/////////////////////////////////////////////////////////////////////////////////////
// Entity bot class 
/////////////////////////////////////////////////////////////////////////////////////
void Entity::init_entity(int new_id, int new_X, int new_Y, int new_Z){
	this->ID = new_id;
	X = new_X; Y = new_Y; Z = new_Z;
}

// position and distance
void Entity::set_position(int new_X, int new_Y, int new_Z){
	X = new_X; Y = new_Y; Z = new_Z;
}
double Entity::get_distance(int my_X, int my_Y){
	return sqrt((my_X - X)*(my_X - X) + (my_Y - Y)*(my_Y - Y));
}

// id
void Entity::set_id(int id){
	ID = id;
}
int Entity::get_id(){
	return ID;
}


/////////////////////////////////////////////////////////////////////////////////////
// Item class 
/////////////////////////////////////////////////////////////////////////////////////
Item::Item(){}
Item::Item(int new_id, int new_X, int new_Y){
	init_entity(new_id, new_X, new_Y, 0);
}

/////////////////////////////////////////////////////////////////////////////////////
// Item Living 
/////////////////////////////////////////////////////////////////////////////////////
void Living::init_living(int new_id, int new_X, int new_Y, int new_Z, int new_HP, int new_max_HP, int new_MP, int new_max_MP){
	init_entity(new_id, new_X, new_Y, new_Z);
	HP = new_HP; max_HP = new_max_HP;
}

// Monster::health
void Living::set_hp(int hp){
	HP = hp;
}
int Living::add_to_hp(int delta){
	HP += delta;
}
int Living::get_hp(){
	return HP;
}

// mana
void Living::set_mp(int mp){
	MP = mp;
}
int Living::add_to_mp(int delta){
	MP += delta;
}
int Living::get_mp(){
	return MP;
}


/////////////////////////////////////////////////////////////////////////////////////
// Monster class
/////////////////////////////////////////////////////////////////////////////////////
Monster::Monster(){}
Monster::Monster(int new_id, int new_X, int new_Y, int new_Z, int new_HP, int new_max_HP){
	init_living(new_id, new_X, new_Y, new_Z, new_HP, new_max_HP, 0, 0);
	state = E_MONSTER_PASSIVE;
	target_ID = -1;
}

void Monster::set_target(int id){
	target_ID = id;
}
int Monster::get_target(){
	return target_ID;
}


/////////////////////////////////////////////////////////////////////////////////////
// Player class
/////////////////////////////////////////////////////////////////////////////////////
Player::Player(){}
Player::Player(int new_id, int new_X, int new_Y, int new_Z, string new_name){
	init_living(new_id, new_X, new_Y, new_Z, 0, 0, 0, 0);
	Z = new_Z;
	name = new_name;
}


/////////////////////////////////////////////////////////////////////////////////////
// Bot class
/////////////////////////////////////////////////////////////////////////////////////
Bot::Bot(){
	ID = -1;
}

void Bot::setup(){
	// init map
	heightmap = Map();

	// load skills - TODO: move this to the config file!
	skills[2] = Skill(2, 1850, 1250, 8, E_SKILL_ATTACK); // fire
	skills[3] = Skill(3, 1400, 800, 8, E_SKILL_ATTACK); // ice
	skills[4] = Skill(4, 1200, 600, 7, E_SKILL_ATTACK); // lightning
	skills[23] = Skill(23, 1300, 700, 7, E_SKILL_ATTACK); // lightning blow

	skills[15] = Skill(15, 900000, 0, 0, E_SKILL_BUFF); // meditation (once in 15 minutes)
	skills[19] = Skill(19, 900000, 0, 0, E_SKILL_BUFF); // def 3 (once in 15 minutes)
}

// get distances to objects
double Bot::get_dist_to_monster(int id){
	if (monsters.count(id) > 0){
		return monsters[id].get_distance(X, Y);
	}
	return -1;
}
double Bot::get_dist_to_item(int id){
	if (items.count(id) > 0){
		return items[id].get_distance(X, Y);
	}
	return -1;
}
double Bot::get_dist_to_player(int id){
	if (players.count(id) > 0){
		return players[id].get_distance(X, Y);
	}
	return -1;
}

int Bot::get_closest_monster_id(){
	double shortest_dist = MAXINT;
	int closest_mob = -1;
	// first kill mobs that are attacking me
	for each(auto mob in monsters){
		if (mob.second.target_ID == ID){
			double distance = mob.second.get_distance(X, Y);
			if (distance < shortest_dist){
				shortest_dist = distance;
				closest_mob = mob.first;
			}
		}
	}

	// attack monster attacking me
	if (closest_mob != -1) return closest_mob;

	for each(auto mob in monsters){
		// only find the monsters that are not attacked or attacking someone
		if ((mob.second.target_ID == -1) || (mob.second.target_ID == ID)){
			double distance = mob.second.get_distance(X, Y);
			if (distance < shortest_dist){
				shortest_dist = distance;
				closest_mob = mob.first;
			}
		}
	}
	return closest_mob;
}


int Bot::get_centered_monster_id(double div){
	double shortest_dist = MAXINT;
	int closest_mob = -1;
	// first kill mobs that are attacking me
	for each(auto mob in monsters){
		if (mob.second.target_ID == ID){
			double distance = mob.second.get_distance(X, Y);
			if (distance < shortest_dist){
				shortest_dist = distance;
				closest_mob = mob.first;
			}
		}
	}

	// attack monster attacking me
	if (closest_mob != -1) return closest_mob;

	for each(auto mob in monsters){
		// only find the monsters that are not attacked or attacking someone
		if ((mob.second.target_ID == -1) || (mob.second.target_ID == ID)){
			double distance_from_start = sqrt((mob.second.X - start_X)*(mob.second.X - start_X) + (mob.second.Y - start_Y)*(mob.second.Y - start_Y));
			double distance_from_bot = get_dist_to_monster(mob.second.target_ID);
			double distance = distance_from_bot + distance_from_start/div;
			if (distance < shortest_dist){
				shortest_dist = distance;
				closest_mob = mob.first;
			}
		}
	}
	return closest_mob;
}

bool Bot::is_target_valid(int mob_id) {
	if (monsters.count(mob_id) > 0){
		if (mob_id == -1) {
			cout << "target id == -1 skipping..." << endl;
			return false;
		}
		if (monsters[mob_id].get_distance(X, Y) == -1){
			cout << "target (" << target_ID <<" ) is not valid because the distance is -1..." << endl;
			return false;
		}
		else if ((monsters[mob_id].target_ID != -1) && (monsters[mob_id].target_ID != ID)){
			cout << "target (" << target_ID << " ) is not valid because i am no KSer..." << endl;
			return false;
		}
		return true;
	}
	cout << "target (" << target_ID << " ) is not valid because there is no such target..." << endl;
	monsters.erase(mob_id);
	return false;
}

int Bot::get_closest_item_id(){
	double shortest_dist = MAXINT;
	int closest_item = -1;
	for each(auto item in items){
		double distance = item.second.get_distance(X, Y);
		if (distance < shortest_dist){
			shortest_dist = distance;
			closest_item = item.first;
		}
	}
	return closest_item;
}

bool Bot::nobody_is_around(){
	if (players.empty()){
		return true;
	}
	return false;
}

byte Bot::get_attack_skill(){
	for each (auto skill in skills){
		// if the skill is ready and there is enough mana
		if (skill.second.ready() && max_MP - MP >= skill.second.mana_cost){
			return skill.first;
		}
	}
	return false;
}

byte Bot::get_buff_skill(){
	for each (auto skill in skills){
		if (skill.second.type == E_SKILL_BUFF){
			if (skill.second.ready() && MP >= skill.second.mana_cost){
				return skill.first;
			}
		}
	}
	return false;
}


void Bot::cleanup(){
	for each(auto mob in monsters){
		if (mob.second.ID != mob.first){
			monsters.erase(mob.first);
		}
	}
	for each(auto player in players){
		if (player.second.ID != player.first){
			players.erase(player.first);
		}
	}
}


// singleton instance
Bot* Bot::instance(){
	static Bot instance_ptr;
	return &instance_ptr;
}

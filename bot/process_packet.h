#ifndef PROCESS_PACKET_H
#define PROCESS_PACKET_H

#include <Windows.h>
#include <vector>
#include <array>
#include <atomic>

using namespace std;

extern char* packet_offset;
extern DWORD U;
extern atomic<int> Z_coords;

// reading the packet
class reader
{
private:
	char* packet_offset; 

public:
	reader(char* packet_offset) : packet_offset(packet_offset) {}

	// reading the packet
	template<typename T>
	inline void read(T& value){
		static_assert(std::is_integral<T>::value, "Only works for numerical values");
		memcpy(&value, packet_offset, sizeof(T));
		packet_offset += sizeof(T);
	}

	template<>
	inline void read<string>(string& value){
		value.resize(strlen(packet_offset));
		memcpy((void*)value.c_str(), packet_offset, value.size() + 1);
		packet_offset += value.size() + 1;
	}

	template<typename T, size_t S>
	inline void read(array<T, S>& data){
		for (int i = 0; i < data.size(); ++i){
			read(data[i]);
		}
	}
};

// writing the packet
template<typename T>
inline void write(vector<char> &data, T wdata){
	size_t start = data.size();
	data.resize(start + sizeof(T));
	memcpy(data.data() + start, &wdata, sizeof(T));
}

template<>
inline void write<string>(vector<char> &data, string wdata){
	size_t start = data.size();
	data.resize(start + wdata.size() + 1);
	memcpy(data.data() + start, (void*)wdata.c_str(), wdata.size() + 1);
}





// update size of packet after adding all its content
void update_packet_size(vector<char> &packet);

// recalculate U for sending packets
void set_U(int appender);

// process received packets
vector<char> extract_packet(vector<char> *buffer, int size);
int check_for_packet(vector<char> *buffer);
void process_recv_packet(vector<char> *buffer);

// process sent packets
int process_send_packet(vector<char> *buffer);

#endif // PROCESS_PACKET_H
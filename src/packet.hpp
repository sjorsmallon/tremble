#pragma once
#include <type_traits>
#include <print>
#include <cassert>
#include "concepts.hpp"
#include "message.hpp"

constexpr int no_sequence_id = 0;

// Ethernet MTU: 1,500 bytes (the most common), which includes the size of the entire IP packet.
// Subtracting headers, the actual safe UDP payload size without fragmentation is around 1,472 bytes (for IPv4) and 1,452 bytes (for IPv6).
//
// This header is necessary to be able to reconstruct on the other side what we want.
// the sequence_id is necessary because if we deliver (very) out of order, we know to what sequence a specific packet belongs.
// the count / capacity is how many packets this object consists of, and the sequence_idx is the idx of this particular packet.
// the payload_size is needed because it can be that whatever we decide to send over the wire does not neatly divide up 
// into Packets. At least, that is my idea now.
//
// max of uint16_t is 65,535 which is more than enough (that's the max of the whole packet in ipv4/ipv6 anyway.)
struct Packet_Header
{
	uint64_t timestamp;
	uint8_t sequence_id;
	uint8_t sequence_count;
	uint8_t sequence_idx;
	uint8_t message_type;
	uint16_t payload_size;
};

// wtf is going on? alignment issues?
constexpr size_t MAX_BUFFER_SIZE_IN_BYTES = 1452 - sizeof(Packet_Header);
struct Packet
{
	Packet_Header header;
	int padding_for_alignment;
	uint8_t buffer[MAX_BUFFER_SIZE_IN_BYTES];
};

//safeguarding myself, 
static_assert(sizeof(Packet_Header) == 16);
static_assert(sizeof(Packet) == (sizeof(Packet_Header) + sizeof(int) + (MAX_BUFFER_SIZE_IN_BYTES * sizeof(uint8_t)))); //

template <typename Type>
requires Pod<Type> || PodVectorConcept<Type>
std::vector<Packet> convert_to_packets(const Type& input, Message_Type message_type)
{
 	if constexpr (Pod<Type>)
 	{
 		// std::print("Pod<Type>\n");
		size_t byte_count = sizeof(input);
		size_t packet_count = 0;

		if (byte_count < MAX_BUFFER_SIZE_IN_BYTES) packet_count = 1;
		else 
		{
			packet_count = (1 + ( byte_count - 1) / MAX_BUFFER_SIZE_IN_BYTES); // if x != 0

		}
		auto packets = std::vector<Packet>(packet_count);

		//FIXME: uh..
		assert(packet_count < UINT8_MAX);

		uint8_t sequence_id = 1; //FIXME: do something better.
		if (packet_count == 1) sequence_id = 0;
		
		uint8_t packet_idx_in_sequence = 0;
		for (auto& packet: packets)
		{
	        // calculate the offset to copy data into the current packet's buffer
	        size_t offset = packet_idx_in_sequence * MAX_BUFFER_SIZE_IN_BYTES;
	        size_t remaining_bytes = byte_count - offset;
	        size_t chunk_size = (remaining_bytes < MAX_BUFFER_SIZE_IN_BYTES) ? remaining_bytes : MAX_BUFFER_SIZE_IN_BYTES;

	        // fill the packet buffer with the appropriate data from the pod
	        std::memcpy(packet.buffer, reinterpret_cast<const uint8_t*>(&input) + offset, chunk_size);
	        packet.header.message_type = message_type;
	        packet.header.sequence_id = sequence_id;
			packet.header.sequence_count = packet_count;
			packet.header.sequence_idx = packet_idx_in_sequence;
			packet.header.payload_size = chunk_size;


			packet_idx_in_sequence += 1;
		}

		return packets;
    } else // if(podvector<>)
    {
 		// std::print("podvector<Type>\n");
    	auto byte_count = sizeof(typename Type::value_type) * input.size();
		auto packet_count = 1 + ((byte_count - 1) / MAX_BUFFER_SIZE_IN_BYTES); // if x != 0
		auto packets = std::vector<Packet>{packet_count};
		std::print("allocated {} packets\n", packet_count);

    	assert(packet_count < UINT8_MAX);

		uint8_t sequence_id = 1; //FIXME: do something better.
		if (packet_count == 1) sequence_id = 0;

		uint8_t packet_idx_in_sequence = 0;

		// Pack this. if we split up the struct, do not commit to it.
		for (auto& packet: packets)
		{
	        // calculate the offset to copy data into the current packet's buffer. 
	        size_t offset = packet_idx_in_sequence * MAX_BUFFER_SIZE_IN_BYTES;
	        size_t remaining_bytes = byte_count - offset;

	        if (remaining_bytes < MAX_BUFFER_SIZE_IN_BYTES) std::print("filling last packet, id: {}, remaining_bytes: {}\n",
	        	packet_idx_in_sequence, remaining_bytes);

	        size_t chunk_size = (remaining_bytes < MAX_BUFFER_SIZE_IN_BYTES) ? remaining_bytes : MAX_BUFFER_SIZE_IN_BYTES;

	        // fill the packet buffer with the appropriate data from the pod
	        std::memcpy(packet.buffer, reinterpret_cast<const uint8_t*>(input.data()) + offset, chunk_size);
	        
	        packet.header.message_type = message_type;
	        packet.header.sequence_id    = sequence_id;
			packet.header.sequence_count = packet_count;
			packet.header.sequence_idx   = packet_idx_in_sequence;
			packet.header.payload_size   = chunk_size;
			// timestamp intentionally not set because you should set that before sending it.

			packet_idx_in_sequence += 1;
		}


		return packets;
    }
}

Packet construct_message_only_packet(Message_Type message_type)
{
	Packet packet{};
	packet.header.sequence_id = 0; // sentinel value for no sequence;
	packet.header.sequence_count = 1;
	packet.header.sequence_idx = 0;
	packet.header.message_type = message_type;
	packet.header.payload_size = 0;
	// set the timestamp before sending it.

	return packet;

}

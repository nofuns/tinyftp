#pragma once

#include "Socket.h"

#include <vector>

class IPAddress;

class SocketTCP : public Socket
{
public:
	SocketTCP();
	unsigned short get_local_port() const;
	IPAddress get_remote_address() const;
	unsigned short get_remote_port() const;
	Status connect(const IPAddress& remoteAddr, unsigned short remotePort, int time = 0);
	void disconnect();
	Status send(const void* data, size_t size);
	Status send(const void* data, size_t size, size_t& sent);
	Status receive(const void* data, size_t size, size_t& received);

private:
	friend class TCPListener;

	struct PendingPacket
	{
		PendingPacket();

		unsigned			size;
		size_t				sizeReceived;
		std::vector<char>	data;
	};

	PendingPacket m_pendingPacket;
};
#pragma once
#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <ws2tcpip.h>

#include "Socket.h"

////////////////////////////////
// WINDOWS SOCKET IMPLEMENTATION


typedef int SocketHandle;

class SocketImpl
{
public:
	typedef int AddrLength;
	static sockaddr_in create_address(unsigned address, unsigned short port);
	static SocketHandle invalid_socket();
	static void close(SocketHandle sock);
	static void set_blocking(SocketHandle sock, bool block);
	static Socket::Status get_err_status();
};
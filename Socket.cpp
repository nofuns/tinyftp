#include "Socket.h"
#include "SocketImpl.h"

#include <iostream>
#include <vector>
#include <winsock2.h>

Socket::Socket(Type type)
	: m_type(type), m_socket(SocketImpl::invalid_socket()), m_isBlocking(true)
{}

Socket::~Socket()
{
	close();
}

void Socket::set_blocking(bool blocking)
{
	if (m_socket != SocketImpl::invalid_socket())
		SocketImpl::set_blocking(m_socket, blocking);

	m_isBlocking = blocking;
}

bool Socket::is_blocking() const
{
	return m_isBlocking;
}

SocketHandle Socket::get_handle() const
{
	return m_socket;
}

void Socket::create()
{
	// Dont create socket if it already exists
	if (m_socket == SocketImpl::invalid_socket()) {
		SocketHandle handle = socket(AF_INET, m_type == TCP ? SOCK_STREAM : SOCK_DGRAM, NULL);

		if (handle == SocketImpl::invalid_socket()) {
			std::cout << "Failed to create socket." << std::endl;	// TODO: replace with dbg message 
			return;
		}

		create(handle);
	}
}

void Socket::create(SocketHandle handle)
{
	if (m_socket == SocketImpl::invalid_socket()) {
		m_socket = handle;
		set_blocking(m_isBlocking);
		// Remove buffering of TCP packets
		if (m_type == TCP) {
			int optVal = 1;
			if (setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&optVal, sizeof(optVal)) == -1) {
				std::cout << "Failed to set option TCP_NODELAY" << std::endl;
			}
		}
		else {
			// TODO: Enable broadcast by default for UDP
			return;
		}
	}
}

void Socket::close()
{
	if (m_socket != SocketImpl::invalid_socket()) {
		SocketImpl::close(m_socket);
		m_socket = SocketImpl::invalid_socket();
	}
}
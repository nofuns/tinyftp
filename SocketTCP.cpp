#include "SocketTCP.h"
#include "IPAddress.h"
#include "Packet.h"
#include "SocketImpl.h"

#include <algorithm>
#include <iostream>

SocketTCP::SocketTCP()
	: Socket(TCP)
{}

unsigned short SocketTCP::get_local_port() const
{
	if (get_handle() != SocketImpl::invalid_socket()) {	// TODO: (?) Change to enum {INVALID_SOCKET = 0};
		sockaddr_in address;
		SocketImpl::AddrLength size = sizeof(address);
		if (getsockname(get_handle(), (sockaddr*)&address, &size) != -1) {
			return ntohs(address.sin_port);
		}
	}

	return 0; // Failed to retrieve port
}

IPAddress SocketTCP::get_remote_address() const
{
	if (get_handle() != SocketImpl::invalid_socket())
	{
		// Retrieve informations about the remote end of the socket
		sockaddr_in address;
		SocketImpl::AddrLength size = sizeof(address);
		if (getpeername(get_handle(), reinterpret_cast<sockaddr*>(&address), &size) != -1)
		{
			return IPAddress(ntohl(address.sin_addr.s_addr));
		}
	}

	// We failed to retrieve the address
	return IPAddress::None;
}

unsigned short SocketTCP::get_remote_port() const
{
	if (get_handle() != SocketImpl::invalid_socket()) {	// TODO: (?) Change to enum {INVALID_SOCKET = 0};
		sockaddr_in address;
		SocketImpl::AddrLength size = sizeof(address);
		if (getpeername(get_handle(), (sockaddr*)&address, &size) != -1) {
			return ntohs(address.sin_port);
		}
	}

	return 0; // Failed to retrieve port
}

Socket::Status SocketTCP::connect(const IPAddress& remoteAddress, unsigned short remotePort, int timeout)
{
	disconnect();
	create();		// Create socket if it doesnt exist

	sockaddr_in address = SocketImpl::create_address(remoteAddress.to_int(), remotePort);
	if (timeout <= 0) {
		if (::connect(get_handle(), (sockaddr*)&address, sizeof(address)) == -1)
			return SocketImpl::get_err_status();

		return Done;
	}
	else {
		bool blocking = is_blocking();
		if (blocking)
			set_blocking(false);

		if (::connect(get_handle(), (sockaddr*)&address, sizeof(address)) == -1) {
			set_blocking(blocking);
			return Done;
		}

		Status status = SocketImpl::get_err_status();

		if (!blocking)
			return status;

		if (status == Socket::NotReady) {
			fd_set selector;
			FD_ZERO(&selector);
			FD_SET(get_handle(), &selector);

			timeval time = { timeout, 0 };
			if (select((int)(get_handle() + 1), NULL, &selector, NULL, &time) > 0) {
				if (get_remote_address() != IPAddress::None)
					status = Done;
				else
					status = SocketImpl::get_err_status();
			}
			else {
				status = SocketImpl::get_err_status();
			}
		}
		set_blocking(true);

		return status;
	}
}

void SocketTCP::disconnect()
{
	close();
	m_pendingPacket = PendingPacket();	// Reset the pending packet data
}

Socket::Status SocketTCP::send(const void* data, std::size_t size)
{
	if (!is_blocking())
		std::cout << "Partial sends might not be handled properly" << std::endl; // TODO: remove with dbg message

	std::size_t sent;

	return send(data, size, sent);
}

Socket::Status SocketTCP::send(const void* data, std::size_t size, std::size_t& sent)
{
	if (!data || (size == 0)) {
		std::cout << "Cannot send data over the network: no data to send." << std::endl; // TODO: remove with dbg message
		return Error;
	}

	int result = 0;
	for (sent = 0; sent < size; sent += result) {
		result = ::send(get_handle(), (const char*)(data)+sent, (int)(size - sent), 0);
		if (result < 0) {
			Status status = SocketImpl::get_err_status();
			if ((status == NotReady) && sent)
				return Partial;

			return status;
		}
	}

	return Done;
}

Socket::Status SocketTCP::receive(const void* data, std::size_t size, std::size_t& received)
{
	received = 0;
	if (!data) {
		std::cout << "Cannot receive data: destination buffer is invalid." << std::endl;   // TODO: remove with dbg message
		return Error;
	}

	int sizeReceived = recv(get_handle(), (char*)data, (int)size, 0);
	if (sizeReceived > 0) {
		received = (std::size_t)sizeReceived;
		return Done;
	}
	else if (sizeReceived == 0) {
		return Socket::Disconnected;
	}
	else {
		return SocketImpl::get_err_status();
	}
}

SocketTCP::PendingPacket::PendingPacket()
	: size(0), sizeReceived(0), data()
{}
#include "SocketImpl.h"

sockaddr_in SocketImpl::create_address(unsigned address, unsigned short port)
{
	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_addr.s_addr	= htonl(address);
	addr.sin_family			= AF_INET;
	addr.sin_port			= htons(port);

	return addr;
}

SocketHandle SocketImpl::invalid_socket()
{
	return INVALID_SOCKET;
}

void SocketImpl::close(SocketHandle sock)
{
	closesocket(sock);
}

void SocketImpl::set_blocking(SocketHandle sock, bool block)
{
	u_long blocking = block ? 0 : 1;
	ioctlsocket(sock, FIONBIO, &blocking);
}

Socket::Status SocketImpl::get_err_status()
{
    switch (WSAGetLastError())
    {
    case WSAEWOULDBLOCK:  return Socket::NotReady;
    case WSAEALREADY:     return Socket::NotReady;
    case WSAECONNABORTED: return Socket::Disconnected;
    case WSAECONNRESET:   return Socket::Disconnected;
    case WSAETIMEDOUT:    return Socket::Disconnected;
    case WSAENETRESET:    return Socket::Disconnected;
    case WSAENOTCONN:     return Socket::Disconnected;
    case WSAEISCONN:      return Socket::Done;          // when connecting a non-blocking socket
    default:              return Socket::Error;
    }
}

struct SocketInitializer
{
    SocketInitializer()
    {
        WSAData data;
        WSAStartup(MAKEWORD(2, 2), &data);
    }

    ~SocketInitializer()
    {
        WSACleanup();
    }
} globalSocketInitializer;
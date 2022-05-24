#pragma once 

#define _WINSOCK_DEPRECATED_NO_WARNINGS

typedef int SocketHandle;
class SocketSelector;

class Socket
{
public:
	enum Status
	{
		Done,
		NotReady,
		Partial,
		Disconnected,
		Error
	};

	enum
	{
		AnyPort = 0
	};

public:
	virtual ~Socket();
	void set_blocking(bool blocking);
	bool is_blocking() const;

protected:
	enum Type
	{
		TCP,
		UDP
	};

	Socket(Type type);
	SocketHandle get_handle() const;
	void create();
	void create(SocketHandle handle);
	void close();

private:
	friend class SocketSelector;
	
	Type			m_type;
	SocketHandle	m_socket;
	bool			m_isBlocking;
};
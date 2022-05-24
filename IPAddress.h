#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS


#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

class IPAddress
{
public:
	IPAddress();
	IPAddress(unsigned address);
	IPAddress(const std::string& address);
	IPAddress(const char* address);
	~IPAddress();

	
	unsigned int to_int() const;
	std::string to_string() const;
	 
	static const IPAddress None;		// empty/invalid address
	static const IPAddress Any;			// (0.0.0.0)
	static const IPAddress LocalHost;	
	static const IPAddress Broadcast;

private:
	void resolve(const std::string& address);
	friend bool operator<(const IPAddress& left, const IPAddress& right);

private:
	unsigned int m_address;
	bool m_valid;
};

bool operator==(const IPAddress& right, const IPAddress& left);
bool operator!=(const IPAddress& right, const IPAddress& left);
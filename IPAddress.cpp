#include "IPAddress.h"


const IPAddress IPAddress::None;
const IPAddress IPAddress::Any("0.0.0.0");
const IPAddress IPAddress::LocalHost("127.0.0.1");
const IPAddress IPAddress::Broadcast("255.255.255.255");


IPAddress::IPAddress() : m_address(0), m_valid(false)
{}

IPAddress::IPAddress(unsigned address)
	: m_address(htonl(address)), m_valid(true)
{}

IPAddress::IPAddress(const char* address) : m_address(0), m_valid(false)
{
	resolve(address);
}

IPAddress::IPAddress(const std::string& address) : m_address(0), m_valid(false)
{
	resolve(address);
}

IPAddress::~IPAddress()
{}

unsigned int IPAddress::to_int() const
{
	return ntohl(m_address);
}

std::string IPAddress::to_string() const
{
	in_addr address;
	address.s_addr = m_address;

	return inet_ntoa(address);
}

void IPAddress::resolve(const std::string& address)
{
	m_address = 0;
	m_valid = false;

	if (address == "255.255.255.255") {
		m_address = INADDR_BROADCAST;
		m_valid = true;
	}
	else if (address == "0.0.0.0") {
		m_address = INADDR_ANY;
		m_valid = true;
	}
	else {
		unsigned ip = inet_addr(address.c_str());
		if (ip != INADDR_NONE) {
			m_address = ip;
			m_valid = true;
		}
		else {
			addrinfo hints;
			std::memset(&hints, 0, sizeof(hints));
			hints.ai_family = AF_INET;
			addrinfo* result = NULL;
			if (getaddrinfo(address.c_str(), NULL, &hints, &result) == 0) {
				if (result) {
					ip = reinterpret_cast<sockaddr_in*>(result->ai_addr)->sin_addr.s_addr;
					freeaddrinfo(result);
					m_address = ip;
					m_valid = true;
				}
			}
		}

	}
}


bool operator <(const IPAddress& left, const IPAddress& right)
{
	return std::make_pair(left.m_valid, left.m_address) < std::make_pair(right.m_valid, right.m_address);
}

bool operator ==(const IPAddress& left, const IPAddress& right)
{
	return !(left < right) && !(right < left);
}

bool operator !=(const IPAddress& left, const IPAddress& right)
{
	return !(left == right);
}

bool operator >(const IPAddress& left, const IPAddress& right)
{
	return right < left;
}

bool operator <=(const IPAddress& left, const IPAddress& right)
{
	return !(right < left);
}

bool operator >=(const IPAddress& left, const IPAddress& right)
{
	return !(left < right);
}
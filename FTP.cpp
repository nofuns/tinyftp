#pragma once

#include "FTP.h"
#include "IPAddress.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <sstream>
#include <cstdio>

class FTP::DataChannel
{
public:
	DataChannel(FTP& owner);
	FTP::Response open(FTP::TransferMode mode);
	void send(std::istream& stream);
	void receive(std::ostream& stream);

private:
	FTP&		m_ftp;
	SocketTCP	m_data_sock;
};

FTP::Response::Response(Status code, const std::string& message)
	: m_status(code), m_message(message)
{}

bool FTP::Response::is_ok() const
{
	return m_status < 400;
}

const std::string& FTP::Response::get_message() const
{
	return m_message;
}

FTP::Response::Status FTP::Response::get_status() const
{
	return m_status;
}

FTP::DirectoryResponse::DirectoryResponse(const FTP::Response& response)
	: FTP::Response(response)
{
	if (is_ok()) {
		std::string::size_type begin	= get_message().find('"', 0);
		std::string::size_type end		= get_message().find('"', begin + 1);
		m_directory = get_message().substr(begin + 1, end - begin - 1);
	}
}

const std::string& FTP::DirectoryResponse::get_directory() const
{
	return m_directory;
}

FTP::ListingResponse::ListingResponse(const FTP::Response& response, const std::string& data)
	: FTP::Response(response)
{
	if (is_ok()) {
		std::string::size_type lastPos = 0;
		for (std::string::size_type pos = data.find("\r\n");
			pos != std::string::npos;
			pos = data.find("\r\n", lastPos)) 
		{
			m_listing.push_back(data.substr(lastPos, pos - lastPos));
			lastPos = pos + 2;
		}
	}
}



const std::vector<std::string>& FTP::ListingResponse::get_listing() const
{
	return m_listing;
}

FTP::~FTP()
{
	disconnect();
}

FTP::Response FTP::connect(const IPAddress& server, unsigned short port, int timeout)
{
	if (m_commandSocket.connect(server, port, timeout) != Socket::Done)
		return Response(Response::ConnectionFailed);

	return get_response();
}

FTP::Response FTP::login()
{
	return login("anonymous", "");
}

FTP::Response FTP::login(const std::string& user, const std::string& password)
{
	Response response = quote("USER", user);
	if (response.is_ok()) 
		response = quote("PASS", password);

	return response;
}

FTP::Response FTP::disconnect()
{
	Response response = quote("QUIT");
	if (response.is_ok())
		m_commandSocket.disconnect();

	return response;
}

FTP::Response FTP::keep_alive()
{
	return quote("NOOP");
}

FTP::DirectoryResponse FTP::pwd() 
{
	return DirectoryResponse(quote("PWD"));
}

FTP::ListingResponse FTP::ls(const std::string& dir)
{
	// Open data channel on default port, ASCII mode
	std::ostringstream dirData;
	DataChannel data(*this);
	Response response = data.open(ASCII);
	if (response.is_ok()) {
		response = quote("NLST", dir);
		if (response.is_ok()) {
			data.receive(dirData);	// Receive listing
			response = get_response();
		}
	}

	return ListingResponse(response, dirData.str());
}

FTP::Response FTP::cd(const std::string& dir)
{
	return quote("CWD", dir);
}

FTP::Response FTP::cdup()
{
	return quote("CDUP");
}

FTP::Response FTP::mkdir(const std::string& name)
{
	return quote("MKD", name);
}

FTP::Response FTP::rmdir(const std::string& name)
{
	return quote("RMD", name);
}

FTP::Response FTP::rename(const std::string& file, const std::string& newName)
{
	Response response = quote("RNFR", file);
	if (response.is_ok())
		response = quote("RNTO", newName);

	return response;
}

FTP::Response FTP::del(const std::string& name)
{
	return quote("DELE", name);
}

FTP::Response FTP::get(const std::string& remoteFile, const std::string& localPath, TransferMode mode)
{
	DataChannel data(*this);
	Response response = data.open(mode);

	if (response.is_ok()) {
		response = quote("RETR", remoteFile);
		if (response.is_ok()) {
			// Extract the filename from the file path
			std::string filename = remoteFile;
			std::string::size_type pos = filename.find_last_of("/\\");
			if (pos != std::string::npos)
				filename = filename.substr(pos + 1);

			// Make sure the destination path ends with slash
			std::string path = localPath;
			if (!path.empty() && (path[path.size() - 1] != '\\') && (path[path.size() - 1] != '/'))
				path += "/";

			std::ofstream file((path + filename).c_str(), std::ios_base::binary | std::ios_base::trunc);
			if (!file)
				return Response(Response::InvalidFile);

			data.receive(file);

			file.close();
			response = get_response();

			if (!response.is_ok())
				std::remove((path + filename).c_str());
		}
	}

	return response;
}

FTP::Response FTP::put(const std::string& localFile, const std::string& remotePath, TransferMode mode, bool append)
{
	std::ifstream file(localFile, std::ios_base::binary);
	if (!file)
		return Response(Response::InvalidFile);

	std::string filename = localFile;
	std::string::size_type pos = filename.find_last_of("/\\");
	if (pos != std::string::npos)
		filename = filename.substr(pos + 1);

	std::string path = remotePath;
	if (!path.empty() && (path[path.size() - 1] != '\\') && (path[path.size() - 1] != '/'))
		path += "/";

	DataChannel data(*this);
	Response response = data.open(mode);
	if (response.is_ok()) {
		response = quote(append ? "APPE" : "STOR", path + filename);
		data.send(file);
		response = get_response();
	}

	return response;
}

FTP::Response FTP::quote(const std::string cmd, const std::string param)
{
	// Build the command string
	std::string commandStr;
	if (param != "")
		commandStr = cmd + " " + param + "\r\n";
	else
		commandStr = cmd + "\r\n";

	if (m_commandSocket.send(commandStr.c_str(), commandStr.length()) != Socket::Done)
		return Response(Response::ConnectionClosed);

	return get_response();
}

FTP::Response FTP::get_response()
{
	unsigned lastCode = 0;
	bool isInsideMultiline = false;
	std::string message;

	while (true) {
		// Receive response from the server
		char buffer[1024];
		std::size_t length;

		if (m_recvBuffer.empty()) {
			if (m_commandSocket.receive(buffer, sizeof(buffer), length) != Socket::Done)
				return Response(Response::ConnectionClosed);
		}
		else {
			std::copy(m_recvBuffer.begin(), m_recvBuffer.end(), buffer);
			length = m_recvBuffer.size();
			m_recvBuffer.clear();
		}

		std::istringstream in(std::string(buffer, length), std::ios_base::binary);
		while (in) {
			unsigned code;
			if (in >> code) {
				// Extract the separator
				char separator;
				in.get(separator);
				// '-' char means a multiline response
				if ((separator == '-') && !isInsideMultiline) {
					isInsideMultiline = true;

					if (lastCode == 0)
						lastCode = code;

					std::getline(in, message);
					// Remove '\r'
					message.erase(message.length() - 1);
					message = separator + message + "\n";
				}
				else {
					if ((separator != '-') && (code == lastCode) || lastCode == 0) {
						std::string line;
						std::getline(in, line);

						// Remove '\r'
						line.erase(line.length() - 1);

						// Append it to the message
						if (code == lastCode) {
							std::ostringstream out;
							out << code << separator << line;
							message += out.str();
						}
						else {
							message = separator + line;
						}

						m_recvBuffer.assign(buffer + (size_t)in.tellg(), length - size_t(in.tellg()));

						return Response((Response::Status)code, message);
					}
					else {
						std::string line;
						std::getline(in, line);
						
						if (!line.empty()) {
							line.erase(line.length() - 1);
							std::ostringstream out;
							out << code << separator << line << "\n";
							message += out.str();
						}
					}
				}
			}
			else if (lastCode != 0){
				in.clear();
				std::string line;
				std::getline(in, line);
				if (!line.empty()) {
					line.erase(line.length() - 1);
					message += line + "\n";
				}
			} else {
				return Response(Response::InvalidResponse);
			}
		}
	}
	// Unreachable
}

FTP::DataChannel::DataChannel(FTP& owner)
	: m_ftp(owner)
{}

FTP::Response FTP::DataChannel::open(FTP::TransferMode mode)
{
	FTP::Response response = m_ftp.quote("PASV");
	if (response.is_ok()) {
		// Extract the connection address and port from the response
		std::string::size_type begin = response.get_message().find_first_of("0123456789");
		if (begin != std::string::npos) {
			unsigned char data[6] = { 0, 0, 0, 0, 0, 0 };
			std::string str = response.get_message().substr(begin);
			std::size_t index = 0;
			for (int i = 0; i < sizeof(data); i++) {
				while (isdigit(str[index])) {
					data[i] = data[i] * 10 + (str[index] - '0');
					index++;
				}
				index++; // Skip separator
			}

			unsigned short port = data[4] * 256 + data[5];
			unsigned addr = ((unsigned)data[0] << 24) |
							((unsigned)data[1] << 16) |
							((unsigned)data[2] <<  8) |
							((unsigned)data[3]);
			IPAddress address(addr);

			// Connect data channel to the server
			if (m_data_sock.connect(address, port) == Socket::Done) {
				std::string modeStr;
				switch (mode) {
					case FTP::Binary:	modeStr = "I"; break;
					case FTP::ASCII:	modeStr = "A"; break;
				}

				response = m_ftp.quote("TYPE", modeStr);
			}
			else {
				response = FTP::Response(FTP::Response::ConnectionFailed);
			}
		}
	}

	return response;
}

void FTP::DataChannel::receive(std::ostream& stream)
{
	char buffer[1024];
	std::size_t received;
	while (m_data_sock.receive(buffer, sizeof(buffer), received) == Socket::Done) {
		stream.write(buffer, (std::streamsize)received);
		if (!stream.good()) {
			std::cout << "FTP Error: Writing to the file has failed." << std::endl;
			break;
		}
	}

	m_data_sock.disconnect();
}

void FTP::DataChannel::send(std::istream& stream)
{
	char buffer[1024];
	std::size_t count;
	while (true) {
		stream.read(buffer, sizeof(buffer));

		if (!stream.good() && !stream.eof()) {
			std::cout << "FTP Error: Reading from the file has failed." << std::endl;
			break;
		}

		count = (std::size_t)stream.gcount();

		if (count > 0) {
			if (m_data_sock.send(buffer, count) != Socket::Done)
				break;
		}
		else {
			// No more data
			break;
		}
	}

	m_data_sock.disconnect();
}
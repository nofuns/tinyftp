#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "FTP.h"

#include <iostream>
#include <string>
#include <chrono>
#include <ctime>



int main()
{
	std::cout << "Started" << std::endl;

	FTP ftp;
	FTP::Response response = ftp.connect("ftp.dlptest.com");
	if (response.is_ok())
		std::cout << "Connected" << std::endl;

	response = ftp.login("dlpuser@dlptest.com", "SzMf7rTE4pCrf9dV286GuNe4N"); 
	if (response.is_ok())
		std::cout << "Logged in" << std::endl;

	FTP::DirectoryResponse directory = ftp.pwd();
	if (directory.is_ok())
		std::cout << "Working directory: " << directory.get_directory() << std::endl;

	response = ftp.mkdir("test folder");
	if (response.is_ok())
		std::cout << "Created new directory" << std::endl;

	FTP::ListingResponse listing = ftp.ls();
	
	if (listing.is_ok())
		for (auto line : listing.get_listing())
			std::cout << line << std::endl;

	std::string filename = "test.txt";
	std::cout << "Upload file: " + filename << std::endl;
	
	response = ftp.put(filename, "/", FTP::TransferMode::ASCII);
	if (response.is_ok())
		std::cout << "File uploaded." << std::endl;

	response = ftp.get(filename, "downloads/", FTP::TransferMode::ASCII);
	if (response.is_ok())
		std::cout << "File dowloaded." << std::endl;

	
	ftp.disconnect();

	std::cin.get();
	return 0;
}

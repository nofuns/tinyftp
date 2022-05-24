#pragma once
#pragma comment(lib, "ws2_32.lib")

#include <iostream>
#include <string>
#include <vector>
#include <winsock2.h>


#include "SocketTCP.h"
#include "IPAddress.h"


class FTP
{
public:
    enum TransferMode {
        Binary,
        ASCII
    };

    class Response
    {
    public:
        enum Status
        {
            // 1xx: the requested action is being initiated, expect another reply before proceeding with a new command
            RestartMarkerReply = 110,           ///< Restart marker reply
            ServiceReadySoon = 120,             ///< Service ready in N minutes
            DataConnectionAlreadyOpened = 125,  ///< Data connection already opened, transfer starting
            OpeningDataConnection = 150,        ///< File status ok, about to open data connection

            // 2xx: the requested action has been successfully completed
            Ok = 200,                       ///< Command ok
            PointlessCommand = 202,         ///< Command not implemented
            SystemStatus = 211,             ///< System status, or system help reply
            DirectoryStatus = 212,          ///< Directory status
            FileStatus = 213,               ///< File status
            HelpMessage = 214,              ///< Help message
            SystemType = 215,               ///< NAME system type, where NAME is an official system name from the list in the Assigned Numbers document
            ServiceReady = 220,             ///< Service ready for new user
            ClosingConnection = 221,        ///< Service closing control connection
            DataConnectionOpened = 225,     ///< Data connection open, no transfer in progress
            ClosingDataConnection = 226,    ///< Closing data connection, requested file action successful
            EnteringPassiveMode = 227,      ///< Entering passive mode
            LoggedIn = 230,                 ///< User logged in, proceed. Logged out if appropriate
            FileActionOk = 250,             ///< Requested file action ok
            DirectoryOk = 257,              ///< PATHNAME created

            // 3xx: the command has been accepted, but the requested action� is dormant, pending receipt of further information
            NeedPassword = 331,         ///< User name ok, need password
            NeedAccountToLogIn = 332,   ///< Need account for login
            NeedInformation = 350,      ///< Requested file action pending further information

            // 4xx: the command was not accepted and the requested action did not take place but the error condition is temporary and the action may be requested again
            ServiceUnavailable = 421,           ///< Service not available, closing control connection
            DataConnectionUnavailable = 425,    ///< Can't open data connection
            TransferAborted = 426,              ///< Connection closed, transfer aborted
            FileActionAborted = 450,            ///< Requested file action not taken
            LocalError = 451,                   ///< Requested action aborted, local error in processing
            InsufficientStorageSpace = 452,     ///< Requested action not taken; insufficient storage space in system, file unavailable

            // 5xx: the command was not accepted and the requested action did not take place
            CommandUnknown = 500,           ///< Syntax error, command unrecognized
            ParametersUnknown = 501,        ///< Syntax error in parameters or arguments
            CommandNotImplemented = 502,    ///< Command not implemented
            BadCommandSequence = 503,       ///< Bad sequence of commands
            ParameterNotImplemented = 504,  ///< Command not implemented for that parameter
            NotLoggedIn = 530,              ///< Not logged in
            NeedAccountToStore = 532,       ///< Need account for storing files
            FileUnavailable = 550,          ///< Requested action not taken, file unavailable
            PageTypeUnknown = 551,          ///< Requested action aborted, page type unknown
            NotEnoughMemory = 552,          ///< Requested file action aborted, exceeded storage allocation
            FilenameNotAllowed = 553,       ///< Requested action not taken, file name not allowed
        
            // 10xx: Custom codes
            InvalidResponse = 1000,     ///< Not part of the FTP standard, generated by SFML when a received response cannot be parsed
            ConnectionFailed = 1001,    ///< Not part of the FTP standard, generated by SFML when the low-level socket connection with the server fails
            ConnectionClosed = 1002,    ///< Not part of the FTP standard, generated by SFML when the low-level socket connection is unexpectedly closed
            InvalidFile = 1003          ///< Not part of the FTP standard, generated by SFML when a local file cannot be read or written
        };
    
        explicit Response(Status code = InvalidResponse, const std::string& message = "");
        bool is_ok() const;
        Status get_status() const;
        const std::string& get_message() const;

    private:
        Status m_status;
        std::string m_message;
    };

    class DirectoryResponse : public Response 
    {
    public:
        DirectoryResponse(const Response& response);
        const std::string& get_directory() const;

    private:
        std::string m_directory;
    };

    class ListingResponse : public Response
    {
    public:
        ListingResponse(const Response& response, const std::string& data);
        const std::vector<std::string>& get_listing() const;

    private:
        std::vector<std::string> m_listing;
    };


    ~FTP();

    Response connect(const IPAddress& server, unsigned short port = 21, int timeout = 0);
    Response disconnect();
    Response login();
    Response login(const std::string& user, const std::string& password);
    Response keep_alive();                      // Send null command
    Response cd(const std::string& dir);        // Change directory
    Response cdup();                            // Go to parent dir
    Response mkdir(const std::string& name);    // Create directory
    Response rmdir(const std::string& name);    // Delete directory
    Response rename(const std::string& file, const std::string& newName);   // Rename file
    Response del(const std::string& name);      // delete file
    Response get(const std::string& remoteFile, const std::string& localPath, TransferMode mode);                       // Download file
    Response put(const std::string& localFile, const std::string& remotePath, TransferMode mode, bool append = false);  // Upload file
    DirectoryResponse pwd();                            // Get working directory
    ListingResponse ls(const std::string& dir = "");    // Get list
    Response quote(const std::string cmd, const std::string param = "");    // Send command to server
    

private:
    Response get_response();
    class DataChannel;
    friend class DataChannel;

    SocketTCP  m_commandSocket;
    std::string m_recvBuffer;
};
#include <iostream>
#include <fstream>

#include <winsock2.h>
#include <pthread.h>

#include "tools/json.hpp"

using json = nlohmann::json;

const char* HttpJsonToCstr(json j_file)
{
    std::string str = "";

    if(j_file.find("Start-line") == j_file.end())
        throw std::invalid_argument("\"Start-line\" key was not found in the JSON file...");

    str += j_file["Start-line"].get<std::string>() + "\r\n";

    if(j_file.find("Headers") == j_file.end())
        throw std::invalid_argument("\"Headers\" key was not found in the JSON file...");

    for(auto it = j_file["Headers"].begin(); it != j_file["Headers"].end(); ++it)
        str += it.key() + ": " + it.value().get<std::string>() + "\r\n";

    str += "\r\n";

    if(j_file.find("Body") == j_file.end())
        return str.c_str();

    for(auto it = j_file["Body"].begin(); it != j_file["Body"].end(); ++it)
    {
        str += it.value().get<std::string>() + "\r\n";        
    }

    return str.c_str();
}

std::string getHostNameFromJson(json j_file)
{
    return j_file["Headers"]["Host"].get<std::string>();
}

std::string processHttpRequest(const char* httpRequest, const char* hostName)
{
    WSADATA wsaData;
    int err = WSAStartup(MAKEWORD(2,2), &wsaData); 
    if (err != 0){
        std::cout << "ERROR: WSAStartup failed - " << err << std::endl;
        return 0;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    // struct hostent* host = gethostbyname(hostName);
    struct hostent* host = gethostbyname("www.google.com");

    SOCKADDR_IN sockAddr;

    sockAddr.sin_port=htons(80);
    sockAddr.sin_family=AF_INET;
    sockAddr.sin_addr.s_addr = *((unsigned long*)host->h_addr);

    if(connect(sock, (SOCKADDR*)(&sockAddr), sizeof(sockAddr)) != 0)
    {
        std::cout << "ERROR: Could not connect - " << WSAGetLastError() << std::endl;
        return 0;
    }

    send(sock, httpRequest, strlen(httpRequest), 0);

    int nDataLength;
    char buffer[10000];
    std::string response = "";
    while((nDataLength = recv(sock, buffer, 10000, 0)) > 0)
    {
        std::cout<< buffer << std::endl;
        int i = 0;
        while (buffer[i] >= 32 || buffer[i] == '\n' || buffer[i] == '\r'){

            response+=buffer[i];
            i++;
        }      
    }

    closesocket(sock);
    WSACleanup();

    return response;
}

int main(int argc, char **argv)
{
    if(argc != 2)
    {
        std::cout << argc - 1 << " arguments were given, only one was expected, exiting..." << std::endl;
        return 0;
    }

    json j;
    std::ifstream i(argv[1]);
    i >> j;

    std::cout << "Request: " << std::endl;
    std::cout << HttpJsonToCstr(j) << std::endl;

    std::cout << "Response: " << std::endl;
    std::cout << processHttpRequest(HttpJsonToCstr(j), getHostNameFromJson(j).c_str()) << std::endl;

    return 0;
}
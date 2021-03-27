#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <stdexcept>
#include <winsock2.h>
#include <pthread.h>

#include "tools/encryption.hpp"

std::string HostnameFromUrl(const char* url)
{
    std::string url_str = url;
    size_t search_head = 0;

    if((search_head = url_str.find( "://", search_head)) != std::string::npos)
    {
        url_str.erase(0, search_head + 3);
        size_t search_head = 0;
    }

    if((search_head = url_str.find( "@", search_head)) != std::string::npos)
    {
        url_str.erase(0, search_head + 1);
        size_t search_head = 0;
    }

    if((search_head = url_str.find( ":", search_head)) != std::string::npos)
        return url_str.substr(0, search_head);
    else if((search_head = url_str.find( "/", search_head)) != std::string::npos)
        return url_str.substr(0, search_head);
    else if((search_head = url_str.find( "?", search_head)) != std::string::npos)
        return url_str.substr(0, search_head);
    else if((search_head = url_str.find( "#", search_head)) != std::string::npos)
        return url_str.substr(0, search_head);

    return url_str;
}

int PortFromUrl(const char* url)
{
    std::string url_str = url;
    size_t search_head = 0;

    if((search_head = url_str.find( "://", search_head)) != std::string::npos)
    {
        url_str.erase(0, search_head + 3);
        size_t search_head = 0;
    }

    if((search_head = url_str.find( "@", search_head)) != std::string::npos)
    {
        url_str.erase(0, search_head + 1);
        size_t search_head = 0;
    }

    if((search_head = url_str.find( ":", search_head)) == std::string::npos)
        return -1;
    else
    {
        url_str.erase(0, search_head + 1);
        size_t search_head = 0;
    }

    if((search_head = url_str.find( "/", search_head)) != std::string::npos)
        url_str = url_str.substr(0, search_head).c_str();
    else if((search_head = url_str.find( "?", search_head)) != std::string::npos)
        url_str = url_str.substr(0, search_head).c_str();
    else if((search_head = url_str.find( "#", search_head)) != std::string::npos)
        url_str = url_str.substr(0, search_head).c_str();

    std::stringstream port_string(url_str);

    int port;
    port_string >> port;

    return port;
}

std::string UrlToHttpRequest(const char* url, std::string method = "GET", std::string protocol_version = "HTTP/1.1")
{
    
    std::string url_str = url;
    std::string http_headers = "";
    std::string start_line = "";
    size_t search_head = 0;
    size_t new_search_head = 0;

    /*  
        URI = scheme:[//authority]path[?query][#fragment]
        authority = [userinfo@]host[:port]

        Parse to:
            Protocol
            User info
            Hostname
            Port
            Query
            Hash
    */

    //Protocol
    if((new_search_head = url_str.find( "://", search_head)) != std::string::npos)
    {
        std::string protocol = url_str.substr(search_head, new_search_head - search_head);
        if(protocol != "http")
            throw std::invalid_argument("Error: " + protocol + "protocol provided, expecting http protocol...");

        // url_str.erase(search_head, new_search_head - search_head + 3); // Deleting "http://" from the start

        search_head = new_search_head + 3; // placing search_head after "://"
    }

    //User info
    if((new_search_head = url_str.find( "@", search_head)) != std::string::npos || url_str.find( ":", search_head) < new_search_head)
    {
        std::string user_info = url_str.substr(search_head, new_search_head - search_head);

        http_headers.append("Authorization: Basic " + base64_encode(user_info) + "\r\n");
        url_str.erase(search_head, new_search_head - search_head + 1); // Deleting "[userinfo@]" from the url
    }

    int port = PortFromUrl(url);
    // std::cout << "PORT FROM REQUEST GEN: " << port << std::endl;
    http_headers += "Host: " + HostnameFromUrl(url) + (port != -1 ? (":" + std::to_string(port)) : "") + "\r\n";

    start_line = method + " " + url_str + " " + protocol_version + "\r\n";
    return start_line + http_headers;

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
    // if(argc != 2)
    // {
    //     std::cout << argc - 1 << " arguments were given, only one was expected, exiting..." << std::endl;
    //     return 0;
    // }

    // json j;
    // std::ifstream i(argv[1]);
    // i >> j;

    // std::cout << "Request: " << std::endl;
    // std::cout << HttpJsonToCstr(j) << std::endl;

    // std::cout << "Response: " << std::endl;
    // std::cout << processHttpRequest(HttpJsonToCstr(j), getHostNameFromJson(j).c_str()) << std::endl;
    // std::cout << "HOST: " << HostnameFromUrl("http://ab:cd@www.google.com:80/anime/isshit?key=lol").c_str() << std::endl;

    // std::cout << "PORT: " << PortFromUrl("http://ab:cd@www.google.com:80/anime/isshit?key=lol") << std::endl;

    std::cout << "HTTP REQUEST: " << std::endl << UrlToHttpRequest("http://ab:cd@www.google.com:80/anime/isshit?key=lol") << std::endl;

    return 0;
}
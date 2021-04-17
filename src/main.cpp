#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <stdexcept>
#include <winsock2.h>
#include <pthread.h>

#include "tools/encryption.hpp"

#define PRINTREQUEST
#define PRINTRESPONSE

std::string HostnameFromUrl(const char* url)
{
    std::string url_str = url;
    int search_head = 0;

    if((search_head = url_str.find( "://", 0)) != std::string::npos)
    {
        url_str.erase(0, search_head + 3);
    }

    if((search_head = url_str.find( "@", 0)) != std::string::npos)
    {
        url_str.erase(0, search_head + 1);
    }

    if((search_head = url_str.find( ":", 0)) != std::string::npos)
        return url_str.substr(0, search_head);

    if((search_head = url_str.find( "/", 0)) != std::string::npos)
        return url_str.substr(0, search_head);

    if((search_head = url_str.find( "?", 0)) != std::string::npos)
        return url_str.substr(0, search_head);
        
    if((search_head = url_str.find( "#", 0)) != std::string::npos)
        return url_str.substr(0, search_head);

    return url_str;
}

int PortFromUrl(const char* url)
{
    std::string url_str = url;
    int search_head = 0;

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
    int search_head = 0;
    int new_search_head = 0;

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
        if(protocol != "http" && protocol != "https")
            throw std::invalid_argument("Error: " + protocol + "protocol provided, expecting http protocol...");

        url_str.erase(search_head, new_search_head - search_head + 3); // Deleting "http://" from the start


        // search_head = new_search_head + 3; // placing search_head after "://"
    }
    search_head = 0;

    //User info
    if((new_search_head = url_str.find( "@", search_head)) != std::string::npos)
    {
        std::string user_info = url_str.substr(search_head, new_search_head - search_head);

        http_headers.append("Authorization: Basic " + base64_encode(user_info) + "\r\n");
        url_str.erase(search_head, new_search_head - search_head + 1); // Deleting "[userinfo@]" from the url
    }

    url_str.erase(0, HostnameFromUrl(url).length());

    int port = PortFromUrl(url);

    http_headers += "Host: " + HostnameFromUrl(url) + (port != -1 ? (":" + std::to_string(port)) : "") + "\r\n";

    http_headers += "Connection: close\r\n";

    start_line = method + " " + url_str + " " + protocol_version + "\r\n";
    return start_line + http_headers + "\r\n";

}

std::string processHttpRequest(const char* url)
{
    WSADATA wsaData;
    int err = WSAStartup(MAKEWORD(2,2), &wsaData); 
    if (err != 0)
    {
        std::cout << "ERROR: WSAStartup failed - " << err << std::endl;
        return 0;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    // struct hostent* host = gethostbyname(hostName);
    struct hostent* host = gethostbyname(HostnameFromUrl(url).c_str());

    SOCKADDR_IN sockAddr;

    int port = PortFromUrl(url);

    sockAddr.sin_port=htons(port != -1 ? port : 80);
    sockAddr.sin_family=AF_INET;
    sockAddr.sin_addr.s_addr = *((unsigned long*)host->h_addr);

    if(connect(sock, (SOCKADDR*)(&sockAddr), sizeof(sockAddr)) != 0) 
    {
        std::cout << "ERROR: Could not connect - " << WSAGetLastError() << std::endl;
        return 0;
    }

    const char* http_request = UrlToHttpRequest(url).c_str();

    send(sock, http_request, strlen(http_request), 0);

    int nDataLength;
    char buffer[10000];
    std::string response = "";
    while((nDataLength = recv(sock, buffer, 10000, 0)) > 0)
    {
        int i = 0;
        for(i = 0; i < nDataLength; i++)
        {
            response+=buffer[i];
        }      
    }

    closesocket(sock);
    WSACleanup();

    #ifdef PRINTREQUEST
        std::cout << "HTTP REQUEST: \n" << UrlToHttpRequest(url).c_str() << std::endl;
    #endif
    #ifdef PRINTRESPONSE
        std::cout << "HTTP RESPONSE: \n" << response << std::endl;
    #endif

    return response;
}

void CreateResponseFile(std::string response)
{
    std::string response_str = response;
    // std::cout << response << std::endl;

    size_t search_head = 0;
    size_t content_length = 0;

    std::string content_type = "";

    if((search_head = response_str.find("Content-Length: ", 0)) == std::string::npos)
        throw std::invalid_argument("ERROR: Response has no content...");
    else
    {
        search_head += ((std::string)"Content-Length: ").length();
        size_t search_tail = response_str.find("\r\n", search_head);

        std::stringstream content_length_stream(response_str.substr(search_head, search_tail - search_head));

        content_length_stream >> content_length;
    }
    search_head = 0;

    if((search_head = response_str.find("Content-Type: ", 0)) == std::string::npos)
        throw std::invalid_argument("ERROR: Unknown content type...");
    else
    {

        if((search_head = response_str.find('/', search_head)) == std::string::npos)
            throw std::invalid_argument("ERROR: Content-Type format incorrect...");
        else
        {
            size_t search_tail = response_str.find("\r\n", search_head);
            content_type = response_str.substr(search_head + 1, search_tail - (search_head + 1));
            if(content_type.length() > 6)
                {
                    size_t search_tail = response_str.find(";", search_head);
                    content_type = response_str.substr(search_head + 1, search_tail - (search_head + 1));
                }
        }
    }

    std::ofstream response_file(("HTTPresponse_content." + content_type).c_str(), std::ios::binary);

    size_t content_start = response_str.find("\r\n\r\n", search_head);
    std::string content = "";

    for(int i = 0; i < content_length; i++)
    {
        content += response_str[content_start + 4 + i];
    }

    response_file.write(content.c_str(), content_length);

    response_file.close();
}

int main(int argc, char **argv)
{
    if(argc != 2)
    {
        std::cout << argc - 1 << " arguments were given, only one was expected, exiting..." << std::endl;
        return 0;
    }

    std::string url = argv[1];

    // std::cout << UrlToHttpRequest(url.c_str()) << std::endl;

    CreateResponseFile(processHttpRequest(url.c_str()));
    return 0;
}
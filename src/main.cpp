#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
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

const char* UriToHttpRequest(const char* url)
{
    std::string url_str = url;
    std::string http_headers = "";
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
        std::cout << "Protocol: " << protocol << std::endl;
        if(protocol != "http")
            throw std::invalid_argument("Error: " + protocol + "protocol provided, expecting http protocol...");

        url_str.erase(search_head, new_search_head - search_head + 3); // Deleting "http://" from the start
        }

    std::cout << "After Protocol url_str: " << url_str << std::endl;

    //User info
    if((new_search_head = url_str.find( "@", search_head)) != std::string::npos)
        {
        std::string user_info = url_str.substr(search_head, new_search_head - search_head);
        std::cout << "Authorization: Basic " + user_info << std::endl;
        http_headers.append("Authorization: Basic " + user_info + "\r\n");
        url_str.erase(search_head, new_search_head - search_head + 1); // Deleting "[userinfo]@" front the start
        }

    std::cout << "After Authentication url_str: " << url_str << std::endl;
    
    //Host name and port
    if(url_str.find( ":", search_head) < url_str.find( "/", search_head))
        {  
        new_search_head = url_str.find( ":", search_head);
        std::string host_name = url_str.substr(search_head, new_search_head - search_head);
        http_headers.append("Host: " + host_name + "\r\n");
        url_str.erase(search_head, new_search_head - search_head);

        if(url_str[search_head] == ':')
            {
            new_search_head = url_str.find( "/", search_head);
            url_str.erase(search_head, new_search_head - search_head);
            }
        }
    else if(url_str.find( ":", search_head) > url_str.find( "/", search_head))
    {
        new_search_head == url_str.find( "/", search_head);
        std::string host_name = url_str.substr(search_head, new_search_head - search_head);
        http_headers.append("Host: " + host_name + "\r\n");
        url_str.erase(search_head, new_search_head - search_head - 1);
    }
    else
    {
        http_headers.append("Host: " + url_str + "\r\n");
        return http_headers.c_str(); // return http request
    }

    //Check if url_str is empty
    if(url_str.empty())
        //return http request

    if((new_search_head = url_str.find( "?", search_head)) != std::string::npos ||
        (new_search_head = url_str.find( "#", search_head)) != std::string::npos)
        {
        // Parse path
        }
    
    //Check if url_str is empty
    if(url_str.empty())
        //return http request

    if((new_search_head = url_str.find( "#", search_head)) != std::string::npos)
        {
        // Parse query
        }
    
    //Check if url_str is empty
    if(!url_str.empty())
        {
        //Parse hash
        }
        
    return http_headers.c_str();
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
    
    std::cout << UriToHttpRequest("http://www.google.com:80") << std::endl;

    return 0;
}
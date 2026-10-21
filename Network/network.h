#ifndef NETWORK_H
#define NETWORK_H

#include <boost/asio.hpp>
#include <vector>
#include <string>

class NetworkServer
{
public:
    NetworkServer(unsigned int port);
    NetworkServer(const NetworkServer&) = delete;
    NetworkServer& operator= (const NetworkServer&) = delete;

    bool acceptConnection ();
    size_t clientsCount ();

    bool send (const std::string& str, size_t id);
    bool receive (std::string& str, size_t id);

    ~NetworkServer ();
private:
    unsigned int port;
    boost::asio::io_context IOservice;

    boost::asio::ip::tcp::acceptor* acceptor = nullptr;
    std::vector <boost::asio::ip::tcp::socket> clientSockets;
};

class NetworkClient
{
public:
    NetworkClient(unsigned int port);
    NetworkClient(const NetworkClient&) = delete;
    NetworkClient& operator= (const NetworkClient&) = delete;

    bool connectTo(std::string ip);

    bool send (const std::string& str);
    bool receive (std::string& str);

    ~NetworkClient ();
private:
    unsigned int port;
    boost::asio::io_context IOservice;

    boost::asio::ip::tcp::socket* sock = nullptr;
};

#endif // NETWORK_H

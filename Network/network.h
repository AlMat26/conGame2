#ifndef NETWORK_H
#define NETWORK_H

#include <boost/asio.hpp>
#include <vector>
#include <string>

struct NetworkClient
{
    boost::asio::ip::tcp::socket* sock;
};

struct NetworkServer
{
    boost::asio::ip::tcp::acceptor* acceptor;
    std::vector <boost::asio::ip::tcp::socket> clientSockets;
};

class Network
{
public:
    Network(unsigned int port, bool isServer);
    void connectTo(std::string ip);
    void acceptConnection ();
    ~Network ();
private:
    bool is_server;
    unsigned int port;
    boost::asio::io_service IOservice;

    NetworkClient client;
    NetworkServer server;
};

#endif

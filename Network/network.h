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

    void recive(char* str);
    void send (char* str);

    void recive(char* str, int id);
    void send (char* str, int id);
    ~Network ();
private:
    bool is_server;
    unsigned int port;
    boost::asio::io_service IOservice;

    NetworkClient client;
    NetworkServer server;
};

#endif

#include "network.h"

using namespace boost::asio;

Network::Network(unsigned int port, bool isServer)
{
    this -> port = port;
    this -> is_server = isServer;
}

void Network::connectTo(std::string ip)
{
    ip::tcp::endpoint endpoint (ip::address::from_string(ip), port);
    client.sock = new ip::tcp::socket (IOservice);
    client.sock -> connect(endpoint);
}

void Network::acceptConnection()
{
    ip::tcp::endpoint endpoint (ip::tcp::v4(), port);
    server.acceptor = new ip::tcp::acceptor (IOservice, endpoint);
    server.clientSockets.push_back(ip::tcp::socket(IOservice));
    server.acceptor -> accept(server.clientSockets[server.clientSockets.size() - 1]);
}

Network::~Network()
{
    if(is_server)
    {
        for (int i = 0; i < server.clientSockets.size(); i++)
        {
            if(server.clientSockets[i].is_open())
            {
                server.clientSockets[i].close();
            }
        }
    } else
    {
        if(client.sock -> is_open())
        {
            client.sock -> close();
        }
    }
}

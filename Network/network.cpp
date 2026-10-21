#include "network.h"

#include <arpa/inet.h>

using namespace boost::asio;

// Сообщение = 4 байта длины (big-endian) + сами данные.
// Ограничение защищает от выделения гигабайт по мусорному заголовку.
static const uint32_t maxMessageSize = 1024 * 1024;

static bool sendMessage(ip::tcp::socket& sock, const std::string& str)
{
    if(str.size() > maxMessageSize)
    {
        return false;
    }

    uint32_t sizeOfStream = htonl(str.size());

    std::vector <const_buffer> buffers;
    buffers.push_back(buffer(&sizeOfStream, sizeof(sizeOfStream)));
    buffers.push_back(buffer(str));

    boost::system::error_code error;
    boost::asio::write(sock, buffers, error);
    return !error;
}

static bool receiveMessage(ip::tcp::socket& sock, std::string& str)
{
    uint32_t sizeOfStream = 0;
    boost::system::error_code error;

    boost::asio::read(sock, buffer(&sizeOfStream, sizeof(sizeOfStream)), error);
    if(error)
    {
        return false;
    }

    sizeOfStream = ntohl(sizeOfStream);
    if(sizeOfStream > maxMessageSize)
    {
        return false;
    }

    str.resize(sizeOfStream);
    boost::asio::read(sock, buffer(str), error);
    return !error;
}

NetworkServer::NetworkServer(unsigned int port)
{
    this -> port = port;
}

bool NetworkServer::acceptConnection()
{
    boost::system::error_code error;

    if(acceptor == nullptr)
    {
        ip::tcp::endpoint endpoint (ip::tcp::v4(), port);
        acceptor = new ip::tcp::acceptor (IOservice);

        acceptor -> open(endpoint.protocol(), error);
        if(!error)
        {
            acceptor -> set_option(ip::tcp::acceptor::reuse_address(true), error);
        }
        if(!error)
        {
            acceptor -> bind(endpoint, error);
        }
        if(!error)
        {
            acceptor -> listen(socket_base::max_listen_connections, error);
        }

        if(error)
        {
            delete acceptor;
            acceptor = nullptr;
            return false;
        }
    }

    clientSockets.push_back(ip::tcp::socket(IOservice));
    acceptor -> accept(clientSockets[clientSockets.size() - 1], error);

    if(error)
    {
        clientSockets.pop_back();
        return false;
    }
    return true;
}

size_t NetworkServer::clientsCount()
{
    return clientSockets.size();
}

bool NetworkServer::send(const std::string& str, size_t id)
{
    if(id >= clientSockets.size())
    {
        return false;
    }
    return sendMessage(clientSockets[id], str);
}

bool NetworkServer::receive(std::string& str, size_t id)
{
    if(id >= clientSockets.size())
    {
        return false;
    }
    return receiveMessage(clientSockets[id], str);
}

NetworkServer::~NetworkServer()
{
    boost::system::error_code error;

    for (size_t i = 0; i < clientSockets.size(); i++)
    {
        if(clientSockets[i].is_open())
        {
            clientSockets[i].close(error);
        }
    }

    delete acceptor;
}

NetworkClient::NetworkClient(unsigned int port)
{
    this -> port = port;
}

bool NetworkClient::connectTo(std::string ip)
{
    boost::system::error_code error;

    ip::address address = ip::make_address(ip, error);
    if(error)
    {
        return false;
    }

    if(sock == nullptr)
    {
        sock = new ip::tcp::socket (IOservice);
    } else if (sock -> is_open())
    {
        sock -> close(error);
    }

    ip::tcp::endpoint endpoint (address, port);
    sock -> connect(endpoint, error);
    return !error;
}

bool NetworkClient::send(const std::string& str)
{
    if(sock == nullptr)
    {
        return false;
    }
    return sendMessage(*sock, str);
}

bool NetworkClient::receive(std::string& str)
{
    if(sock == nullptr)
    {
        return false;
    }
    return receiveMessage(*sock, str);
}

NetworkClient::~NetworkClient()
{
    if(sock != nullptr)
    {
        boost::system::error_code error;
        if(sock -> is_open())
        {
            sock -> close(error);
        }
        delete sock;
    }
}

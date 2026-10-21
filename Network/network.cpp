#include "network.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <cstring>

using namespace boost::asio;

// Сообщение = 4 байта длины (big-endian) + сами данные.
// Ограничение защищает от выделения гигабайт по мусорному заголовку.
static const uint32_t maxMessageSize = 1024 * 1024;

// Сокеты неблокирующие, чтобы сеть не останавливала игровой цикл.
// Отправка всё равно дожидается, пока сообщение уйдёт целиком.
static bool sendMessage(ip::tcp::socket& sock, const std::string& str)
{
    if(str.size() > maxMessageSize)
    {
        return false;
    }

    uint32_t sizeOfStream = htonl(str.size());
    std::string data ((const char*)&sizeOfStream, sizeof(sizeOfStream));
    data += str;

    size_t sent = 0;
    boost::system::error_code error;

    while(sent < data.size())
    {
        sent += sock.write_some(buffer(data.data() + sent, data.size() - sent), error);
        if(error == boost::asio::error::would_block)
        {
            sock.wait(socket_base::wait_write, error);
        }
        if(error)
        {
            return false;
        }
    }
    return true;
}

// Забирает из сокета всё, что уже пришло. false — соединение закрыто
static bool readAvailable(NetworkConnection& connection)
{
    char data[4096];
    boost::system::error_code error;

    while(true)
    {
        size_t length = connection.sock.read_some(buffer(data, sizeof(data)), error);
        if(error == boost::asio::error::would_block)
        {
            return true;
        }
        if(error)
        {
            return false;
        }
        connection.received.append(data, length);
    }
}

static bool extractMessage(std::string& received, std::string& str, bool& broken)
{
    if(received.size() < sizeof(uint32_t))
    {
        return false;
    }

    uint32_t sizeOfStream = 0;
    memcpy(&sizeOfStream, received.data(), sizeof(sizeOfStream));
    sizeOfStream = ntohl(sizeOfStream);

    if(sizeOfStream > maxMessageSize)
    {
        broken = true;
        return false;
    }
    if(received.size() < sizeof(uint32_t) + sizeOfStream)
    {
        return false;
    }

    str = received.substr(sizeof(uint32_t), sizeOfStream);
    received.erase(0, sizeof(uint32_t) + sizeOfStream);
    return true;
}

// true — получено целое сообщение. Если соединение потеряно, closed = true
static bool receiveMessage(NetworkConnection& connection, std::string& str, bool& closed)
{
    bool broken = false;

    if(extractMessage(connection.received, str, broken))
    {
        return true;
    }

    bool alive = readAvailable(connection);
    if(extractMessage(connection.received, str, broken))
    {
        return true;
    }

    closed = !alive || broken;
    return false;
}

static void setupSocket(ip::tcp::socket& sock)
{
    boost::system::error_code error;
    sock.non_blocking(true, error);
    sock.set_option(ip::tcp::no_delay(true), error);
}

static void closeSocket(ip::tcp::socket& sock)
{
    boost::system::error_code error;
    if(sock.is_open())
    {
        sock.shutdown(socket_base::shutdown_both, error);
        sock.close(error);
    }
}

NetworkServer::NetworkServer(unsigned int port)
{
    this -> port = port;
}

bool NetworkServer::start()
{
    if(acceptor != nullptr)
    {
        return true;
    }

    boost::system::error_code error;
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
    if(!error)
    {
        acceptor -> non_blocking(true, error);
    }

    if(error)
    {
        delete acceptor;
        acceptor = nullptr;
        return false;
    }
    return true;
}

int NetworkServer::acceptConnection()
{
    if(acceptor == nullptr)
    {
        return -1;
    }

    NetworkConnection* connection = new NetworkConnection (IOservice);
    boost::system::error_code error;

    acceptor -> accept(connection -> sock, error);
    if(error)
    {
        delete connection;
        return -1;
    }

    setupSocket(connection -> sock);
    clients.push_back(connection);
    return clients.size() - 1;
}

void NetworkServer::disconnect(int id)
{
    if(!isConnected(id))
    {
        return;
    }

    closeSocket(clients[id] -> sock);
    delete clients[id];
    clients[id] = nullptr;
}

bool NetworkServer::isConnected(int id)
{
    return id >= 0 && id < (int)clients.size() && clients[id] != nullptr;
}

bool NetworkServer::send(const std::string& str, int id)
{
    if(!isConnected(id))
    {
        return false;
    }
    return sendMessage(clients[id] -> sock, str);
}

bool NetworkServer::receive(std::string& str, int id)
{
    if(!isConnected(id))
    {
        return false;
    }

    bool closed = false;
    if(receiveMessage(*clients[id], str, closed))
    {
        return true;
    }
    if(closed)
    {
        disconnect(id);
    }
    return false;
}

// Первый IPv4-адрес, кроме 127.0.0.1 — его хост сообщает друзьям
std::string NetworkServer::getLocalAddress()
{
    std::string result = "127.0.0.1";
    ifaddrs* addresses = nullptr;

    if(getifaddrs(&addresses) != 0)
    {
        return result;
    }

    for (ifaddrs* address = addresses; address != nullptr; address = address -> ifa_next)
    {
        if(address -> ifa_addr == nullptr || address -> ifa_addr -> sa_family != AF_INET)
        {
            continue;
        }

        char text[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &((sockaddr_in*)address -> ifa_addr) -> sin_addr, text, sizeof(text));
        if(strcmp(text, "127.0.0.1") != 0)
        {
            result = text;
            break;
        }
    }

    freeifaddrs(addresses);
    return result;
}

NetworkServer::~NetworkServer()
{
    for (size_t i = 0; i < clients.size(); i++)
    {
        disconnect(i);
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

    disconnect();
    connection = new NetworkConnection (IOservice);

    ip::tcp::endpoint endpoint (address, port);
    connection -> sock.connect(endpoint, error);
    if(error)
    {
        disconnect();
        return false;
    }

    setupSocket(connection -> sock);
    return true;
}

void NetworkClient::disconnect()
{
    if(connection == nullptr)
    {
        return;
    }

    closeSocket(connection -> sock);
    delete connection;
    connection = nullptr;
}

bool NetworkClient::isConnected()
{
    return connection != nullptr;
}

bool NetworkClient::send(const std::string& str)
{
    if(connection == nullptr)
    {
        return false;
    }
    return sendMessage(connection -> sock, str);
}

bool NetworkClient::receive(std::string& str)
{
    if(connection == nullptr)
    {
        return false;
    }

    bool closed = false;
    if(receiveMessage(*connection, str, closed))
    {
        return true;
    }
    if(closed)
    {
        disconnect();
    }
    return false;
}

NetworkClient::~NetworkClient()
{
    disconnect();
}

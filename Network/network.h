#ifndef NETWORK_H
#define NETWORK_H

#include <boost/asio.hpp>
#include <vector>
#include <string>

struct NetworkConnection
{
    NetworkConnection(boost::asio::io_context& IOservice) : sock (IOservice) {}

    boost::asio::ip::tcp::socket sock;
    std::string received;
};

class NetworkServer
{
public:
    NetworkServer(unsigned int port);
    NetworkServer(const NetworkServer&) = delete;
    NetworkServer& operator= (const NetworkServer&) = delete;

    bool start ();
    int acceptConnection ();
    void disconnect (int id);
    bool isConnected (int id);

    bool send (const std::string& str, int id);
    bool receive (std::string& str, int id);

    std::string getLocalAddress ();

    ~NetworkServer ();
private:
    unsigned int port;
    boost::asio::io_context IOservice;

    boost::asio::ip::tcp::acceptor* acceptor = nullptr;
    std::vector <NetworkConnection*> clients;
};

class NetworkClient
{
public:
    NetworkClient(unsigned int port);
    NetworkClient(const NetworkClient&) = delete;
    NetworkClient& operator= (const NetworkClient&) = delete;

    bool connectTo(std::string ip);
    void disconnect ();
    bool isConnected ();

    bool send (const std::string& str);
    bool receive (std::string& str);

    ~NetworkClient ();
private:
    unsigned int port;
    boost::asio::io_context IOservice;

    NetworkConnection* connection = nullptr;
};

#endif // NETWORK_H

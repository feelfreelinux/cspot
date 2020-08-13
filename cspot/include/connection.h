#include <sys/socket.h>
#include <vector>
#include <string.h>

enum class ConnectType
{
    Handshake,
    Stream
};

class Connection
{
private:
    int apSock;
    std::vector<unsigned char> partialBuffer;
    ConnectType connectType;

public:
    Connection(std::string apAddress);
    void connect();
    void recvPacket();
    void handshakeCompleted(std::vector<unsigned char>, std::vector<unsigned char>);
    void sendPacket(std::vector<unsigned char>, std::vector<unsigned char>);
};
#include <ixwebsocket/IXWebSocketServer.h>
#include <iostream>

int port = 8008;
std::string host("localhost"); 
ix::WebSocketServer server(port, host);


int main(){

server.setOnClientMessageCallback([](std::shared_ptr<ix::ConnectionState> connectionState, ix::WebSocket & webSocket, const ix::WebSocketMessagePtr & msg) {
    // The ConnectionState object contains information about the connection,
    // at this point only the client ip address and the port.
    std::cout << "Remote ip: " << connectionState->getRemoteIp() << std::endl;

    if (msg->type == ix::WebSocketMessageType::Open)
    {
        std::cout << "New connection" << std::endl;

        // A connection state object is available, and has a default id
        // You can subclass ConnectionState and pass an alternate factory
        // to override it. It is useful if you want to store custom
        // attributes per connection (authenticated bool flag, attributes, etc...)
        std::cout << "id: " << connectionState->getId() << std::endl;

        // The uri the client did connect to.
        std::cout << "Uri: " << msg->openInfo.uri << std::endl;

        std::cout << "Headers:" << std::endl;
        for (auto it : msg->openInfo.headers)
        {
            std::cout << "\t" << it.first << ": " << it.second << std::endl;
        }
    }
    else if (msg->type == ix::WebSocketMessageType::Message)
    {
        // For an echo server, we just send back to the client whatever was received by the server
        // All connected clients are available in an std::set. See the broadcast cpp example.
        // Second parameter tells whether we are sending the message in binary or text mode.
        // Here we send it in the same mode as it was received.
        std::cout << "Received: " << msg->str << std::endl;

        webSocket.send(msg->str, msg->binary);
    }
});

auto res = server.listen();
if (!res.first)
{
    // Error handling
    return 1;
}

// Per message deflate connection is enabled by default. It can be disabled
// which might be helpful when running on low power devices such as a Rasbery Pi
server.disablePerMessageDeflate();

// Run the server in the background. Server can be stoped by calling server.stop()
server.start();

// Block until server.stop() is called.
server.wait();
return 0;}
#include <iostream>
#include <ixwebsocket/IXWebSocketServer.h>
#include <ixwebsocket/IXNetSystem.h>

int main()
{
    ix::initNetSystem();

    int port = 8080;
    ix::WebSocketServer server(port);

    ix::SocketTLSOptions tlsOptions;
    tlsOptions.certFile = "server.crt";
    tlsOptions.keyFile = "private.key";
    tlsOptions.tls = true;
    server.setTLSOptions(tlsOptions);

    server.setOnConnectionCallback(
        [](std::weak_ptr<ix::WebSocket> weakWebSocket,
           std::shared_ptr<ix::ConnectionState> connectionState) {
            auto webSocket = weakWebSocket.lock();
            if (webSocket) {
                webSocket->setOnMessageCallback(
                    [webSocket](const ix::WebSocketMessagePtr& msg) {
                        if (msg->type == ix::WebSocketMessageType::Message) {
                            std::cout << "Received message: " << msg->str << std::endl;
                            webSocket->send("Hello from TLS server!");
                        }
                    });
            }
        });


    auto res = server.listen();
    if (!res.first) {
        std::cerr << "Error: " << res.second << std::endl;
        return 1;
    }

    server.start();
    std::cout << "Server started on port " << port << std::endl;

    server.wait();
    return 0;
}
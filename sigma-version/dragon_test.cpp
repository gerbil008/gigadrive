#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <iostream>
#include <thread>
#include <chrono>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;



void handle_session(tcp::socket socket, net::ssl::context& ctx) {
    try {
        beast::ssl_stream<beast::tcp_stream> stream(std::move(socket), ctx);

        stream.handshake(net::ssl::stream_base::server);
        websocket::stream<beast::ssl_stream<beast::tcp_stream>> ws(std::move(stream));
        ws.accept();

        beast::flat_buffer buffer;
        while (true) {
            ws.read(buffer);
            ws.text(ws.got_text());
            for(int i = 0; i < 1000; i++){
                std::this_thread::sleep_for(std::chrono::seconds(2)); 
                ws.write(buffer.data());
            }
            buffer.clear();
        }
    } catch (const std::exception& e) {
        std::cerr << "Session error: " << e.what() << std::endl;
    }
}

int main() {
    try {
        net::io_context ioc;
        net::ssl::context ctx(net::ssl::context::tlsv12);

        ctx.use_certificate_file("server.crt", net::ssl::context::pem);
        ctx.use_private_key_file("server.key", net::ssl::context::pem);

        tcp::acceptor acceptor(ioc, {tcp::v4(), 8080});
        std::cout << "WebSocket-Server läuft auf wss://localhost:8080" << std::endl;

        while (true) {
            tcp::socket socket(ioc);
            acceptor.accept(socket);
            std::thread(handle_session, std::move(socket), std::ref(ctx)).detach();
        }
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
    }

    return 0;
}

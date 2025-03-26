#include <server_ws.hpp>
#include <iostream>
#include <string>

using namespace std;
using namespace SimpleWeb;

int main() {
    // WebSocket-Server auf Port 8080
    WS::Server server;
    server.config.port = 8080;

    // Endpunkt "/echo" hinzufügen
    auto &echo = server.endpoint["^/echo/?$"];

    // Handler für eingehende Nachrichten
    echo.on_message = [](shared_ptr<WS::Connection> connection, shared_ptr<WS::InMessage> in_message) {
        auto message = in_message->string();
        cout << "Nachricht erhalten: " << message << endl;

        // Nachricht zurück an den Client senden
        auto send_stream = make_shared<WS::OutMessage>();
        *send_stream << "Echo: " << message;
        connection->send(send_stream);
    };

    // Handler für neue Verbindungen
    echo.on_open = [](shared_ptr<WS::Connection> connection) {
        cout << "Neue Verbindung geöffnet: " << connection.get() << endl;
    };

    // Handler für geschlossene Verbindungen
    echo.on_close = [](shared_ptr<WS::Connection> connection, int status, const string &reason) {
        cout << "Verbindung geschlossen: " << connection.get() << " mit Statuscode: " << status << " und Grund: " << reason << endl;
    };

    // Handler für Fehler
    echo.on_error = [](shared_ptr<WS::Connection> connection, const SimpleWeb::error_code &ec) {
        cout << "Fehler in Verbindung: " << connection.get() << " - " << ec.message() << endl;
    };

    // Server starten
    server.start();
    return 0;
}
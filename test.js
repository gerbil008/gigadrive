// WebSocket-Verbindung herstellen
const socket = new WebSocket('wss://gigadrive.ddns.net:12357');
socket.binaryType = 'arraybuffer'; // WebSocket auf Binärdaten vorbereiten

// Datei auswählen und senden
document.getElementById('fileInput').addEventListener('change', function (event) {
    const file = event.target.files[0]; // Erste ausgewählte Datei
    if (file) {
        const reader = new FileReader();
        
        reader.onload = function (e) {
            const arrayBuffer = e.target.result; // Inhalt der Datei als ArrayBuffer
            
            // Sende den binären Inhalt der Datei über den WebSocket
            socket.send(arrayBuffer);
            console.log('PDF-Datei gesendet.');
        };
        
        reader.onerror = function () {
            console.error('Fehler beim Einlesen der Datei.');
        };
        
        // Datei als ArrayBuffer einlesen
        reader.readAsArrayBuffer(file);
    } else {
        console.log('Keine Datei ausgewählt.');
    }
});

// WebSocket-Ereignisse behandeln
socket.onopen = () => console.log('WebSocket-Verbindung geöffnet.');
socket.onmessage = (event) => console.log('Nachricht vom Server:', event.data);
socket.onerror = (error) => console.error('WebSocket-Fehler:', error);
socket.onclose = () => console.log('WebSocket-Verbindung geschlossen.');

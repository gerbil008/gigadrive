let socket = new WebSocket("wss://gigachat.ddns.net:12369");
let socket1 = new WebSocket("wss://gigachat.ddns.net:12368");
let response = "";
let wopened = false;
let running = false;
let identnum;
let wwopened = false;
let response1 = "";
let chunks = [];
let fileName = "";
let totalChunks = 0;

socket1.binaryType =  "arraybuffer";


function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

function csend(msg) {
    socket.send(msg);
}

function msend(msg) {
    socket1.send(msg);
}



async function delete_file(filename) {
    running = true;
    if (wopened) {
        csend("d" + filename);
        while (response == "") {
            await sleep(10);
        }
        if (response == "d") {
            response == "";
            running = false;
            return true;
        }
    }
}

async function list_files(folder) {
    if (wopened) {
        csend("l" + folder);
        while (response == "") {
            await sleep(10);
        }
        let rresponse = response;
        response = "";
        return rresponse;
    }
}


async function _read_file(filename) {
    msend("r/" + filename); // Sende Anfrage an den Server
    let chunks = []; // Array für Datenblöcke
    let active = true;

    while (active) {
        while (response1 == "") {
            await sleep(10); // Warte auf Daten
            console.log(response1);
        }

        if (response1 == "e") {
            // Ende der Dateiübertragung
            const blob = new Blob(chunks); // Erstelle Blob aus den gesammelten Daten
            const a = document.createElement("a");
            a.href = URL.createObjectURL(blob);
            a.download = filename; // Setze den Dateinamen
            a.click(); // Simuliere Klick, um die Datei herunterzuladen
            response1 = ""; // Zurücksetzen
            active = false; // Übertragung beendet
        } else {
            // Binäre Daten empfangen
            const chunk = response1 instanceof ArrayBuffer 
                ? response1 
                : stringToArrayBuffer(response1); // Konvertiere ggf. Hex-String zu ArrayBuffer

            chunks.push(chunk); // Füge Chunk hinzu
            response1 = ""; // Antwort zurücksetzen
        }
    }
}

// Hilfsfunktion: Hex-String in ArrayBuffer umwandeln (falls nötig)
function stringToArrayBuffer(hexString) {
    const byteArray = new Uint8Array(hexString.length / 2);
    for (let i = 0; i < hexString.length; i += 2) {
        byteArray[i / 2] = parseInt(hexString.substr(i, 2), 16);
    }
    return byteArray.buffer;
}



async function write_file() {
    const fileInput = document.getElementById("addFile");
    const file = fileInput.files[0];
    const reader = new FileReader();
    const chunkSize = 2 * 1024 * 1024;
    const totalChunks = Math.ceil(file.size / chunkSize);
    let currentChunk = 0;

    if (wopened) {
        const path = `w/${file.name}%${totalChunks}`;
        csend(path);
        console.log("requested");
        while (response == "") {
            await sleep(10);
            console.log("waiting");
        }
        console.log("got");
        identnum = response;
        response = "";
        console.log("identum", identnum);
        console.log(`Informing server: ${path}`);
    } else {
        console.error("WebSocket is not open!");
        return false;
    }

    const readChunk = (start, end) => {
        return new Promise((resolve, reject) => {
            const blob = file.slice(start, end);
            reader.onload = () => resolve(reader.result);
            reader.onerror = () => reject("Failed to read file chunk.");
            reader.readAsArrayBuffer(blob);
        });
    };

    for (currentChunk = 0; currentChunk < totalChunks; currentChunk++) {
        const start = currentChunk * chunkSize;
        const end = Math.min(start + chunkSize, file.size);

        try {
            const chunkData = await readChunk(start, end);
            if (wwopened) {
                const hexData = arrayBufferToHex(chunkData);
                console.log(hexData);
                msend(identnum + hexData);
                sleep(500);
                console.log(`Sent chunk ${currentChunk + 1} of ${totalChunks}`);
            } else {
                console.error("WebSocket is not open during chunk sending!");
                return false;
            }
        } catch (error) {
            console.error(error);
            return false;
        }

    }

    console.log("All chunks sent.");
    return true;
}

async function make_file(filename) {
    if (wopened) {
        csend("m" + filename);
        while (response == "") {
            await sleep(10);
        }
        if (response == "d") {
            response == ""
            return true;
        }
    }
}

async function _read_file(filename) {
    msend("r/" + filename);
    let chunks = [];
    let active = true;

    while (active) {
        while (response1 === "") {
            await sleep(10); 
        }

        if (response1 === "e") {
            const hexString = chunks.join(""); 
            const byteArray = hexToByteArray(hexString); 
            const blob = new Blob([byteArray]); 

            const a = document.createElement("a");
            a.href = URL.createObjectURL(blob);
            a.download = filename;
            a.click();

            response1 = "";
            active = false;
        } else {
            console.log("Chunk empfangen:", response1);
            chunks.push(response1); 
            response1 = ""; 
        }
    }
}

async function _read_file(filename) {
    msend("r/" + filename); // Sende Anfrage an den Server
    let chunks = []; // Array für Datenblöcke
    let active = true;

    while (active) {
        while (response1 == "") {
            await sleep(10); // Warte auf Daten
            console.log(response1);
        }

        if (response1 == "e") {
            // Ende der Dateiübertragung
            const blob = new Blob(chunks); // Erstelle Blob aus den gesammelten Daten
            const a = document.createElement("a");
            a.href = URL.createObjectURL(blob);
            a.download = filename; // Setze den Dateinamen
            a.click(); // Simuliere Klick, um die Datei herunterzuladen
            response1 = ""; // Zurücksetzen
            active = false; // Übertragung beendet
        } else {
            // Binäre Daten empfangen
            const chunk = response1 instanceof ArrayBuffer 
                ? response1 
                : stringToArrayBuffer(response1); // Konvertiere ggf. Hex-String zu ArrayBuffer

            chunks.push(chunk); // Füge Chunk hinzu
            response1 = ""; // Antwort zurücksetzen
        }
    }
}

async function _read_file(filename){
    msend("r/"+filename);
    let active = true;
    let end_string = "";
    while(active){
        while(response1 == ""){
            await sleep(10);
            console.log(response1);
        }
        if(response1 != "e" && response != ""){
                end_string += response1;
                response1 = "";
        }
        else if(response1 == "e"){
            active = false;
            const a = document.createElement("a");
            a.href = URL.createObjectURL(end_string);
            a.download = filename; 
            a.click();
            response1 = "";
        }
    }
}

function hexToByteArray(hexString) {
    const byteArray = [];
    for (let i = 0; i < hexString.length; i += 2) {
        const byte = parseInt(hexString.substr(i, 2), 16);
        byteArray.push(byte);
    }
    return new Uint8Array(byteArray);
}



socket.onopen = async function (e) {
    wopened = true;
};

socket.onmessage = function (event) {
    response = event.data;
};

socket.onclose = function (event) {
    wopened = false;
    if (event.wasClean) {
        document.getElementById('warningPopupCon').style.display = 'flex';
    } else {
        document.getElementById('warningPopupCon').style.display = 'flex';
    }
};

socket.onerror = function (error) {
    wopened = false;
    document.getElementById('warningPopupCon').style.display = 'flex';
};

socket1.onopen = async function (e) {
    wwopened = true;
};

socket1.onmessage = function (event1) {
    response1 = event1.data;
};

socket1.onclose = function (event1) {
    wwopened = false;
    if (event1.wasClean) {
        document.getElementById('warningPopupCon').style.display = 'flex';
    } else {
        document.getElementById('warningPopupCon').style.display = 'flex';
    }
};

socket1.onerror = function (error) {
    wwopened = false;
    document.getElementById('warningPopupCon').style.display = 'flex';
};
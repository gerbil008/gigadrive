let read_active = false;
let response = "";
let wopened = false;
let running = false;
let identnum;
let wwopened = false;
let response1 = "";
let chunks = [];
let fileName = "";
let totalChunks = 0;
let filename_glob;
let counter = 0;


function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

function csend(msg) {
    socket.send(msg);
}

function msend(msg) {
    socket1.send(msg);
}

sleep(500);
let socket = new WebSocket("wss://gigadrive.ddns.net:12369");
let socket1 = new WebSocket("ws://gigadrive.ddns.net:12368");


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
            //reader.onerror = () => reject("Failed to read file chunk.");
            reader.readAsArrayBuffer(blob);
        });
    };

    for (currentChunk = 0; currentChunk < totalChunks; currentChunk++) {
        const start = currentChunk * chunkSize;
        const end = Math.min(start + chunkSize, file.size);

        try {
            const chunkData = await readChunk(start, end);
            if (wwopened) {
                console.log(chunkData);
                msend(identnum);
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

const chunksize = 2 * 1024 * 1024; // Größe jedes Chunks in Bytes
const send_delay = 50; // Verzögerung in Millisekunden


async function sendChunks() {
    const fileInput = document.getElementById("addFile");
    const file = fileInput.files[0];
    const size = file.size;
    const totalChunks = Math.ceil(size / chunksize);
    console.log(`Total chunks: ${totalChunks}`);
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

    let currentChunk = 0;
    const reader = new FileReader();

    reader.onload = function (event) {
        if (event.target.error) {
            console.error("Error reading file:", event.target.error);
            return;
        }

        const chunkData = event.target.result; // ArrayBuffer des aktuellen Chunks
        console.log(`Sending chunk ${currentChunk + 1} out of ${totalChunks}`);
        const decoder = new TextDecoder();
        const chunkString = decoder.decode(chunkData); // Konvertiert ArrayBuffer zu String
        console.log(identnum);
        console.log(`identsrojufgjh: ${identnum}`);
        // Füge die Identifikationsnummer und die Daten zusammen
        const combinedString = `${identnum}${chunkString}`;

        // Sende die kombinierte Nachricht über den WebSocket
        socket1.send(combinedString);

        // Sende die kombinierten Daten
        console.log(combinedString);
        console.log(`Sending chunk ${currentChunk + 1} out of ${totalChunks}`);

        //socket1.send(identnum+chunkData);

        currentChunk++;

        if (currentChunk < totalChunks) {
            setTimeout(() => readNextChunk(), send_delay);
        } else {
            console.log("All chunks sent. Sending completion message.");
        }
    };

    function readNextChunk() {
        const start = currentChunk * chunksize;
        const end = Math.min(start + chunksize, size);
        const blob = file.slice(start, end);

        // Lies den nächsten Chunk als ArrayBuffer
        reader.readAsArrayBuffer(blob);
    }

    // Start der ersten Leseoperation
    readNextChunk();
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
    filename_glob = filename;
    read_active = true;
}

async function pinger() {
    msend("p69");
}


setInterval(pinger, 3000);



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
    console.log(event1.data);
    if (read_active) {
        if (response1 == "e") {
            const blob = new Blob(chunks);
            const a = document.createElement("a");
            a.href = URL.createObjectURL(blob);
            a.download = filename_glob;
            a.click();
            response1 = "";
            active = false;
            read_active = false;
            counter = 0;
        } else {
            counter++;
            const chunk = response1;
            chunks.push(chunk);
            response1 = "";
            console.log(counter);
        }
    }
};

socket1.onclose = function (event1) {
    wwopened = false;
    console.log("closed file");
    if (event1.wasClean) {
        document.getElementById('warningPopupCon').style.display = 'flex';
    } else {
        document.getElementById('warningPopupCon').style.display = 'flex';
    }
};

socket1.onerror = function (error) {
    console.log("error file");
    console.log(error);
    wwopened = false;
    document.getElementById('warningPopupCon').style.display = 'flex';
};
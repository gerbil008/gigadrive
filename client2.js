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
let _chunks_total_send;
let _chunks_send;
let _chunks_total_receive;
let _chunks_receive;
const _chunks_multiple = 3;

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
let socket = new WebSocket("ws://localhost:12369");
let socket1 = new WebSocket("ws://localhost:12368");

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

const chunksize = 2 * 1024 * 1024; 
const send_delay = 200; 

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

async function sendChunks() {
    console.log("Start sending");
    const fileInput = document.getElementById("addFile");
    const file = fileInput.files[0];
    const size = file.size;
    const totalChunks = Math.ceil(size / chunksize);
    _chunks_total_send = totalChunks*_chunks_multiple;

    _chunks_send = 0;

    if (wopened) {
        const path = `w/${file.name}%${totalChunks}`;
        csend(path);
        while (response === "") {
            await sleep(10);
        }
        identnum = response;
        response = "";
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

        const chunkData = new Uint8Array(event.target.result);
        const identnumBytes = new TextEncoder().encode(identnum);
        const combined = new Uint8Array(identnumBytes.length + chunkData.length);
        combined.set(identnumBytes, 0);
        combined.set(chunkData, identnumBytes.length);
        console.log(combined);
        socket1.send(combined);

        currentChunk++;
        _chunks_send++;

        if (currentChunk < totalChunks) {
            setTimeout(() => readNextChunk(), send_delay);
        }
    };

    function readNextChunk() {
        const start = currentChunk * chunksize;
        const end = Math.min(start + chunksize, size);
        const blob = file.slice(start, end);
        reader.readAsArrayBuffer(blob);
    }

    readNextChunk();
}

async function sigma_counter(){
    while(true){
    sleep(0.7);
    if(_chunks_send != _chunks_total_send -1){
            _chunks_send++;
    }
    else{
        break
    }}
}

async function _read_file(filename) {
    _chunks_receive = 0;
    _chunks_total_receive
    msend("r/" + filename);
    filename_glob = filename;
    //await createFile(filename);
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
    if(response1 == "d"){
        response1 = "";
        _chunks_send = _chunks_total_send;
    }
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
            _chunks_receive = 0;
            _chunks_total_receive = 0;
        } else if (response1 != ""){
            counter++;
            const chunk = response1;
            chunks.push(chunk);
            response1 = "";
            console.log(counter);
            _chunks_receive++;
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
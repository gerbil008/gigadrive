let socket = new WebSocket("wss://gigachat.ddns.net:12369");
let socket1 =  new WebSocket("wss://gigachat.ddns.net:12368");
let response = "";
let wopened = false;
let running = false;
let identnum;
let wwopened = false;
let response1 = "";

function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

function csend(msg){
    socket.send(msg);
}

function msend(msg){
    socket1.send(msg);
}



async function delete_file(filename){
    running = true;
    if(wopened){
        csend("d"+filename);
        while(response == ""){
            await sleep(10);
        }
        if(response == "d"){
            response == "";
            running = false;
            return true;
        }
    }
}

async function list_files(folder){
    if(wopened){
        csend("l"+folder);
    while(response == ""){
        await sleep(10);
    }
    let rresponse = response;
    response = "";
    return rresponse;}
}


async function read_file(filename){
    running = true;
    if(wopened){
        console.log(filename);
    let path = "r/" + user + "/" + filename;
    csend(path);
    while(response == ""){
        await sleep(10);
    }
    let rresponse = response;
    const a = document.createElement("a");
    a.href = URL.createObjectURL(response);
    a.download = filename;
    a.click();
    response = "";
    running = false;
    return rresponse;}

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
        while(response1 == ""){
            await sleep(10);
        }
        identnum = response;
        response1 = "";
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
                msend(identnum+chunkData); 
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




async function make_file(filename){
    if(wopened){
        csend("m"+filename);
        while(response == ""){
            await sleep(10);
        }
        if(response == "d"){
            response == ""
            return true;
        }
    }}


socket.onopen = async function(e) {
    wopened = true;
};

socket.onmessage = function(event) {
    response = event.data;
};

socket.onclose = function(event) {
    wopened = false;
  if (event.wasClean) {
    document.getElementById('warningPopupCon').style.display = 'flex';
  } else {
    document.getElementById('warningPopupCon').style.display = 'flex';
  }
};

socket.onerror = function(error) {
    wopened = false;
    document.getElementById('warningPopupCon').style.display = 'flex';
};

socket1.onopen = async function(e) {
    wwopened = true;
};

socket1.onmessage = function(event) {
    response1 = event.data;
};

socket1.onclose = function(event) {
    wwopened = false;
  if (event.wasClean) {
    document.getElementById('warningPopupCon').style.display = 'flex';
  } else {
    document.getElementById('warningPopupCon').style.display = 'flex';
  }
};

socket1.onerror = function(error) {
    wwopened = false;
    document.getElementById('warningPopupCon').style.display = 'flex';
};
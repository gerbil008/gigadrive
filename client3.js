async function write_file() {
    const fileInput = document.getElementById("addFile");
    const file = fileInput.files[0];
    const reader = new FileReader();
    const chunkSize = 2 * 1024 * 1024; 
    const totalChunks = Math.ceil(file.size / chunkSize);
    let currentChunk = 0;

    if (wopened) {
        const path = `w/${user.name}/${file.name}%${totalChunks}`;
        csend(path);
        console.log(`Informing server: ${path}`);
    } else {
        console.error("WebSocket is not open!");
        return false;
    } 

    const readChunk = async () => {
        if (currentChunk >= totalChunks) {
            console.log("All chunks sent.");
            return true; 
        }

        const start = currentChunk * chunkSize;
        const end = Math.min(start + chunkSize, file.size);
        const blob = file.slice(start, end);

        reader.onload = () => {
            if (wopened) {
                const packet = `chunk/${currentChunk + 1}/${totalChunks}`;
                csend(packet); 
                csend(reader.result); 
                console.log(`Sent chunk ${currentChunk + 1} of ${totalChunks}`);
                currentChunk++;
                readChunk(); 
            } else {
                console.error("WebSocket is not open during chunk sending!");
                return false;
            }
        };

        reader.readAsArrayBuffer(blob);
    };

    readChunk();

    while (response == "") {
        await sleep(10);
    }
    if (response == "d") {
        response = "";
        running = false;
        return true;
    }
}

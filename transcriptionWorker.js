const { parentPort, workerData } = require('worker_threads');
const addon = require('./build/Debug/addon.node.node');
const transcriber = new addon.NodeTranscription();

function handleTranscriptionData(data) {
    ret = JSON.stringify({
        "type": "transcription",
        "data": data
    });

    self.postMessage(ret);
}

async function startTranscription(settings) {
     transcriber.start(handleTranscriptionData, settings);
}   

self.onmessage = (event) => {
    evt = JSON.parse(event.data);

    switch (evt.action) {
        case "loadModel":
            settings = evt.data;
            ret = transcriber.loadModel(settings);
            break;
        case "start":
            settings = evt.data;
            Promise.resolve().then(() => {
                startTranscription(settings);
            }).catch(error => {
                console.error('Transcription error:', error);
            });

            break;

        case "stop":
            console.log("call stop");
            transcriber.stop();
            console.log("call stop");

            break;

        case "freeModel":
            transcriber.freeModel();
            break;
        default:
            break;
    }
};

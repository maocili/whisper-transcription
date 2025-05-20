const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('electronAPI', {
    // Existing API methods
    readFile: (filePath) => ipcRenderer.invoke('read-file', filePath),
    writeFile: (filePath, data, encoding) => 
        ipcRenderer.invoke('write-file', { filePath, data, encoding }),
    readFileSync: (filePath) => ipcRenderer.sendSync('read-file-sync', filePath),
    readDir: (dirPath) => ipcRenderer.invoke('read-dir', dirPath),
    readDirSync: (dirPath) => ipcRenderer.sendSync('read-dir-sync', dirPath),
    loadModel: (settings) => ipcRenderer.invoke('load-model', settings),
});

window.addEventListener('DOMContentLoaded', () => {
    // Your existing preload code if any
});


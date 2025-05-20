const { app, BrowserWindow, ipcMain } = require('electron');
const path = require('path');
const fs = require('fs').promises;
const fsSync = require('fs');

let mainWindow;

// Create a function to handle transcription data
function handleTranscriptionData(data) {
    console.log("handleTranscriptionData", data);
    mainWindow.webContents.send('message-from-main', 'Hello from Main Process!')
}

function createWindow() {
    mainWindow = new BrowserWindow({
        width: 1200,
        height: 800,
        webPreferences: {
            nodeIntegration: true,
            contextIsolation: true,
            nodeIntegrationInWorker: true,
            preload: path.join(__dirname, 'preload.js')
        }
    });

    mainWindow.DevToolsExtension = true; // Enable DevTools extension
    mainWindow.webContents.openDevTools();

    mainWindow.loadFile('index.html');
}

app.whenReady().then(() => {
    createWindow();
    app.on('activate', function () {
        if (BrowserWindow.getAllWindows().length === 0) createWindow();
    });

});

app.on('window-all-closed', function () {
    if (process.platform !== 'darwin') app.quit();
});

// Transcription control handlers
ipcMain.handle('start-transcription', (event, settings) => {
    t.start(handleTranscriptionData, settings);
    return { success: true, message: 'Transcription started' };
});

ipcMain.handle('stop-transcription', async (event) => {
    try {
        t.stop();
        return { success: true, message: 'Transcription stopped' };
    } catch (error) {
        console.error('Error stopping transcription:', error);
        throw new Error('Failed to stop transcription');
    }
});

// IPC handlers for file operations
ipcMain.handle('read-file', async (event, filePath) => {
    try {
        // Ensure the file path is absolute and normalized
        const normalizedPath = path.normalize(filePath);
        const content = await fs.readFile(normalizedPath, 'utf8');
        return { success: true, content };
    } catch (error) {
        console.error('Error reading file:', error);
        return { success: false, error: error.message };
    }
});

ipcMain.on('read-file-sync', (event, filePath) => {
    try {
        // Ensure the file path is absolute and normalized
        const normalizedPath = path.normalize(filePath);
        const content = fsSync.readFileSync(normalizedPath, 'utf8');
        event.returnValue = { success: true, content };
    } catch (error) {
        console.error('Error reading file synchronously:', error);
        event.returnValue = { success: false, error: error.message };
    }
});

ipcMain.handle('write-file', async (event, { filePath, data, encoding = 'utf8' }) => {
    try {
        const normalizedPath = path.normalize(filePath);

        // Create directory if it doesn't exist
        const directory = path.dirname(normalizedPath);
        await fs.mkdir(directory, { recursive: true });

        // Write the file
        if (data instanceof Buffer) {
            await fs.writeFile(normalizedPath, data);
        } else if (typeof data === 'string') {
            await fs.writeFile(normalizedPath, data, encoding);
        } else {
            // If data is an object or array, stringify it
            await fs.writeFile(normalizedPath, JSON.stringify(data, null, 2), encoding);
        }

        return { success: true, message: 'File written successfully' };
    } catch (error) {
        console.error('Error writing file:', error);
        return { success: false, error: error.message };
    }
});

// IPC handlers for directory operations
ipcMain.handle('read-dir', async (event, dirPath) => {
    try {
        const normalizedPath = path.normalize(dirPath);
        const files = await fs.readdir(normalizedPath, { withFileTypes: true });
        const results = files.map(file => ({
            name: file.name,
            isDirectory: file.isDirectory(),
            isFile: file.isFile(),
            path: path.join(normalizedPath, file.name)
        }));
        return { success: true, files: results };
    } catch (error) {
        console.error('Error reading directory:', error);
        return { success: false, error: error.message };
    }
});

ipcMain.on('read-dir-sync', (event, dirPath) => {
    try {
        const normalizedPath = path.normalize(dirPath);
        const files = fsSync.readdirSync(normalizedPath, { withFileTypes: true });
        const results = files.map(file => ({
            name: file.name,
            isDirectory: file.isDirectory(),
            isFile: file.isFile(),
            path: path.join(normalizedPath, file.name)
        }));
        event.returnValue = { success: true, files: results };
    } catch (error) {
        console.error('Error reading directory synchronously:', error);
        event.returnValue = { success: false, error: error.message };
    }
});

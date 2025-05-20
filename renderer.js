// run transcription worker
var worker = new Worker('transcriptionWorker.js');


// DOM Elements
const sideBarHeader = document.getElementById('sidebar-header');
const sideBarContent = document.getElementById('sidebar-content');


const sideBarHeaderMap = {
    "sessions": `<span>Sessions</span>
    <button id="newSessionBtn" class="action-button" title="New Session" style="background: #28a745; color: white; width: 100%; margin-top: 8px; padding: 8px;">
        New Session
    </button>`,

    "settings": `<span>Settings</span>
     <button id="saveSettings" class="action-button" title="Save" style="background: #28a745; color: white;" onclick="clickSaveSettings()">
        <svg width="16" height="16" viewBox="0 0 16 16" fill="currentColor">
            <path d="M13.78 4.22a.75.75 0 010 1.06l-7.25 7.25a.75.75 0 01-1.06 0L2.22 9.28a.75.75 0 011.06-1.06L6 10.94l6.72-6.72a.75.75 0 011.06 0z"/>
        </svg>
    </button>
    `,
}

const sideBarMap = {
    "sessions": sessionsSideBarContent,
    "settings": settingsSideBarContent
}

function showSideBar(tag) {
    sideBarHeader.innerHTML = "";
    sideBarContent.innerHTML = '';
    sideBarMap[tag]();
}

// session function
function sessionsSideBarContent() {
    const sessionHeader = document.createElement('div');
    sessionHeader.innerHTML = sideBarHeaderMap["sessions"];
    sessionHeader.addEventListener('click', (e) => {
        createSession();
    });

    var sessions = getSessions();

    const sessionList = document.createElement('div');

    sessions.forEach(session => {
        if (session.deleted) {
            return;
        }

        const element = document.createElement('div');
        element.className = 'session-item';
        element.setAttribute('data-id', session.id);

        element.innerHTML = `
            <div style="flex: 1; min-width: 0;">
                <div style="white-space: nowrap; overflow: hidden; text-overflow: ellipsis;">${session.name}</div>
                <div style="font-size: 11px; color: #666;">${session.date}</div>
            </div>
            <div class="session-actions" style="display: flex; gap: 4px; opacity: 0.6;">
                <button class="action-button" onclick="renameSession(${session.id})" title="Rename">
                    <svg width="16" height="16" viewBox="0 0 16 16" fill="currentColor">
                        <path d="M13.23 1h-1.46L3.52 9.25l-.16.22L1 13.59 2.41 15l4.12-2.36.22-.16L15 4.23V2.77L13.23 1zM2.41 13.59l1.51-3 1.45 1.45-2.96 1.55z"/>
                    </svg>
                </button>
                <button class="action-button" onclick="deleteSession(${session.id})" title="Delete">
                    <svg width="16" height="16" viewBox="0 0 16 16" fill="currentColor">
                        <path d="M8 8.707l3.646 3.647.708-.707L8.707 8l3.647-3.646-.707-.708L8 7.293 4.354 3.646l-.707.708L7.293 8l-3.646 3.646.707.708L8 8.707z"/>
                    </svg>
                </button>
            </div>
        `;
        element.addEventListener('click', (e) => {
            selectSession(session.id);
        });
        sessionList.appendChild(element);
    })

    sideBarHeader.appendChild(sessionHeader);
    return sideBarContent.appendChild(sessionList);
}

function getSessions() {
    // const saved = localStorage.getItem('transcription-sessions');
    // return saved ? JSON.parse(saved) : [];
    let files = readDirSync("sessions");

    var sessionsList = [];
    files.forEach(f => {
        raw = readFileSync(f.path)
        sessionsList.push(JSON.parse(raw));
    });
    return sessionsList;

}

function getSessionById(id) {
    let path = "sessions/" + id + ".json";
    let content = readFileSync(path)
    return JSON.parse(content);
}

function saveSession(session) {
    path = "sessions/" + session.id + ".json";
    writeFile(path, session);
}

function selectSession(id) {
    setCurrentSession(id);
    let s = getSessionById(id)
    showTranscription(s);
}

function createSession() {
    const session = {
        id: Date.now(),
        name: `New Session`,
        date: new Date().toLocaleString(),
        transcript: '',
    };

    setCurrentSession(session.id);
    saveSession(session);
    showSideBar("sessions");
    return session;
}

function deleteSession(id) {
    s = getSessionById(id);
    s.deleted = Date.now();
    saveSession(s);
    showSideBar("sessions");
}

// settings function
function settingsSideBarContent() {
    var settings = loadSettings();

    var currentSettingsList = document.createElement('div');
    currentSettingsList.className = 'settings-sidebar';
    currentSettingsList.style.cssText = `
        overflow: auto;
        height: 100%;
        padding: 8px;
        display: flex;
        flex-direction: column;
        gap: 4px;
    `;

    // 创建设置组
    const groups = {
        'Model Settings': ['modelPath', 'language'],
        'Performance': ['nThreads', 'useGpu'],
        'Audio Processing': ['stepMs', 'lengthMs', 'vadThreshold'],
        'Advanced Options': ['translate', 'noTimestamps', 'tinyDiarize']
    };

    for (const [groupName, keys] of Object.entries(groups)) {
        const groupDiv = document.createElement('div');
        groupDiv.className = 'settings-group';
        groupDiv.style.cssText = `
            margin-bottom: 16px;
            background: var(--bg-color);
            border-radius: 6px;
            border: 1px solid var(--border-color);
        `;

        // 添加组标题
        const groupTitle = document.createElement('div');
        groupTitle.style.cssText = `
            padding: 8px 12px;
            font-size: 12px;
            font-weight: 600;
            color: var(--text-color);
            background: var(--active-bg);
            border-bottom: 1px solid var(--border-color);
            border-radius: 6px 6px 0 0;
        `;
        groupTitle.textContent = groupName;
        groupDiv.appendChild(groupTitle);

        // 添加组内设置项
        const groupContent = document.createElement('div');
        groupContent.style.cssText = `padding: 12px;`;

        keys.forEach(key => {
            if (key in settings) {
                const settingItem = document.createElement('div');
                settingItem.className = 'settings-item';
                settingItem.style.cssText = `
                    margin-bottom: 12px;
                    display: flex;
                    flex-direction: column;
                    gap: 4px;
                `;

                const label = document.createElement('label');
                label.setAttribute('for', key);
                label.style.cssText = `
                    font-size: 12px;
                    color: var(--text-color);
                    display: flex;
                    justify-content: space-between;
                    align-items: center;
                `;
                label.textContent = key;

                const inputWrapper = document.createElement('div');
                let inputType;
                if (typeof settings[key] === 'boolean') {
                    intputd = document.createElement('div');
                    inputType = 'checkbox';
                    intputd.innerHTML = `<input class="setting-item-value"  type=${inputType} style="margin: 0px; width: 16px; height: 16px;" id=${key} checked=${settings[key]} >`;
                    label.appendChild(intputd);
                } else {

                    intputd = document.createElement('div');
                    inputType = typeof settings[key] === 'number' ? 'number' : 'text';
                    intputd.innerHTML = `<input class="setting-item-value"  type="${inputType}" id="${key}"  value=${settings[key]} style="width: 100%; background: var(--input-bg); color: var(--text-color); border: 1px solid var(--border-color); padding: 6px 8px; border-radius: 4px; font-size: 12px;">`
                    inputWrapper.appendChild(intputd);
                }

                settingItem.appendChild(label);
                if (inputType !== 'checkbox') {
                    settingItem.appendChild(inputWrapper);
                }
                groupContent.appendChild(settingItem);
            }
        });

        groupDiv.appendChild(groupContent);
        currentSettingsList.appendChild(groupDiv);
    }

    return sideBarContent.appendChild(currentSettingsList);
}

const settingsStorageKey = "whisper-settings";
function loadSettings() {
    const defaults = {
        "modelPath": 'models/ggml-tiny.en.bin',
        "language": 'en',
        "nThreads": Math.min(4, window.navigator.hardwareConcurrency || 4),
        "stepMs": 3000,
        "lengthMs": 10000,
        "keepMs": 200,
        "captureId": 1,
        "maxTokens": 32,
        "audioCtx": 0,
        "beamSize": -1,
        "vadThreshold": 0.6,
        "freqThreshold": 100.0,
        "translate": false,
        "noFallback": false,
        "printSpecial": false,
        "noContext": true,
        "noTimestamps": false,
        "tinyDiarize": false,
        "saveAudio": false,
        "useGpu": true,
        "flashAttn": false
    };

    const saved = JSON.parse(readFileSync("settings.json"))
    if (saved != null) {
        if ("modelPath" in saved) {
            return saved
        }
    }
    return defaults;
}

function saveSettings() {
    let currentSessions = loadSettings();
    let items = document.getElementsByClassName("setting-item-value")
    for (let i = 0; i < items.length; i++) {
        const item = items[i];
        if (item.type === 'checkbox') {
            currentSessions[item.id] = item.checked;
        } else {
            currentSessions[item.id] = item.value;
        }
    }

    writeFile("settings.json", currentSessions);
}

// transcription function
function showTranscription(session) {
    const id = getCurrentSessionID();
    const activeSessionName = document.getElementById('activeSessionName');
    const transcriptDisplay = document.getElementById('transcriptDisplay');

    activeSessionName.textContent = session.name;
    transcriptDisplay.innerHTML = session.transcript;
}

// key-value 
const currentSession = "currentSession";

function getCurrentSessionID() {
    return localStorage.getItem(currentSession);
}

function setCurrentSession(id) {
    return localStorage.setItem(currentSession, id);
}


// Record button click handler
let isRecording = false;
document.getElementById("recordButton").addEventListener('click', () => {
    const currentSessionID = getCurrentSessionID();
    const recordButton = document.getElementById("recordButton");
    if (!isRecording) {
        recordButton.style.background = '#dc3545';
        recordButton.innerHTML = 'Stop Recording';
        isRecording = true;
        loadModel();
        startTranscription();
    } else {
        recordButton.style.background = '#28a745';
        recordButton.innerHTML = 'Start Recording';
        isRecording = false;
        stopTranscription();
    }
});

// File reading functions
async function readFile(filePath) {
    try {
        const result = await window.electronAPI.readFile(filePath);
        if (result.success) {
            return result.content;
        } else {
            console.error('Failed to read file:', result.error);
            throw new Error(result.error);
        }
    } catch (error) {
        console.error('Error reading file:', error);
        throw error;
    }
}

/**
 * Read a file synchronously
 * @param {string} filePath - The path of the file to read
 * @param {string} [defaultValue=null] - Value to return if file reading fails
 * @returns {string|null} The file content or defaultValue if reading fails
 */
function readFileSync(filePath, defaultValue = null) {
    try {
        const result = window.electronAPI.readFileSync(filePath);
        if (result.success) {
            return result.content;
        } else {
            console.error('Failed to read file:', result.error);
            return defaultValue;
        }
    } catch (error) {
        console.error('Error reading file synchronously:', error);
        return defaultValue;
    }
}

/**
 * Write data to a file
 * @param {string} filePath - The path to write the file to
 * @param {string|Buffer|Object} data - The data to write
 * @param {string} [encoding='utf8'] - The encoding to use (when writing strings)
 * @returns {Promise<void>}
 */
async function writeFile(filePath, data, encoding = 'utf8') {
    try {
        const result = await window.electronAPI.writeFile(filePath, data, encoding);
        if (!result.success) {
            throw new Error(result.error);
        }
        return result;
    } catch (error) {
        console.error('Error writing file:', error);
        throw error;
    }
}

/**
 * Read directory contents asynchronously
 * @param {string} dirPath - The path of the directory to read
 * @returns {Promise<Array>} Array of file and directory information
 */
async function readDir(dirPath) {
    try {
        const result = await window.electronAPI.readDir(dirPath);
        if (result.success) {
            return result.files;
        } else {
            console.error('Failed to read directory:', result.error);
            throw new Error(result.error);
        }
    } catch (error) {
        console.error('Error reading directory:', error);
        throw error;
    }
}

/**
 * Read directory contents synchronously
 * @param {string} dirPath - The path of the directory to read
 * @param {Array} [defaultValue=[]] - Value to return if reading fails
 * @returns {Array} Array of file and directory information
 */
function readDirSync(dirPath, defaultValue = []) {
    try {
        const result = window.electronAPI.readDirSync(dirPath);
        if (result.success) {
            return result.files;
        } else {
            console.error('Failed to read directory:', result.error);
            return defaultValue;
        }
    } catch (error) {
        console.error('Error reading directory synchronously:', error);
        return defaultValue;
    }
}

function loadModel() {
    let settings = loadSettings();
    worker.postMessage(JSON.stringify({
        action: 'loadModel',
        data: settings,
    }));
}

function startTranscription() {
    let settings = loadSettings();
    worker.postMessage(JSON.stringify({
        action: 'start',
        data: settings,
    }));
}

function stopTranscription() {
    worker.terminate();
    worker = new Worker('transcriptionWorker.js');
}

function handleWorkerMessage(event) {
    const evt = JSON.parse(event.data);
    switch (evt.type) {
        case "transcription":
            const data = evt.data;
            updateTranscriptionDisplay(data);
    }
}

worker.onmessage = (event) => {
    handleWorkerMessage(event);
}

function test() {
    loadModel();
    startTranscription();
}

function updateTranscriptionDisplay(transcript) {
    currentId = getCurrentSessionID();
    const transcriptDisplay = document.getElementById('transcriptDisplay');
    transcriptDisplay.innerHTML += transcript;
    session = getSessionById(currentId)
    session.transcript = transcriptDisplay.innerHTML;

    saveSession(session);
}


// botton function
function clickSaveSettings() {
    saveSettings();
    alert("Settings saved successfully!");
}

// Activity Bar Navigation
document.getElementById('sessionsButton').addEventListener('click', () => {
    showSideBar('sessions');
});

document.getElementById('settingsButton').addEventListener('click', () => {
    showSideBar('settings');
});

// init windows
showSideBar('sessions');

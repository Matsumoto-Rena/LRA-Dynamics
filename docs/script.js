const HEADER_MANUAL = 0x01, HEADER_AUTO = 0x02, HEADER_PARAM = 0x03, HEADER_MANUAL2 = 0x04, DIR_STOP = 0;
const HEADER_VOLTAGE = 0x05; 
let videoElement, canvas, ctx;
let bleDevice, bleCharacteristic;
let cv; // cvReadyは使わず、cv変数の有無で管理

let currentMode = 3, isSending = false, isVideoFileMode = false;
let lastCommandId = -1; // 直前に送ったコマンドを記憶する変数
let lastSendTime = 0;   // 最後に送信した時間
let recordedData = [], isRecording = false, recordStartTime = 0;

let targetX = -1, targetY = -1; // 目標地点の絶対座標（-1なら設定なし）
const TARGET_TOLERANCE = 20;    // 目標にどれくらい近づけばOKとするか（ピクセル）
let normX=0, normY=0;
let pixelX=0, pixelY=0;
let latestAngle=0, currentDirStr="";

const COLOR_CENTER_LOW = [20, 100, 100]; 
const COLOR_CENTER_HIGH = [40, 255, 255];
const COLOR_FRONT_LOW1 = [0, 120, 70];   
const COLOR_FRONT_HIGH1 = [10, 255, 255];
const COLOR_FRONT_LOW2 = [165, 120, 70]; 
const COLOR_FRONT_HIGH2 = [180, 255, 255];

function waitForOpenCV() {
    if (window.cv && window.cv.Mat) {
        cv = window.cv;
        document.getElementById('loading').style.display = 'none';
    } else {
        setTimeout(waitForOpenCV, 500);
    }
}

window.onload = () => {
    videoElement = document.getElementById('videoElement');
    canvas = document.getElementById('canvas');
    ctx = canvas.getContext('2d', { willReadFrequently: true });

    canvas.addEventListener('mousedown', (e) => {
        const rect = canvas.getBoundingClientRect();
        // キャンバス内のクリック位置を計算
        const x = e.clientX - rect.left;
        const y = e.clientY - rect.top;
        
        // 実際の解像度(640x480)に合わせて座標変換
        const scaleX = canvas.width / rect.width;
        const scaleY = canvas.height / rect.height;
        
        targetX = Math.round(x * scaleX);
        targetY = Math.round(y * scaleY);
        
        console.log(`目標セット: ${targetX}, ${targetY}`);
    });

    document.getElementById('cameraButton').onclick = startWebcam;
    document.getElementById('connectButton').onclick = connectBluetooth;
    document.getElementById('btn-update').onclick = sendParamUpdate;
    document.getElementById('btn-voltage').onclick = requestVoltage;
    
    [1,2,3,4,5].forEach(m => document.getElementById(`mode${m}Btn`).onclick = () => setMode(m));

    const videoInput = document.getElementById('videoInput');
    videoInput.addEventListener('change', handleFileSelect, false);
    
    document.getElementById('v-play').onclick = () => videoElement.paused ? videoElement.play() : videoElement.pause();
    document.getElementById('v-reset').onclick = () => { videoElement.currentTime = 0; videoElement.pause(); recordedData=[]; updateLogCount(); };
    document.getElementById('saveBtn').onclick = saveCSV;
    document.getElementById('clearBtn').onclick = () => { recordedData=[]; updateLogCount(); };

    setupDpad();
    setMode(3); 
    
    waitForOpenCV();
    requestAnimationFrame(processLoop);
};

async function startWebcam() {
    if (!cv) {
        alert("OpenCVのロードが終わっていません。画面上の「ロード中...」が消えてからもう一度押してください！");
        return;
    }
    try {
        isVideoFileMode = false;
        const stream = await navigator.mediaDevices.getUserMedia({ video: { width: 640, height: 480 }, audio: false });
        videoElement.srcObject = stream;
        
        videoElement.setAttribute('autoplay', '');
        videoElement.setAttribute('muted', '');
        videoElement.setAttribute('playsinline', '');
        
        videoElement.play(); 
        canvas.style.display = 'block';
        document.getElementById('cameraButton').textContent = "Webcam動作中";
        if(currentMode === 4) setMode(3);
    } catch (err) { 
        alert("カメラエラー: " + err); 
    }
}

function handleFileSelect(event) {
    const file = event.target.files[0];
    if (!file) return;
    isVideoFileMode = true;
    videoElement.srcObject = null;
    videoElement.src = URL.createObjectURL(file);
    videoElement.load();
    canvas.style.display = 'block';
    document.getElementById('cameraButton').textContent = "動画ファイルモード";
    setMode(4); recordedData = []; updateLogCount();
}

function setMode(mode) {
    currentMode = mode;
    document.querySelectorAll('.tab').forEach(b => b.classList.remove('active'));
    document.getElementById(`mode${mode}Btn`).classList.add('active');

    const dpad = document.getElementById('dpad-area');
    const saveArea = document.getElementById('save-area');
    const paramArea = document.getElementById('param-area');
    const fileContainer = document.getElementById('file-input-container');
    const status = document.getElementById('status');
    const canvasEl = document.getElementById('canvas');

    dpad.classList.remove('mode1-active', 'mode3-active', 'mode5-active');
    fileContainer.style.display = (mode === 4) ? "block" : "none";
    paramArea.style.display = (mode === 4) ? "none" : "block";
    
    let color = "#fff";
    if (mode === 3) { status.textContent="Mode 3: 動作確認"; color="#4CAF50"; dpad.style.display="block"; dpad.classList.add('mode3-active'); saveArea.style.display="none"; }
    else if (mode === 5) { status.textContent="Mode 5: 別波形動作"; color="#00BCD4"; dpad.style.display="block"; dpad.classList.add('mode5-active'); saveArea.style.display="none"; }
    else if (mode === 1) { status.textContent="Mode 1: 3秒記録"; color="#2196F3"; dpad.style.display="block"; dpad.classList.add('mode1-active'); saveArea.style.display="block"; }
    else if (mode === 2) { status.textContent="Mode 2: 自動追尾"; color="#9C27B0"; dpad.style.display="none"; saveArea.style.display="none"; }
    else if (mode === 4) { status.textContent="Mode 4: 動画解析"; color="#FF5722"; dpad.style.display="none"; saveArea.style.display="block"; }
    
    status.style.color = color;
    canvasEl.style.borderColor = color;
}

function findLargestColorCenter(hsvMat, lowColor1, highColor1, lowColor2=null, highColor2=null) {
    let mask = new cv.Mat();
    let low1 = new cv.Mat(hsvMat.rows, hsvMat.cols, hsvMat.type(), new cv.Scalar(lowColor1[0], lowColor1[1], lowColor1[2], 0));
    let high1 = new cv.Mat(hsvMat.rows, hsvMat.cols, hsvMat.type(), new cv.Scalar(highColor1[0], highColor1[1], highColor1[2], 255));
    cv.inRange(hsvMat, low1, high1, mask);

    if (lowColor2 && highColor2) {
        let mask2 = new cv.Mat();
        let low2 = new cv.Mat(hsvMat.rows, hsvMat.cols, hsvMat.type(), new cv.Scalar(lowColor2[0], lowColor2[1], lowColor2[2], 0));
        let high2 = new cv.Mat(hsvMat.rows, hsvMat.cols, hsvMat.type(), new cv.Scalar(highColor2[0], highColor2[1], highColor2[2], 255));
        cv.inRange(hsvMat, low2, high2, mask2);
        
        cv.bitwise_or(mask, mask2, mask);
        
        mask2.delete(); low2.delete(); high2.delete();
    }

    let contours = new cv.MatVector();
    let hierarchy = new cv.Mat();
    cv.findContours(mask, contours, hierarchy, cv.RETR_EXTERNAL, cv.CHAIN_APPROX_SIMPLE);

    let maxArea = 0;
    let bestPos = null;
    for (let i = 0; i < contours.size(); i++) {
        let cnt = contours.get(i);
        let area = cv.contourArea(cnt, false);
        if (area > 20 && area > maxArea) {
            maxArea = area;
            let M = cv.moments(cnt);
            bestPos = { x: M.m10 / M.m00, y: M.m01 / M.m00 };
        }
    }
    mask.delete(); low1.delete(); high1.delete(); contours.delete(); hierarchy.delete();
    return bestPos;
}

function processLoop() {
    try {
        if (!cv) { requestAnimationFrame(processLoop); return; }

        let vw = videoElement.videoWidth;
        let vh = videoElement.videoHeight;

        if (vw > 0) {
            document.getElementById('resolution-display').textContent = `Resolution: ${vw} x ${vh}`;
        }

        if (videoElement.paused || videoElement.ended) { requestAnimationFrame(processLoop); return; }

        if (canvas.width !== 640) {
            let aspect = vh / vw;
            if(isNaN(aspect)) aspect = 0.75;
            canvas.width = 640; canvas.height = 640 * aspect;
        }
        ctx.drawImage(videoElement, 0, 0, canvas.width, canvas.height);

        let src = cv.matFromImageData(ctx.getImageData(0, 0, canvas.width, canvas.height));
        let hsv = new cv.Mat();
        cv.cvtColor(src, hsv, cv.COLOR_RGBA2RGB);
        cv.cvtColor(hsv, hsv, cv.COLOR_RGB2HSV);

        let centerPos = findLargestColorCenter(hsv, COLOR_CENTER_LOW, COLOR_CENTER_HIGH);
        let frontPos = findLargestColorCenter(hsv, COLOR_FRONT_LOW1, COLOR_FRONT_HIGH1, COLOR_FRONT_LOW2, COLOR_FRONT_HIGH2);

        let hasAngle = false;

        if (centerPos) {
            normX = Math.round((centerPos.x / canvas.width) * 255);
            normY = Math.round((centerPos.y / canvas.height) * 255);

            if (vw > 0 && vh > 0) {
                pixelX = Math.round((centerPos.x / canvas.width) * vw);
                pixelY = Math.round((centerPos.y / canvas.height) * vh);
            } else {
                pixelX = normX; pixelY = normY;
            }

            cv.circle(src, new cv.Point(centerPos.x, centerPos.y), 6, [255, 255, 0, 255], -1);
            cv.circle(src, new cv.Point(centerPos.x, centerPos.y), 8, [0, 0, 0, 255], 2);

            if (frontPos) {
                cv.circle(src, new cv.Point(frontPos.x, frontPos.y), 4, [255, 0, 0, 255], -1); 

                let dx = frontPos.x - centerPos.x;
                let dy = frontPos.y - centerPos.y;
                let rad = Math.atan2(dy, dx);
                let deg = rad * (180 / Math.PI);
                if (deg < 0) deg += 360;
                latestAngle = Math.round(deg);
                hasAngle = true;

                cv.arrowedLine(src, new cv.Point(centerPos.x, centerPos.y), new cv.Point(frontPos.x, frontPos.y), [0, 255, 0, 255], 2);
            }

 if (currentMode === 2 && bleCharacteristic && !isVideoFileMode) {
                if (targetX === -1 || targetY === -1) {
                    // 目標なし
                } 
                else if (!isSending) {

                    let commandId = 0; // 0:停止

                    // 1. ロボットからターゲットへのベクトル (dx, dy)
                    let dx = targetX - pixelX;
                    let dy = targetY - pixelY;

                    // 2. 角度をラジアンに変換
                    let rad = latestAngle * (Math.PI / 180);

                    // 3. 座標変換
                    let relFront = dx * Math.cos(rad) + dy * Math.sin(rad);
                    let relRight = -dx * Math.sin(rad) + dy * Math.cos(rad);

                    // 4. 指令の決定
                    if (Math.abs(relFront) < TARGET_TOLERANCE && Math.abs(relRight) < TARGET_TOLERANCE) {
                        commandId = 0; 
                    }
                    else if (Math.abs(relFront) > Math.abs(relRight)) {
                        if (relFront > 0) commandId = 1; // 前方
                        else commandId = 2;              // 後方
                    }
                    else {
                        if (relRight > 0) commandId = 3; // 右へ
                        else commandId = 4;              // 左へ
                    }

                    // コマンド送信
                    const now = Date.now();

                    // ✅ ここで初めて判定する（!isSending は親の else if で確認済みなので外してOK）
                    if (commandId !== lastCommandId || now - lastSendTime > 500) {
                        
                        isSending = true; // ✅ 送信するときだけロックをかける
                        
                        bleCharacteristic.writeValue(new Uint8Array([HEADER_MANUAL, commandId, 0]))
                            .then(() => {
                                lastCommandId = commandId;
                                lastSendTime = now;
                                isSending = false; // ✅ 送信成功したらロック解除
                            })
                            .catch((error) => {
                                console.error("送信エラー:", error);
                                isSending = false; // ✅ エラー時もロック解除
                            });
                    }
                }
            }

            // ★画面描画の追加（目標地点に×印を描画）
            if (targetX !== -1) {
                // 緑色のクロスを描画
                cv.line(src, new cv.Point(targetX - 10, targetY - 10), new cv.Point(targetX + 10, targetY + 10), [0, 255, 0, 255], 2);
                cv.line(src, new cv.Point(targetX + 10, targetY - 10), new cv.Point(targetX - 10, targetY + 10), [0, 255, 0, 255], 2);
                cv.putText(src, "TARGET", new cv.Point(targetX + 15, targetY), cv.FONT_HERSHEY_SIMPLEX, 0.5, [0, 255, 0, 255], 1);
            }
        }

        let infoText = "";
        if (currentMode === 1 && isRecording) {
            let t = Date.now() - recordStartTime;
            recordedData.push({ t: t, in: currentDirStr, x: pixelX, y: pixelY, angle: latestAngle });
            infoText = "REC";
        } else if (currentMode === 4 && !videoElement.paused) {
            let t = Math.round(videoElement.currentTime * 1000);
            let last = recordedData[recordedData.length - 1];
            if (!last || last.t !== t) {
                recordedData.push({ t: t, in: "Video", x: pixelX, y: pixelY, angle: latestAngle });
            }
            infoText = `Time:${t}`;
            updateLogCount();
        }

        if (infoText) cv.putText(src, infoText, new cv.Point(20, 40), cv.FONT_HERSHEY_SIMPLEX, 0.8, [255, 255, 255, 255], 2);
        
        let coordsText = `Pos:(${pixelX}, ${pixelY})`;
        if (hasAngle) coordsText += ` Ang:${latestAngle}`;
        else if (centerPos) coordsText += ` Ang:??? (Front Lost)`;
        else coordsText = `Searching Yellow...`;
        
        cv.putText(src, coordsText, new cv.Point(20, 70), cv.FONT_HERSHEY_SIMPLEX, 0.6, [255, 255, 0, 255], 1.5);

        cv.imshow('canvas', src);
        src.delete(); hsv.delete();

    } catch (e) { console.log(e); }
    requestAnimationFrame(processLoop);
}

async function connectBluetooth() {
    const SERVICE_UUID = '0000aaaa-0000-1000-8000-00805f9b34fb';
    const CHARACTERISTIC_UUID = '0000bbbb-0000-1000-8000-00805f9b34fb';
    try {
        document.getElementById('status').textContent = "接続試行中...";
        document.getElementById('status').style.color = "#FF9800";

        bleDevice = await navigator.bluetooth.requestDevice({ acceptAllDevices: true, optionalServices: [SERVICE_UUID] });
        
        bleDevice.addEventListener('gattserverdisconnected', () => {
            document.getElementById('status').textContent = "BLE切断されました";
            document.getElementById('status').style.color = "red";
            bleCharacteristic = null;
        });

        const server = await bleDevice.gatt.connect();
        await new Promise(resolve => setTimeout(resolve, 500));

        const service = await server.getPrimaryService(SERVICE_UUID);
        bleCharacteristic = await service.getCharacteristic(CHARACTERISTIC_UUID);
        await bleCharacteristic.startNotifications();
        bleCharacteristic.addEventListener('characteristicvaluechanged', handleReceiveData);
        document.getElementById('status').textContent = "Bluetooth接続OK";
        document.getElementById('status').style.color = "#4CAF50";
    } catch (err) { 
        alert("接続失敗: " + err); 
        document.getElementById('status').textContent = "接続エラー";
        document.getElementById('status').style.color = "red";
    }
}

async function sendManualCommand(id, headerType = HEADER_MANUAL) {
    if (!bleCharacteristic || isVideoFileMode) return;
    try { await bleCharacteristic.writeValue(new Uint8Array([headerType, id, 0])); } catch(e) {}
}

async function sendParamUpdate() {
    const btn = document.getElementById('btn-update');
    if (!bleCharacteristic) { 
        btn.textContent = "未接続です"; btn.style.backgroundColor = "#9E9E9E"; 
        setTimeout(() => { btn.textContent = "パラメータ更新"; btn.style.backgroundColor = "#FF9800"; }, 1000);
        return; 
    }
    const r1 = parseInt(document.getElementById('p-res1').value)||5;
    const p1 = parseInt(document.getElementById('p-rep1').value)||4;
    const r2 = parseInt(document.getElementById('p-res2').value)||10;
    const p2 = parseInt(document.getElementById('p-rep2').value)||4;
    try {
        await bleCharacteristic.writeValue(new Uint8Array([HEADER_PARAM, r1, p1, r2, p2]));
        btn.textContent = "更新完了!"; btn.style.backgroundColor = "#4CAF50";
        setTimeout(() => { btn.textContent = "パラメータ更新"; btn.style.backgroundColor = "#FF9800"; }, 1000);
    } catch (e) {
        btn.textContent = "エラー"; btn.style.backgroundColor = "#f44336";
        setTimeout(() => { btn.textContent = "パラメータ更新"; btn.style.backgroundColor = "#FF9800"; }, 1000);
    }
}

function setupDpad() {
    const btns = document.querySelectorAll('.d-btn');
    btns.forEach(btn => {
        const id = parseInt(btn.dataset.dir), name = btn.textContent;
        const press = (e) => {
            e.preventDefault(); 
            if(currentMode===3) { 
                sendManualCommand(id, HEADER_MANUAL); 
                document.getElementById('status').textContent = `動作中: ${name}`; 
            }
            else if(currentMode===5) { 
                sendManualCommand(id, HEADER_MANUAL2); 
                document.getElementById('status').textContent = `別波形で動作中: ${name}`; 
            }
            else if(currentMode===1 && !isRecording && !isVideoFileMode) {
                isRecording=true; recordStartTime=Date.now(); currentDirStr=name; 
                sendManualCommand(id, HEADER_MANUAL);
                document.getElementById('status').textContent = `REC中: ${name}`;
                setTimeout(()=>{ 
                    sendManualCommand(DIR_STOP, HEADER_MANUAL); 
                    isRecording=false; 
                    document.getElementById('status').textContent="完了"; 
                    updateLogCount(); 
                }, 3000);
            }
        };
        const release = (e) => { 
            e.preventDefault(); 
            if(currentMode===3) { 
                sendManualCommand(DIR_STOP, HEADER_MANUAL); 
                document.getElementById('status').textContent="待機中"; 
            } 
            else if(currentMode===5) { 
                sendManualCommand(DIR_STOP, HEADER_MANUAL2); 
                document.getElementById('status').textContent="待機中"; 
            } 
        };
        ['mousedown','touchstart'].forEach(ev=>btn.addEventListener(ev, press, {passive:false}));
        ['mouseup','mouseleave','touchend'].forEach(ev=>btn.addEventListener(ev, release));
    });
}

function updateLogCount() { document.getElementById('log-count').textContent = `データ数: ${recordedData.length}行`; }
function saveCSV() {
    if (recordedData.length===0) { alert("データなし"); return; }
    let csv = "Time(ms),Input,PixelX,PixelY,Angle(deg)\n";
    recordedData.forEach(r => csv += `${r.t},${r.in},${r.x},${r.y},${r.angle}\n`);
    const a = document.createElement('a');
    a.href = URL.createObjectURL(new Blob([csv], {type: 'text/csv'}));
    a.download = `log_${Date.now()}.csv`; a.click();
}

async function requestVoltage() {
    if (!bleCharacteristic || isVideoFileMode) {
        alert("Bluetoothが接続されていないか、動画モードです。");
        return;
    }
    try {
        document.getElementById('voltage-display').textContent = "取得中...";
        await bleCharacteristic.writeValue(new Uint8Array([HEADER_VOLTAGE]));
    } catch (e) {
        console.log("電圧要求エラー:", e);
        document.getElementById('voltage-display').textContent = "Error";
    }
}

function handleReceiveData(event) {
    const value = event.target.value;
    let data = new Uint8Array(value.buffer);
    
    if (data.length > 0) {
        if (data[0] === HEADER_VOLTAGE && data.length >= 3) {
            let volts_int = data[1];
            let volts_dec = data[2];
            document.getElementById('voltage-display').textContent = `${volts_int}.${volts_dec} V`;
        }
    }
}
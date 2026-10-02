#pragma once

#include <Arduino.h>

// The web interface. Stored as a raw string literal so it stays readable HTML.
// The page renders times locally between polls (smooth display with
// hundredths of a second) and resyncs with the ESP32 every second.
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32 Stopwatch</title>
<style>
:root{--bg:#f3f4f6;--card:#fff;--text:#1f2937;--muted:#6b7280;--accent:#2563eb;--accent-h:#1d4ed8;--danger:#dc2626;--border:#e5e7eb}
@media (prefers-color-scheme:dark){:root{--bg:#111827;--card:#1f2937;--text:#f9fafb;--muted:#9ca3af;--border:#374151}}
*{box-sizing:border-box}
body{margin:0;padding:16px;font-family:system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;background:var(--bg);color:var(--text)}
header{max-width:420px;margin:0 auto;display:flex;justify-content:space-between;align-items:center}
h1{font-size:20px;margin:0}
.status{font-size:13px;color:var(--muted)}
.status.offline{color:var(--danger)}
.error{max-width:420px;margin:8px auto 0;color:var(--danger);text-align:center;font-size:14px}
section{max-width:420px;margin:12px auto;padding:20px;background:var(--card);border-radius:12px;box-shadow:0 2px 8px rgba(0,0,0,.08)}
h2{margin:0 0 8px;font-size:14px;color:var(--muted);font-weight:600;text-transform:uppercase;letter-spacing:.05em}
.display{font-size:44px;font-weight:300;font-variant-numeric:tabular-nums;text-align:center;margin:8px 0 16px}
.display.done{color:var(--danger);animation:blink 1s steps(2) infinite}
@keyframes blink{50%{opacity:.3}}
.row{display:flex;gap:8px}
.row>*{flex:1;min-width:0}
.mb{margin-bottom:8px}
button{padding:14px;font-size:16px;border:0;border-radius:8px;background:var(--accent);color:#fff;cursor:pointer}
button:hover{background:var(--accent-h)}
button.secondary{background:var(--border);color:var(--text)}
button:disabled{opacity:.4;cursor:default}
.presets button{padding:8px;font-size:14px}
input{width:100%;padding:12px;font-size:16px;border:1px solid var(--border);border-radius:8px;background:var(--card);color:var(--text)}
label{display:block;font-size:13px;color:var(--muted);margin-bottom:4px}
.hint{text-align:center;color:var(--muted);font-size:14px;margin:0 0 12px}
progress{width:100%;height:8px;margin-bottom:8px}
ol{margin:12px 0 0;padding:0;list-style:none;max-height:220px;overflow-y:auto;font-variant-numeric:tabular-nums}
li{display:flex;justify-content:space-between;padding:6px 0;border-top:1px solid var(--border);font-size:15px}
li span:first-child{color:var(--muted)}
</style>
</head>
<body>
<header>
  <h1>ESP32 Stopwatch</h1>
  <span id="status" class="status">Connecting…</span>
</header>
<p id="error" class="error" hidden></p>

<section>
  <h2>Chronometer</h2>
  <div id="chrono" class="display">00:00:00.00</div>
  <div class="row">
    <button id="chronoSide" class="secondary" disabled>Reset</button>
    <button id="chronoMain">Start</button>
  </div>
  <ol id="laps"></ol>
</section>

<section>
  <h2>Timer</h2>
  <div id="timer" class="display">00:00:00</div>
  <div class="row presets mb">
    <button class="secondary tset" data-min="1">1 min</button>
    <button class="secondary tset" data-min="5">5 min</button>
    <button class="secondary tset" data-min="10">10 min</button>
    <button class="secondary tset" data-min="15">15 min</button>
  </div>
  <div class="row presets mb">
    <input id="tmin" type="number" min="0" max="5999" placeholder="min" inputmode="numeric">
    <input id="tsec" type="number" min="0" max="59" placeholder="sec" inputmode="numeric">
    <button id="timerSet" class="secondary tset">Set</button>
  </div>
  <div class="row">
    <button id="timerReset" class="secondary">Reset</button>
    <button id="timerMain">Start</button>
  </div>
</section>

<section>
  <h2>Interval Counter</h2>
  <div id="count" class="display">0</div>
  <progress id="ivBar" max="1" value="0"></progress>
  <p id="ivNext" class="hint">Not started</p>
  <label for="ivSec">Interval (seconds)</label>
  <input id="ivSec" class="mb" type="number" min="1" step="1" value="60" inputmode="numeric">
  <div class="row">
    <button id="ivReset" class="secondary">Reset</button>
    <button id="ivPause" class="secondary" disabled>Pause</button>
    <button id="ivStart">Start</button>
  </div>
</section>

<script>
const $ = id => document.getElementById(id);
let state = null, receivedAt = 0, audioCtx = null;
let prevTimerFinished = null, prevIvCount = null, lapsKey = '', errorTimeout = 0;

// ---------- formatting ----------
const pad = n => String(n).padStart(2, '0');
function fmt(ms, withHundredths) {
  const total = Math.floor(ms / 1000);
  let out = pad(Math.floor(total / 3600)) + ':' + pad(Math.floor(total / 60) % 60) + ':' + pad(total % 60);
  if (withHundredths) out += '.' + pad(Math.floor(ms / 10) % 100);
  return out;
}

// ---------- status / errors ----------
function setStatus(text, bad) {
  const el = $('status');
  el.textContent = text;
  el.classList.toggle('offline', bad);
}
function showError(msg) {
  const el = $('error');
  el.textContent = msg;
  el.hidden = false;
  clearTimeout(errorTimeout);
  errorTimeout = setTimeout(() => el.hidden = true, 4000);
}

// ---------- sound / vibration ----------
function unlockAudio() {
  // Browsers only allow audio after a user gesture, so create it on first click.
  if (!audioCtx) {
    const Ctx = window.AudioContext || window.webkitAudioContext;
    if (Ctx) audioCtx = new Ctx();
  }
  if (audioCtx && audioCtx.state === 'suspended') audioCtx.resume();
}
function beep(at, duration, freq) {
  const osc = audioCtx.createOscillator(), gain = audioCtx.createGain();
  osc.frequency.value = freq;
  gain.gain.value = 0.2;
  osc.connect(gain).connect(audioCtx.destination);
  osc.start(at);
  osc.stop(at + duration);
}
function alarm() {
  if (navigator.vibrate) navigator.vibrate([400, 200, 400, 200, 400]);
  if (!audioCtx) return;
  const t = audioCtx.currentTime;
  for (let i = 0; i < 3; i++) beep(t + i * 0.6, 0.4, 880);
}
function tick() {
  if (navigator.vibrate) navigator.vibrate(100);
  if (audioCtx) beep(audioCtx.currentTime, 0.12, 660);
}

// ---------- state from the ESP32 ----------
function applyState(s) {
  state = s;
  receivedAt = performance.now();
  const c = s.chrono, t = s.timer, iv = s.interval;

  $('chronoMain').textContent = c.running ? 'Stop' : 'Start';
  $('chronoSide').textContent = c.running ? 'Lap' : 'Reset';
  $('chronoSide').disabled = !c.running && c.elapsed === 0;
  renderLaps(c.laps);

  $('timerMain').textContent = t.running ? 'Pause'
    : t.finished ? 'Restart'
    : t.remaining < t.duration ? 'Resume' : 'Start';
  $('timerMain').disabled = t.duration === 0;
  $('timer').classList.toggle('done', t.finished);
  document.querySelectorAll('.tset').forEach(b => b.disabled = t.running);
  if (prevTimerFinished === false && t.finished) alarm();
  prevTimerFinished = t.finished;

  $('ivPause').textContent = iv.running ? 'Pause' : 'Resume';
  $('ivPause').disabled = iv.interval === 0;
  $('ivStart').disabled = iv.running;
  const ivCount = iv.interval ? Math.floor(iv.elapsed / iv.interval) : 0;
  if (prevIvCount !== null && ivCount > prevIvCount && iv.running) tick();
  prevIvCount = ivCount;
}

function renderLaps(laps) {
  const key = laps.join(',');
  if (key === lapsKey) return;
  lapsKey = key;
  const list = $('laps');
  list.innerHTML = '';
  for (let i = laps.length - 1; i >= 0; i--) {  // newest first
    const lapMs = laps[i] - (i > 0 ? laps[i - 1] : 0);
    const li = document.createElement('li');
    li.innerHTML = `<span>Lap ${i + 1}</span><span>${fmt(lapMs, true)}</span><span>${fmt(laps[i], true)}</span>`;
    list.appendChild(li);
  }
}

// ---------- smooth local rendering between polls ----------
function frame() {
  if (state) {
    const c = state.chrono, t = state.timer, iv = state.interval;
    const dt = performance.now() - receivedAt;

    $('chrono').textContent = fmt(c.elapsed + (c.running ? dt : 0), true);

    const left = Math.max(0, t.remaining - (t.running ? dt : 0));
    $('timer').textContent = fmt(Math.ceil(left / 1000) * 1000, false);

    if (iv.interval > 0) {
      const e = iv.elapsed + (iv.running ? dt : 0);
      const into = e % iv.interval;
      $('count').textContent = Math.floor(e / iv.interval);
      $('ivBar').value = into / iv.interval;
      $('ivNext').textContent = iv.running
        ? 'Next in ' + Math.ceil((iv.interval - into) / 1000) + ' s'
        : 'Paused';
    } else {
      $('count').textContent = '0';
      $('ivBar').value = 0;
      $('ivNext').textContent = 'Not started';
    }
  }
  requestAnimationFrame(frame);
}

// ---------- networking ----------
async function poll() {
  try {
    const res = await fetch('/api/state', { cache: 'no-store' });
    if (!res.ok) throw new Error(res.status);
    applyState(await res.json());
    setStatus('Connected', false);
  } catch (e) {
    setStatus('Offline – retrying…', true);
  } finally {
    // Keep polling no matter what; slow down while the tab is hidden.
    setTimeout(poll, document.hidden ? 5000 : 1000);
  }
}

async function post(path) {
  unlockAudio();
  try {
    const res = await fetch(path, { method: 'POST' });
    if (!res.ok) { showError(await res.text()); return; }
    applyState(await res.json());  // immediate feedback, no waiting for the next poll
  } catch (e) {
    showError('Request failed – check the Wi-Fi connection');
  }
}

// ---------- controls ----------
$('chronoMain').onclick = () => post(state?.chrono.running ? '/api/chrono/stop' : '/api/chrono/start');
$('chronoSide').onclick = () => post(state?.chrono.running ? '/api/chrono/lap' : '/api/chrono/reset');

$('timerMain').onclick = () => post(state?.timer.running ? '/api/timer/stop' : '/api/timer/start');
$('timerReset').onclick = () => post('/api/timer/reset');
document.querySelectorAll('[data-min]').forEach(b =>
  b.onclick = () => post('/api/timer/set?seconds=' + b.dataset.min * 60));
$('timerSet').onclick = () => {
  const seconds = Math.floor((+$('tmin').value || 0) * 60 + (+$('tsec').value || 0));
  if (seconds < 1) return showError('Enter a duration');
  post('/api/timer/set?seconds=' + seconds);
};

$('ivStart').onclick = () => {
  const seconds = Math.floor(+$('ivSec').value);
  if (!(seconds >= 1)) return showError('Enter an interval of at least 1 second');
  post('/api/interval/start?seconds=' + seconds);
};
$('ivPause').onclick = () => post(state?.interval.running ? '/api/interval/pause' : '/api/interval/resume');
$('ivReset').onclick = () => post('/api/interval/reset');

// Keyboard: Space = start/stop chronometer, L = lap
document.addEventListener('keydown', e => {
  if (e.target.tagName === 'INPUT') return;
  if (e.code === 'Space') { e.preventDefault(); $('chronoMain').click(); }
  else if ((e.key === 'l' || e.key === 'L') && state?.chrono.running) $('chronoSide').click();
});
document.addEventListener('pointerdown', unlockAudio);

poll();
requestAnimationFrame(frame);
</script>
</body>
</html>
)rawliteral";
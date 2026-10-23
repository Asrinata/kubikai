'use strict';
// API tesisi: anahtarlar, 14 günlük yük grafiği, canlı test istasyonu ve kod örnekleri
const $ = selector => document.querySelector(selector);
const el = (tag, cls, text) => { const n = document.createElement(tag); if (cls) n.className = cls; if (text != null) n.textContent = text; return n; };
const fmt = value => Number(value || 0).toLocaleString('tr');
const base = kubikTransport.url('') || location.origin;
let data = null, metric = 'requests';

function toast(message) { $('#toast').textContent = message; $('#toast').classList.add('show'); setTimeout(() => $('#toast').classList.remove('show'), 4000); }
async function api(path, options = {}) {
  const headers = {'X-Kubik-Request': '1'};
  if (options.body) { headers['Content-Type'] = 'application/json'; options.body = JSON.stringify(options.body); }
  const response = await kubikTransport.fetch('/api' + path, {...options, headers}), value = await response.json();
  if (!response.ok) throw Object.assign(new Error(typeof value.detail === 'string' ? value.detail : 'İstek başarısız.'), {status: response.status});
  return value;
}
async function copy(text) { try { await navigator.clipboard.writeText(text); toast('Panoya kopyalandı.'); } catch { toast('Kopyalanamadı, elle seç.'); } }

async function load() {
  try { data = await api('/developer/keys'); }
  catch (error) {
    $('#locked').hidden = false; $('#lockedText').textContent = error.status === 401 ? 'Önce Kübik’e giriş yap.' : error.message;
    for (const n of document.querySelectorAll('.readouts, .bays, .panel:not(.locked)')) n.hidden = true;
    return;
  }
  const live = data.keys.filter(k => !k.revoked);
  $('#rKeys').textContent = `${live.length}/5`;
  $('#rReq').textContent = fmt(data.daily.reduce((s, d) => s + d.requests, 0));
  $('#rTok').textContent = fmt(data.daily.reduce((s, d) => s + d.tokens, 0));
  $('#rLim').textContent = data.hourly_limit;
  $('#keys').replaceChildren(...(data.keys.length ? data.keys.map(keyRow) : [el('p', 'dim', 'Dolap boş. İlk anahtarını üret.')]));
  drawChart();
}

function keyRow(key) {
  const row = el('div', 'key' + (key.revoked ? ' off' : '')), info = el('div', 'key-info');
  info.append(el('b', '', key.name), el('code', '', key.prefix + '…'),
    el('small', '', `${fmt(key.requests)} istek · ${fmt(key.tokens)} token` + (key.last_used ? ` · son ${new Date(key.last_used * 1000).toLocaleString('tr')}` : ' · hiç kullanılmadı')));
  row.append(el('span', 'led ' + (key.revoked ? 'red' : 'green')), info);
  if (key.revoked) row.append(el('span', 'tag', 'İPTAL'));
  else {
    const b = el('button', 'btn danger', 'İPTAL ET'); b.type = 'button';
    b.onclick = async () => { if (!confirm(`“${key.name}” iptal edilsin mi? Kullanan uygulamalar çalışmaz.`)) return;
      try { await api('/developer/keys/' + key.id, {method: 'DELETE'}); load(); } catch (error) { toast(error.message); } };
    row.append(b);
  }
  return row;
}

function drawChart() {
  const peak = Math.max(1, ...data.daily.map(d => d[metric]));
  $('#chart').replaceChildren(...data.daily.map(d => {
    const col = el('div', 'col'), fill = el('span', 'fill');
    fill.style.height = (100 * d[metric] / peak) + '%';
    col.title = `${d.day}: ${fmt(d.requests)} istek, ${fmt(d.tokens)} token`;
    col.append(el('i', 'val', d[metric] ? fmt(d[metric]) : ''), el('span', 'tube'), el('small', '', d.day.slice(8)));
    col.querySelector('.tube').append(fill);
    return col;
  }));
  $('#chartNote').textContent = `Son 14 gün · en yüksek ${fmt(peak)} ${metric === 'requests' ? 'istek' : 'token'}`;
}
for (const b of document.querySelectorAll('[data-metric]')) b.onclick = () => {
  metric = b.dataset.metric; document.querySelectorAll('[data-metric]').forEach(x => x.classList.toggle('on', x === b)); if (data) drawChart(); };

$('#keyForm').onsubmit = async event => {
  event.preventDefault();
  try {
    const {key} = await api('/developer/keys', {method: 'POST', body: {name: $('#keyName').value}});
    $('#keyName').value = ''; $('#freshKey').textContent = key; $('#fresh').hidden = false; $('#benchKey').value = key;
    copy(key); load();
  } catch (error) { toast(error.message); }
};
$('#copyFresh').onclick = () => copy($('#freshKey').textContent);

// ---------------------------------------------------------------- test istasyonu
const log = line => { const c = $('#console'); c.textContent += `\n[${new Date().toLocaleTimeString('tr')}] ${line}`; c.scrollTop = c.scrollHeight; };
async function v1(path, key, body) {
  const response = await fetch(base + '/api/v1' + path, {method: body ? 'POST' : 'GET',
    headers: {Authorization: 'Bearer ' + key, ...(body ? {'Content-Type': 'application/json'} : {})}, body: body && JSON.stringify(body)});
  const value = await response.json().catch(() => ({}));
  if (!response.ok) throw new Error(`${response.status} · ${typeof value.detail === 'string' ? value.detail : 'hata'}`);
  return value;
}
$('#bench').onsubmit = async event => {
  event.preventDefault();
  const key = $('#benchKey').value.trim(), message = $('#benchMsg').value.trim(), run = $('#benchRun');
  if (!key.startsWith('kbk_')) return toast('Geçerli bir kbk_ anahtarı gir.');
  if (!message) return;
  run.disabled = true; $('#console').textContent = '> POST /api/v1/chat';
  const started = Date.now();
  try {
    const job = await v1('/chat', key, {message});
    log(`iş açıldı · id ${job.id}`);
    let last = '';
    for (;;) {
      await new Promise(r => setTimeout(r, 2000));
      const state = await v1('/chat/' + job.id, key);
      if (state.status !== last) { log(`durum: ${state.status}`); last = state.status; }
      if (state.status === 'done') {
        const u = state.usage || {};
        log(`${state.model || 'Kübik'} · ${((Date.now() - started) / 1000).toFixed(1)} sn · ${fmt(u.prompt_tokens)} giriş + ${fmt(u.completion_tokens)} çıkış token`);
        $('#console').textContent += '\n\n' + state.text + '\n\n> bitti_';
        break;
      }
      if (state.status === 'error') { log('HATA: ' + (state.error || state.text || 'bilinmiyor')); break; }
    }
    load();
  } catch (error) { log('HATA ' + error.message); }
  finally { run.disabled = false; }
};

// ---------------------------------------------------------------- kod örnekleri
const SNIPPETS = {
  python: `from kubik import Kubik          # kubik.py: üstteki KUBIK.PY bağlantısı

kubik = Kubik("kbk_ANAHTARIN")
print(kubik.sor("Merhaba! Kendini bir cümleyle tanıt."))

sohbet = kubik.sohbet()           # aynı konuşmada devam eder
print(sohbet.sor("Python'da liste nasıl sıralanır?"))`,
  curl: `curl -X POST ${base}/api/v1/chat \\
  -H "Authorization: Bearer kbk_ANAHTARIN" -H "Content-Type: application/json" \\
  -d '{"message": "Merhaba"}'

curl ${base}/api/v1/chat/IS_ID \\
  -H "Authorization: Bearer kbk_ANAHTARIN"`,
  js: `const base = "${base}/api/v1";
const headers = {Authorization: "Bearer " + process.env.KUBIK_KEY, "Content-Type": "application/json"};
const job = await (await fetch(base + "/chat", {method: "POST", headers, body: JSON.stringify({message: "Merhaba"})})).json();
let result;
do { await new Promise(r => setTimeout(r, 2000)); result = await (await fetch(base + "/chat/" + job.id, {headers})).json(); }
while (result.status === "queued" || result.status === "running");
console.log(result.text);`};
const showSnippet = lang => { $('#snippet').textContent = SNIPPETS[lang]; document.querySelectorAll('[data-lang]').forEach(x => x.classList.toggle('on', x.dataset.lang === lang)); };
for (const b of document.querySelectorAll('[data-lang]')) b.onclick = () => showSnippet(b.dataset.lang);
$('#copyCode').onclick = () => copy($('#snippet').textContent);
showSnippet('python');

// ---------------------------------------------------------------- sistem LED'i
(async () => {
  try { const r = await kubikTransport.fetch('/api/status'); if (!r.ok) throw 0;
    $('#sysLed').className = 'led green'; $('#sysText').textContent = 'SİSTEM ÇEVRİMİÇİ'; }
  catch { $('#sysLed').className = 'led red'; $('#sysText').textContent = 'SUNUCUYA ULAŞILAMIYOR'; }
})();
load();

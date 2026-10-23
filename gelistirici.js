'use strict';
// API platformu: anahtarlar, 14 günlük kullanım grafiği, deneme alanı ve kod örnekleri
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
    for (const n of document.querySelectorAll('.stats, .dev-card:not(.locked)')) n.hidden = true;
    return;
  }
  $('#rKeys').textContent = `${data.keys.filter(k => !k.revoked).length}/5`;
  $('#rReq').textContent = fmt(data.daily.reduce((s, d) => s + d.requests, 0));
  $('#rTok').textContent = fmt(data.daily.reduce((s, d) => s + d.tokens, 0));
  $('#rLim').textContent = data.hourly_limit;
  $('#keys').replaceChildren(...(data.keys.length ? data.keys.map(keyRow) : [emptyRow()]));
  drawChart();
}

function emptyRow() { const tr = el('tr'), td = el('td', 'muted', 'Henüz anahtar yok. İlk anahtarını oluştur.'); td.colSpan = 6; tr.append(td); return tr; }
function keyRow(key) {
  const tr = el('tr', key.revoked ? 'off' : ''), cell = (...parts) => { const td = el('td'); td.append(...parts); tr.append(td); };
  cell(el('b', '', key.name), ...(key.revoked ? [' ', el('span', 'pill', 'İptal edildi')] : []));
  cell(el('code', '', key.prefix + '…'));
  cell(fmt(key.requests));
  cell(fmt(key.tokens));
  cell(key.last_used ? new Date(key.last_used * 1000).toLocaleString('tr', {dateStyle: 'short', timeStyle: 'short'}) : '—');
  if (key.revoked) cell('');
  else {
    const b = el('button', 'text-button', 'İptal et'); b.type = 'button';
    b.onclick = async () => { if (!confirm(`“${key.name}” iptal edilsin mi? Kullanan uygulamalar çalışmaz.`)) return;
      try { await api('/developer/keys/' + key.id, {method: 'DELETE'}); load(); } catch (error) { toast(error.message); } };
    cell(b);
  }
  return tr;
}

function drawChart() {
  const peak = Math.max(1, ...data.daily.map(d => d[metric]));
  $('#chart').replaceChildren(...data.daily.map(d => {
    const col = el('div', 'col'), bar = el('span', 'bar');
    bar.style.height = (100 * d[metric] / peak) + '%';
    col.title = `${d.day}: ${fmt(d.requests)} istek, ${fmt(d.tokens)} token`;
    const track = el('span', 'track'); track.append(bar);
    col.append(track, el('small', '', d.day.slice(8)));
    return col;
  }));
  const total = data.daily.reduce((s, d) => s + d[metric], 0);
  $('#chartNote').textContent = `Son 14 gün · toplam ${fmt(total)} ${metric === 'requests' ? 'istek' : 'token'}`;
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

// ---------------------------------------------------------------- deneme alanı
const log = line => { const c = $('#console'); c.textContent += `\n${new Date().toLocaleTimeString('tr')}  ${line}`; c.scrollTop = c.scrollHeight; };
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
  run.disabled = true; $('#console').textContent = 'POST /api/v1/chat';
  const started = Date.now();
  try {
    const job = await v1('/chat', key, {message});
    log(`İş açıldı · ${job.id}`);
    let last = '';
    for (;;) {
      await new Promise(r => setTimeout(r, 2000));
      const state = await v1('/chat/' + job.id, key);
      if (state.status !== last) { log(`Durum: ${state.status}`); last = state.status; }
      if (state.status === 'done') {
        const u = state.usage || {};
        log(`${state.model || 'Kübik'} · ${((Date.now() - started) / 1000).toFixed(1)} sn · ${fmt(u.prompt_tokens)} giriş + ${fmt(u.completion_tokens)} çıkış token`);
        $('#console').textContent += '\n\n' + state.text;
        break;
      }
      if (state.status === 'error') { log('Hata: ' + (state.error || state.text || 'bilinmiyor')); break; }
    }
    load();
  } catch (error) { log('Hata: ' + error.message); }
  finally { run.disabled = false; }
};

// ---------------------------------------------------------------- kod örnekleri
const SNIPPETS = {
  python: `from kubik import Kubik          # kubik.py: soldaki menüden indir

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

// sol menü: tıklanan bölümü vurgula
for (const a of document.querySelectorAll('.dev-side nav a'))
  a.addEventListener('click', () => document.querySelectorAll('.dev-side nav a').forEach(x => x.classList.toggle('active', x === a)));

(async () => {
  try { const r = await kubikTransport.fetch('/api/status'); if (!r.ok) throw 0;
    $('#sysLed').className = 'dot ok'; $('#sysText').textContent = 'Tüm sistemler çalışıyor'; }
  catch { $('#sysLed').className = 'dot bad'; $('#sysText').textContent = 'Sunucuya ulaşılamıyor'; }
})();
load();

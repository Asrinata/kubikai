'use strict';
// Kübik ekleri: maskot, profil ve başarımlar, e-posta, davet, API platformu, rehber turu, destek asistanı,
// uyarılar, model kapanış sayacı, Cube Connect. app.js'teki $, node, api, t, toast, account, siteStatus ortak.

// ---------------------------------------------------------------- maskot (kağıttaki çizimden: sekizgen kafa, merdiven boyun, ekranlı gövde)
let SKINS = {klasik: {name: 'Klasik', rarity: 'common', colors: ['#e9e6dc', '#f4f2ea', '#2b2b2b', '#1d1d1d'], extra: ''}};
const RARITY = {common: 'Sıradan', rare: 'Nadir', epic: 'Destansı', legendary: 'Efsanevi'};
api('/skins').then(data => { SKINS = data.skins; document.querySelectorAll('[data-maskot]').forEach(drawMaskot); }).catch(() => {});
function maskot(skin = 'klasik', className = '') {
  const holder = node('span', 'maskot ' + className); holder.dataset.maskot = skin; drawMaskot(holder); return holder;
}
function drawMaskot(holder) {
  const s = SKINS[holder.dataset.maskot] || SKINS.klasik, [body, head, accent, eye] = s.colors, ink = '#1d1d1d';
  const id = 'm' + Math.random().toString(36).slice(2, 9), line = `stroke="${ink}" stroke-width="2.5" stroke-linejoin="round" stroke-linecap="round"`;
  const extra = {
    anten: `<line x1="50" y1="10" x2="50" y2="0" ${line}/><circle cx="50" cy="-1" r="3.5" fill="${accent}" ${line}/>`,
    gozluk: `<rect x="18" y="20" width="22" height="13" rx="3" fill="${accent}" opacity=".85"/><rect x="60" y="20" width="22" height="13" rx="3" fill="${accent}" opacity=".85"/><line x1="40" y1="25" x2="60" y2="25" stroke="${accent}" stroke-width="2.5"/>`,
    bant: `<line x1="7" y1="15" x2="93" y2="33" ${line}/><ellipse cx="29" cy="27" rx="9.5" ry="8.5" fill="${ink}"/>`,
    yildiz: [[30, 88], [52, 62], [70, 90], [64, 80]].map(([x, y]) => `<circle cx="${x}" cy="${y}" r="1.4" fill="#fff"/>`).join(''),
    sef: `<path d="M28 11 Q20 -2 33 -4 Q39 -13 50 -7 Q61 -13 67 -4 Q80 -2 72 11 Z" fill="#fff" ${line}/>`,
    tac: `<path d="M28 11 L32 -5 L41 3 L50 -10 L59 3 L68 -5 L72 11 Z" fill="${accent}" ${line}/><circle cx="50" cy="-2" r="2.5" fill="#e84a5f"/>`,
  }[s.extra] || '';
  holder.innerHTML = `<svg viewBox="-2 -16 104 136" aria-hidden="true">
    <defs><pattern id="${id}" width="7" height="7" patternUnits="userSpaceOnUse" patternTransform="rotate(45)"><rect width="3.5" height="7" fill="${accent}"/></pattern></defs>
    <g class="m-arms"><path d="M21 64 Q8 60 6 46" fill="none" ${line}/><path d="M79 64 Q92 60 94 46" fill="none" ${line}/></g>
    <line x1="35" y1="96" x2="35" y2="116" ${line}/><line x1="65" y1="96" x2="65" y2="116" ${line}/>
    <rect x="41" y="43" width="18" height="14" fill="${body}" ${line}/><line x1="41" y1="48" x2="59" y2="48" ${line}/><line x1="41" y1="52.5" x2="59" y2="52.5" ${line}/>
    <rect x="20" y="56" width="60" height="40" rx="4" fill="${body}" ${line}/>
    <circle cx="28" cy="65" r="2.8" fill="${accent}"/><line x1="34" y1="65" x2="72" y2="65" stroke="${accent}" stroke-width="2"/>
    <circle cx="28" cy="74" r="2.8" fill="${accent}"/><line x1="34" y1="74" x2="64" y2="74" stroke="${accent}" stroke-width="3.5"/>
    <rect x="21.5" y="82" width="57" height="12.5" fill="url(#${id})"/><line x1="20" y1="82" x2="80" y2="82" ${line}/>
    <g class="m-head"><polygon points="16,10 84,10 94,19 94,35 84,44 16,44 6,35 6,19" fill="${head}" ${line}/>
    <g class="m-eyes"><circle cx="29" cy="27" r="6" fill="${eye}"/><circle cx="71" cy="27" r="6" fill="${eye}"/></g>
    <path d="M41 31 Q50 38 59 31" fill="none" stroke="${eye}" stroke-width="2.5" stroke-linecap="round"/>${extra}</g></svg>`;
}
function wearSkin(skin) {
  document.querySelectorAll('.maskot.mine').forEach(m => { m.dataset.maskot = skin; drawMaskot(m); });
}

// ---------------------------------------------------------------- davet linki ve kayıt
const invite = new URLSearchParams(location.search).get('davet');
if (invite && /^[a-z0-9_.-]{3,32}$/.test(invite)) {
  try { sessionStorage.setItem('kubik-davet', invite); } catch {}
  if (!registerMode) $('#authToggle').click();
  $('#authSubtitle').textContent = `${invite} seni davet etti: kayıt olunca ikiniz de Max+ ve 500 Küp kazanırsınız.`;
}
const inviteCode = () => { try { return sessionStorage.getItem('kubik-davet') || ''; } catch { return ''; } };
const inviteLink = name => (window.KUBIK_PAGES ? location.origin + location.pathname : PUBLIC_SITE) + '?davet=' + encodeURIComponent(name);
const asset = file => window.KUBIK_PAGES ? file : '/static/' + file;      // Pages kökte, Pi'de /static/ altında

// ---------------------------------------------------------------- profil
async function openProfile() {
  const dialog = $('#profileDialog'); if (!dialog.open) dialog.showModal();
  const box = $('#profileBody'); box.replaceChildren(node('p', 'muted', '…'));
  try {
    const p = await api('/profile'), head = node('div', 'profile-head'), info = node('div');
    const big = maskot(p.skin, 'mine big');
    info.append(node('h3', '', p.username), node('p', 'muted', `${p.plan} · ${p.kup.toLocaleString(lang)} Küp · ${new Date(p.created * 1000).toLocaleDateString(lang)} tarihinden beri`));
    head.append(big, info);
    // kostümler
    const skins = node('div', 'skin-grid');
    for (const [id, skin] of Object.entries(SKINS)) {
      const owned = p.skins.includes(id), b = node('button', 'skin' + (p.skin === id ? ' worn' : '') + (owned ? '' : ' locked') + ' ' + skin.rarity);
      b.type = 'button'; b.title = `${skin.name} · ${RARITY[skin.rarity]}` + (owned ? '' : ` · Mağazada ${skin.price} Küp`);
      b.append(maskot(id), node('small', '', owned ? skin.name : `🔒 ${skin.price}`));
      b.onclick = async () => { if (!owned) { dialog.close(); return openShop(); }
        try { await api('/profile/skin', {method: 'PUT', body: {skin: id}}); wearSkin(id); account.skin = id; openProfile(); } catch (error) { toast(error.message); } };
      skins.append(b);
    }
    const shopButton = node('button', 'secondary', '🛍 Mağaza'); shopButton.type = 'button'; shopButton.onclick = () => { dialog.close(); openShop(); };
    // davet
    const link = node('input'); link.readOnly = true; link.value = inviteLink(p.invite.code); link.setAttribute('aria-label', 'Davet linki');
    const copy = node('button', 'secondary', 'Kopyala'); copy.type = 'button';
    copy.onclick = async () => { try { await navigator.clipboard.writeText(link.value); toast('Davet linki kopyalandı.'); } catch { link.select(); } };
    const inviteRow = node('div', 'work-link'); inviteRow.append(link, copy);
    // e-posta
    const mail = node('form', 'inline-form'), mailInput = node('input'), codeInput = node('input');
    mailInput.type = 'email'; mailInput.placeholder = 'eposta@ornek.com'; mailInput.value = p.email; mailInput.maxLength = 254; mailInput.required = true;
    codeInput.placeholder = '6 haneli kod'; codeInput.inputMode = 'numeric'; codeInput.maxLength = 6; codeInput.hidden = true;
    const send = node('button', 'secondary', p.email_verified ? 'Değiştir' : 'Kod gönder');
    mail.append(mailInput, codeInput, send);
    mail.onsubmit = async event => { event.preventDefault(); send.disabled = true;
      try {
        if (codeInput.hidden) { await api('/account/email', {method: 'POST', body: {email: mailInput.value}}); codeInput.hidden = false; send.textContent = 'Doğrula'; codeInput.focus(); toast('Kod gönderildi, e-postana bak.'); }
        else { await api('/account/email/verify', {method: 'POST', body: {code: codeInput.value.trim()}}); toast('E-posta doğrulandı!'); openProfile(); }
      } catch (error) { toast(error.message); } finally { send.disabled = false; } };
    // başarımlar
    const done = p.achievements.filter(a => a.done).length, badges = node('div', 'badge-grid');
    for (const a of p.achievements) { const b = node('div', 'achievement' + (a.done ? ' done' : '')); b.append(node('strong', '', (a.done ? '★ ' : '☆ ') + a.name), node('small', '', a.desc)); badges.append(b); }
    // bağlantılar
    const links = node('div', 'button-row');
    const apiButton = node('button', 'secondary', '⌘ API platformu'); apiButton.type = 'button';
    apiButton.onclick = () => { if (!p.api) return toast('API platformu Max+ ve üstü planlarda var.'); dialog.close(); openApi(); };
    const tour = node('button', 'secondary', 'Rehberi tekrar göster'); tour.type = 'button'; tour.onclick = () => { dialog.close(); startTour(true); };
    links.append(apiButton, tour);
    if (isApp) { const scan = node('button', 'primary', '✦ Cube Connect · bilgisayarda giriş'); scan.type = 'button'; scan.onclick = () => { dialog.close(); openScanner(); }; links.prepend(scan); }
    for (const [href, text] of [['docs.html', 'Kübik Docs'], ['modeller.html', 'Model ömürleri']]) { const a = node('a', 'secondary', text); a.href = asset(href); a.target = '_blank'; a.rel = 'noopener'; links.append(a); }
    box.replaceChildren(head, node('h3', '', 'Kostümler'), skins, shopButton,
      node('h3', '', `Arkadaş davet et · ${p.invite.used}/${p.invite.limit}`), node('p', 'fine', 'Arkadaşın bu linkle kayıt olunca ikiniz de Max+ (30 gün) ve 500 Küp alırsınız.'), inviteRow,
      node('h3', '', 'E-posta ' + (p.email_verified ? '· doğrulandı ✓' : '· doğrulanmadı')), mail,
      node('h3', '', `Başarımlar · ${done}/${p.achievements.length}`), badges, links);
  } catch (error) { box.replaceChildren(node('p', 'muted', error.message)); }
}
$('#profileButton').onclick = () => account?.role === 'guest' ? toast('Profil için hesap oluşturman gerekiyor.') : openProfile();
$('#closeProfile').onclick = () => $('#profileDialog').close();

// ---------------------------------------------------------------- API platformu
async function openApi() {
  const dialog = $('#apiDialog'); if (!dialog.open) dialog.showModal();
  const box = $('#apiBody');
  try {
    const data = await api('/developer/keys'), list = node('div', 'memory-list');
    for (const key of data.keys) {
      const row = node('div', 'request-row' + (key.revoked ? ' revoked' : ''));
      row.append(node('span', '', `${key.name} · ${key.prefix}… · ${key.requests} istek · ${key.tokens.toLocaleString(lang)} token` + (key.last_used ? ` · son ${new Date(key.last_used * 1000).toLocaleString(lang)}` : '') + (key.revoked ? ' · iptal' : '')));
      if (!key.revoked) { const b = node('button', 'secondary', 'İptal et'); b.onclick = async () => { if (!confirm('Bu anahtar iptal edilsin mi? Kullanan uygulamalar çalışmaz.')) return; await api('/developer/keys/' + key.id, {method: 'DELETE'}); openApi(); }; row.append(b); }
      list.append(row);
    }
    if (!data.keys.length) list.append(node('p', 'muted', 'Henüz anahtar yok.'));
    // son 14 gün: istek sayısı çubukları
    const peak = Math.max(1, ...data.daily.map(d => d.requests)), chart = node('div', 'api-chart');
    for (const d of data.daily) { const bar = node('div', 'api-bar'), fill = node('span'); fill.style.height = Math.max(2, 100 * d.requests / peak) + '%';
      bar.title = `${d.day}: ${d.requests} istek, ${d.tokens.toLocaleString(lang)} token`; bar.append(fill, node('small', '', d.day.slice(8))); chart.append(bar); }
    const total = data.daily.reduce((s, d) => ({r: s.r + d.requests, t: s.t + d.tokens}), {r: 0, t: 0});
    const form = node('form', 'inline-form'), name = node('input'); name.placeholder = 'Anahtar adı (ör. Discord botum)'; name.maxLength = 40;
    form.append(name, node('button', 'primary', 'Yeni anahtar'));
    form.onsubmit = async event => { event.preventDefault();
      try { const {key} = await api('/developer/keys', {method: 'POST', body: {name: name.value}}); await openApi();
        const shown = node('pre', 'secret-display', key + '\nBu anahtarı şimdi kopyala; bir daha gösterilmez.'); $('#apiBody').prepend(shown);
        try { await navigator.clipboard.writeText(key); toast('Anahtar panoya kopyalandı.'); } catch {} }
      catch (error) { toast(error.message); } };
    const docs = node('p', 'fine'); const docLink = node('a', '', 'API belgeleri'), lib = node('a', '', 'kubik.py kütüphanesi');
    docLink.href = asset('docs.html#api'); docLink.target = '_blank'; lib.href = asset('kubik.py'); lib.download = 'kubik.py';
    docs.append(docLink, ' · ', lib, ` · saatte en fazla ${data.hourly_limit} istek`);
    box.replaceChildren(form, node('h3', '', 'Anahtarların'), list, node('h3', '', `Son 14 gün · ${total.r} istek · ${total.t.toLocaleString(lang)} token`), chart, docs);
  } catch (error) { box.replaceChildren(node('p', 'muted', error.message)); }
}
$('#closeApi').onclick = () => $('#apiDialog').close();

// ---------------------------------------------------------------- rehber turu (ilk girişte bir kez)
const TOUR = [
  ['.mode-switch', 'Chat, Code ve Work burada. Code’da Kübik kodu kendi sanal makinesinde yazıp çalıştırır.'],
  ['#modelPicker', 'Kübik’in modeli. Uyku modundaysa ilk mesajın onu uyandırır.'],
  ['#usageMeter', 'Bu halka 5 saatlik ve haftalık kullanımını gösterir. Tıkla, ayrıntıları gör.'],
  ['#prompt', 'Sorunu buraya yaz. Dosya ve fotoğraf da ekleyebilirsin.'],
  ['#openSettings', 'Düşünme derinliği, verimlilik, multitask ve dil ayarları burada.'],
  ['#planButton', 'Planlar Küp ile alınır. Mağazadan maskota kostüm de alabilirsin.'],
  ['#profileButton', 'Profilin: başarımlar, maskot kostümleri, arkadaş daveti ve API.'],
  ['#supportButton', 'Ben Kübit, Kübik’in maskotu! Takıldığın yerde bana sor.'],
];
function startTour(force) {
  if (!force && store.get('kubik-tour', '') === 'done') return;
  const steps = TOUR.filter(([sel]) => { const e = $(sel); return e && e.offsetParent !== null; });
  if (!steps.length) return;
  const veil = $('#tour'), spot = $('#tourSpot'), bubble = $('#tourBubble'); let index = 0;
  const finish = () => { veil.hidden = true; store.set('kubik-tour', 'done'); removeEventListener('resize', place); };
  function place() {
    const [sel, text] = steps[index], r = $(sel).getBoundingClientRect(), pad = 6;
    Object.assign(spot.style, {left: r.left - pad + 'px', top: r.top - pad + 'px', width: r.width + pad * 2 + 'px', height: r.height + pad * 2 + 'px'});
    $('#tourText').textContent = text; $('#tourCount').textContent = `${index + 1}/${steps.length}`;
    $('#tourNext').textContent = index === steps.length - 1 ? 'Bitir' : 'İleri';
    const below = r.bottom + 190 < innerHeight, left = Math.min(Math.max(12, r.left), innerWidth - bubble.offsetWidth - 12);
    Object.assign(bubble.style, {left: left + 'px', top: (below ? r.bottom + 14 : Math.max(12, r.top - bubble.offsetHeight - 14)) + 'px'});
  }
  $('#tourNext').onclick = () => { if (++index >= steps.length) finish(); else place(); };
  $('#tourSkip').onclick = finish;
  veil.hidden = false; addEventListener('resize', place); place();
}

// ---------------------------------------------------------------- destek asistanı (önce hazır cevaplar, sonra yapay zekâ)
const FAQ = [
  [/küp|kup|kazan/i, 'Küp kazanmak için: hesap açınca 2000, her gün ilk mesaj 100, her mesaj 10 (günde 200’e kadar), yanıtlara 👍/👎 vermek 25, onaylanan “Düzelt ve öğret” 50, 1000 Küp üstü her alışverişte 200 geri, arkadaş daveti 500.'],
  [/plan|hyper|ultra|max/i, 'Planlar Planım penceresinde, Küp ile 30 günlüğüne alınır. HyperAI en üst plan (30.000 Küp).'],
  [/davet|arkada/i, 'Profil > Arkadaş davet et’teki linki paylaş. Arkadaşın kayıt olunca ikiniz de Max+ ve 500 Küp alırsınız (en fazla 3 davet).'],
  [/mağaza|magaza|kostüm|skin|maskot/i, 'Maskot kostümleri Mağaza’da Küp ile alınır (Planım ya da Profil > Mağaza). Aldığın kostümü Profil’den değiştirebilirsin.'],
  [/api|anahtar|geliştir/i, 'API platformu Max+ ve üstünde: Profil > API platformu. Belgeler Kübik Docs sayfasında.'],
  [/cube|telefon|qr|bağlan/i, 'Cube Connect: bilgisayardaki giriş ekranında “Cube Connect” yıldız kodunu açıp telefondaki Kübik uygulamasıyla okut.'],
  [/e-?posta|mail/i, 'Profil > E-posta: adresini yaz, gelen 6 haneli kodu gir.'],
  [/limit|token|sınır|yenile/i, 'Limit 5 saatlik oturum ve haftalık. Model adının yanındaki halkaya tıkla; “Token yenile” 1000 Küp.'],
  [/sil|kvkk|gizlilik/i, 'Hesabını “Gizlilik ve koşullar” penceresinin en altından silebilirsin.'],
  [/code|kod/i, 'Kübik Code Max+ ve üstünde. Kübik dosyaları yazar, VM’de çalıştırır, sağdaki listeden indirirsin.'],
  [/work/i, 'Kübik Work: sana özel linkten yazanlara yapay zekâ cevap verir. Max ve üstü planlarda istenir.'],
];
const support = {conversation: null, busy: false};
function supportSay(role, text) { const b = node('div', 'bubble ' + (role === 'user' ? 'me' : 'them')); b.append(node('p', '', text)); $('#supportMessages').append(b); $('#supportMessages').scrollTop = 1e9; return b; }
$('#supportButton').onclick = () => {
  const panel = $('#supportPanel'); panel.hidden = !panel.hidden;
  if (!panel.hidden && !$('#supportMessages').childElementCount) supportSay('ai', 'Selam! Ben Kübit. Küp, planlar, davet, API, Cube Connect… ne sorarsan.');
  if (!panel.hidden) $('#supportInput').focus();
};
$('#closeSupport').onclick = () => { $('#supportPanel').hidden = true; };
$('#supportForm').onsubmit = async event => {
  event.preventDefault(); const text = $('#supportInput').value.trim(); if (!text || support.busy) return;
  $('#supportInput').value = ''; supportSay('user', text);
  const known = FAQ.find(([pattern]) => pattern.test(text));
  if (known) {
    const b = supportSay('ai', known[1]), more = node('button', 'text-button', 'Yapay zekâya sor →'); more.type = 'button';
    more.onclick = () => { more.remove(); askSupport(text); }; b.append(more); return;
  }
  askSupport(text);
};
async function askSupport(text) {
  support.busy = true; const wait = supportSay('ai', 'Düşünüyorum…');
  try {
    const started = await api('/chat', {method: 'POST', body: {message: text, mode: 'support', conversation_id: support.conversation, language: lang}});
    support.conversation = started.conversation_id;
    for (;;) { const job = await api('/jobs/' + started.job_id);
      if (job.state === 'done') { wait.querySelector('p').textContent = job.result.text; break; }
      if (job.state === 'error' || job.state === 'canceled') { wait.querySelector('p').textContent = job.status; break; }
      wait.querySelector('p').textContent = job.status || 'Düşünüyorum…'; await delay(1500); }
  } catch (error) { wait.querySelector('p').textContent = error.message; }
  finally { support.busy = false; }
}

// ---------------------------------------------------------------- girişte: uyarılar, maskot, tur, kapanış sayacı
let lifespanData = null;
addEventListener('kubik:enter', async () => {
  document.querySelectorAll('.maskot.mine').forEach(m => { m.dataset.maskot = account?.skin || 'klasik'; drawMaskot(m); });
  if (account?.role !== 'guest') {
    try { const {warnings} = await api('/notices', {method: 'POST'}); if (warnings.length) showWarning(warnings); } catch {}
  }
  try { lifespanData = await api('/models/lifespan'); } catch {}
  setTimeout(() => startTour(false), 600);
});
function showWarning(warnings) {
  $('#warningText').textContent = `Son mesaj${warnings.length > 1 ? 'ların' : 'ın'} Kübik’in kurallarına aykırı bulundu (${warnings.map(w => w.reason.replace(/^(YZ denetimi|Otomatik filtre): /, '')).join(', ')}). Tekrarlanırsa hesabın askıya alınabilir. Yanlışlık olduğunu düşünüyorsan: kubik.iletisim@gmail.com`;
  $('#warningDialog').showModal();
}
$('#closeWarning').onclick = () => $('#warningDialog').close();
function countdown(seconds) {
  const d = Math.floor(seconds / 86400), h = Math.floor(seconds % 86400 / 3600), m = Math.floor(seconds % 3600 / 60), s = Math.floor(seconds % 60);
  return d ? `${d} gün ${h} sa ${m} dk` : `${h} sa ${m} dk ${s} sn`;
}
setInterval(() => {
  const banner = $('#shutdownBanner'); if (!banner) return;
  const name = siteStatus.ensemble?.name, model = lifespanData?.models.find(m => m.name === name);
  const end = model?.shutdown ? new Date(model.shutdown + 'T00:00:00+03:00').getTime() : 0, left = (end - Date.now()) / 1000;
  banner.hidden = !end || left > 60 * 86400;
  if (!banner.hidden) banner.textContent = left > 0 ? `⏳ ${name} ${countdown(left)} sonra kapanıyor. Ayrıntılar: Model ömürleri sayfası.` : `${name} kullanımdan kalktı.`;
}, 1000);

// ---------------------------------------------------------------- Cube Connect
const isApp = /KubikApp/.test(navigator.userAgent), isPhone = /Android|iPhone|iPad|Mobile/.test(navigator.userAgent);
$('#cubeConnect').hidden = isApp || isPhone;
if (/Android/.test(navigator.userAgent) && !isApp) document.querySelectorAll('.android-download').forEach(e => { e.hidden = false; });
const cube = {timer: null, frame: 0};
async function openCube() {
  const dialog = $('#cubeDialog'), canvas = $('#cubeCanvas'); if (!dialog.open) dialog.showModal();
  clearTimeout(cube.timer); cancelAnimationFrame(cube.frame);
  try {
    const link = await api('/cube/new', {method: 'POST'}), start = performance.now();
    (function paint(now) { kubikCube.draw(canvas, link.code, now - start);
      const left = Math.max(0, link.expires - Date.now() / 1000); $('#cubeTimer').textContent = `${Math.ceil(left)} sn`;
      if (dialog.open) cube.frame = requestAnimationFrame(paint); })(start);
    (async function poll() {
      if (!dialog.open) return;
      try { const state = await api('/cube/' + link.id);
        if (state.state === 'approved') { dialog.close(); cancelAnimationFrame(cube.frame); toast('Cube Connect ile giriş yapıldı.'); await enter(); return; }
        if (state.state === 'expired') { openCube(); return; } } catch {}
      cube.timer = setTimeout(poll, 2000);
    })();
  } catch (error) { toast(error.message); dialog.close(); }
}
$('#cubeConnect').onclick = openCube;
$('#closeCube').onclick = () => { $('#cubeDialog').close(); clearTimeout(cube.timer); cancelAnimationFrame(cube.frame); };
let stopScan = null;
async function openScanner() {
  $('#scanDialog').showModal(); $('#scanStatus').textContent = 'Bilgisayardaki yıldız kodunu çerçeveye al.';
  try {
    stopScan = await kubikCube.scan($('#scanVideo'), async code => {
      $('#scanDialog').close(); stopScan = null;
      if (!confirm('Bilgisayarda Kübik hesabınla giriş yapılsın mı? Kodu sen açmadıysan Hayır de.')) return;
      try { const r = await api('/cube/approve', {method: 'POST', body: {code}}); toast(`${r.agent} bilgisayarında giriş yapıldı.`); }
      catch (error) { toast(error.message); }
    });
  } catch { $('#scanStatus').textContent = 'Kamera açılamadı. Uygulamaya kamera izni ver.'; }
}
$('#closeScan').onclick = () => { stopScan?.(); stopScan = null; $('#scanDialog').close(); };

// ---------------------------------------------------------------- mağaza
async function openShop() {
  if (account?.role === 'guest') return toast('Mağaza için hesap oluşturman gerekiyor.');
  $('#plansDialog').close(); const dialog = $('#shopDialog'); if (!dialog.open) dialog.showModal();
  try {
    const p = await api('/profile'), grid = $('#shopGrid');
    $('#shopBalance').textContent = `${p.kup.toLocaleString(lang)} Küp`;
    grid.replaceChildren(...Object.entries(SKINS).filter(([id]) => id !== 'klasik').map(([id, skin]) => {
      const owned = p.skins.includes(id), card = node('div', 'shop-item ' + skin.rarity + (p.skin === id ? ' worn' : ''));
      const b = node('button', owned ? 'secondary' : 'primary', owned ? (p.skin === id ? 'Giyili' : 'Giy') : `${skin.price.toLocaleString(lang)} Küp`);
      b.type = 'button'; b.disabled = (owned && p.skin === id) || (!owned && p.kup < skin.price);
      b.onclick = async () => { b.disabled = true;
        try {
          if (owned) await api('/profile/skin', {method: 'PUT', body: {skin: id}});
          else { if (!confirm(`${skin.name} kostümü ${skin.price} Küp. Alınsın mı?`)) { b.disabled = false; return; } await api('/shop/skin', {method: 'POST', body: {skin: id}}); toast(`${skin.name} senin! Hemen giydirdim.`); }
          wearSkin(id); account.skin = id; refreshAccount().catch(() => {}); openShop(); peekAnimate('dans');
        } catch (error) { toast(error.message); b.disabled = false; } };
      card.append(maskot(id, 'big'), node('strong', '', skin.name), node('small', '', RARITY[skin.rarity]), b);
      return card;
    }));
  } catch (error) { $('#shopGrid').replaceChildren(node('p', 'muted', error.message)); }
}
$('#closeShop').onclick = () => $('#shopDialog').close();

// ---------------------------------------------------------------- yazı çubuğunun üstündeki maskot: Code'da hep orada, Chat'te ara sıra uğrar
const PEEK_LINES = ['Selam! Bir şey mi yazıyorsun?', 'Kod derlendi mi? Ben buradayım.', 'Bugün ne üretiyoruz?', 'Bir bug gördüm sandım… şakaydı.',
  'Ben Kübit! Küp biriktir, bana kostüm al.', 'Ctrl+S’ye basmayı unutma. Ha, burada gerek yok.', 'Ayarlar’dan daha derin düşünmemi isteyebilirsin.', 'Yıldızları seviyorum. ✦'];
const PEEK_MOVES = ['zipla', 'don', 'salla', 'dans', 'sasir'];
function peekAnimate(move) {
  const m = $('#peekButton .maskot'); if (!m) return;
  m.classList.remove(...PEEK_MOVES); void m.offsetWidth; m.classList.add(move);
  setTimeout(() => m.classList.remove(move), 1300);
}
function peekSay(text) { const say = $('#peekSay'); say.textContent = text; say.hidden = false; clearTimeout(peekSay.timer); peekSay.timer = setTimeout(() => { say.hidden = true; }, 3500); }
$('#peekButton').onclick = () => { peekAnimate(PEEK_MOVES[Math.floor(Math.random() * PEEK_MOVES.length)]); peekSay(PEEK_LINES[Math.floor(Math.random() * PEEK_LINES.length)]); };
function peekVisit() {      // Chat'te: arada bir gelip el sallar, sonra gider
  const peek = $('#peek'); if (document.body.classList.contains('code-theme') || !account) return;
  peek.hidden = false; peek.classList.add('visit'); peekAnimate('salla');
  setTimeout(() => { peek.classList.remove('visit'); if (!document.body.classList.contains('code-theme')) peek.hidden = true; }, 7000);
}
new MutationObserver(() => { $('#peek').hidden = !document.body.classList.contains('code-theme') && !$('#peek').classList.contains('visit'); })
  .observe(document.body, {attributes: true, attributeFilter: ['class']});
new MutationObserver(records => {
  if (records.some(r => [...r.addedNodes].some(n => n.classList?.contains('assistant'))) && Math.random() < .2) peekVisit();
}).observe($('#messages'), {childList: true});

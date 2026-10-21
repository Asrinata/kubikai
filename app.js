'use strict';
const $ = s => document.querySelector(s);
let account = null, registerMode = false, current = null, job = null, mode = 'chat', siteStatus = {};
let selectedModel = null;
const store = { get(k, d) { try { return localStorage.getItem(k) ?? d; } catch { return d; } }, set(k, v) { try { localStorage.setItem(k, v); } catch {} } };
let lang = store.get('kubik-lang', 'tr');
const T = {
  tr: {new_chat:'Yeni sohbet', your_chats:'SOHBETLERİN', search_chats:'Sohbetlerde ara', my_plan:'Planım', plan_requests:'Abonelik istekleri', memory:'Hafızam',
    attach:'+ Dosya / fotoğraf', thinking:'Düşünme', fast:'Hızlı', balanced:'Dengeli', deep:'Derin', ultra_think:'Ultra Think', web_search:'Web araması',
    web_off:'Web kapalı', web_quick:'Web · Hızlı', web_balanced:'Web · Dengeli', web_deep:'Web · Derin', deepsearch:'Deep Search', supersearch:'Super Search',
    settings:'Ayarlar', stop:'Durdur', footnote:'Kübik hata yapabilir. Önemli bilgileri kaynaklarıyla kontrol et.', built_on:'Kübik 2.0 şu açık modellerle çalışır:', privacy_title:'Gizlilik, çerezler ve koşullar', privacy_link:'Gizlilik ve koşullar', delete_title:'Hesabımı ve verilerimi sil', delete_help:'Sohbetlerin, hafızan, dosyaların ve Code projelerin kalıcı olarak silinir. Geri alınamaz.', password_ph:'Parolan', delete_button:'Kalıcı olarak sil', delete_confirm:'Hesabın ve tüm verilerin kalıcı olarak silinecek. Emin misin?', quality:'Verimlilik',
    quality_help:'Hız ile doğruluk arasındaki denge. Ultra Reliable her soruyu birkaç kez çözüp en tutarlı cevabı seçer.', q_efficient:'Verimli (en hızlı)',
    q_reliable:'Güvenilir (kontrol turu)', q_ultrareliable:'Ultra Reliable', multitask:'Yapay zeka multitask',
    multitask_help:'Kübik 2.0 bir soruyu kaç modele sorsun? Cevaplar karşılaştırılır, en iyisi seçilir. Çok model = daha yavaş.', m_auto:'Otomatik',
    m_cok_az:'Çok az (1 YZ)', m_az:'Az (2 YZ)', m_orta:'Orta (3 YZ)', m_fazla:'Fazla (4 YZ)', m_cok_fazla:'Çok fazla (5 YZ)', m_ultraai:'UltraAI (6 YZ)', m_hepsi:'Hepsi',
    language:'Dil', language_help:'Arayüz ve cevap dili.', plans:'Planlar', plans_help:'Planlar ücretsizdir; istek gönderirsin, onaylanınca planın değişir. Haklar her gün gece yarısı yenilenir.',
    members:'Plan sahipleri', hibernating_banner:'Sunucu uyku modunda. Sohbete devam etmek için sohbet penceresine bir şey yaz.', hibernating:'uyku modunda',
    ready:'hazır', selected:'seçildi', not_installed:'kurulu değil', unreachable:'Sunucuya ulaşılamıyor', ask:'Kübik’e bir şey sor…', ask_code:'Ne geliştirmek istiyorsun?',
    commands:'Komutlar', commands_help:'Bir modeli tek başına kullanmak için mesajın başına komut yaz:', unlimited:'Sınırsız (token bitene kadar)', per_day:'/ gün',
    storage:'depolama', tokens:'token', messages_l:'mesaj', photos:'fotoğraf', wait_yes:'Yoğunlukta sıra beklenir', wait_no:'Bekleme yok', code_yes:'Kübik Code var', code_no:'Code yok',
    current_plan:'Mevcut planın', request:'İstek gönder', requested:'İstek gönderildi · geri çek', need_account:'Plan için hesap aç', accept:'Kabul', reject:'Reddet',
    no_requests:'Bekleyen istek yok.', no_members:'Henüz plan sahibi yok.', request_sent:'İsteğin gönderildi. Onaylanınca planın değişir.', ultrapack_locked:'Ultrapack',
    consulted:'model karşılaştırıldı', chose:'seçilen', welcome_eyebrow:'BİRLİKTE BİR ADIM İLERİ', welcome_title:'Aklında ne var?', welcome_sub:'Bir soruyla başla. Bir fikri geliştir. Yeni bir şey keşfet.',
    code_eyebrow:'FİKRİNDEN İLK SATIRA', code_title:'Birlikte ne üretelim?', code_sub:'Bir fikir anlat. Kübik planlasın, kodlasın ve denesin.', today:'Bugün'},
  en: {new_chat:'New chat', your_chats:'YOUR CHATS', search_chats:'Search chats', my_plan:'My plan', plan_requests:'Subscription requests', memory:'Memory',
    attach:'+ File / photo', thinking:'Thinking', fast:'Fast', balanced:'Balanced', deep:'Deep', ultra_think:'Ultra Think', web_search:'Web search',
    web_off:'Web off', web_quick:'Web · Fast', web_balanced:'Web · Balanced', web_deep:'Web · Deep', deepsearch:'Deep Search', supersearch:'Super Search',
    settings:'Settings', stop:'Stop', footnote:'Kübik can make mistakes. Check important information with its sources.', built_on:'Kübik 2.0 runs on these open models:', privacy_title:'Privacy, cookies and terms', privacy_link:'Privacy and terms', delete_title:'Delete my account and data', delete_help:'Your chats, memory, files and Code projects are permanently deleted. This cannot be undone.', password_ph:'Your password', delete_button:'Delete permanently', delete_confirm:'Your account and all your data will be permanently deleted. Are you sure?', quality:'Efficiency',
    quality_help:'Balance between speed and accuracy. Ultra Reliable solves each question several times and picks the most consistent answer.', q_efficient:'Efficient (fastest)',
    q_reliable:'Reliable (review pass)', q_ultrareliable:'Ultra Reliable', multitask:'AI multitask',
    multitask_help:'How many models should Kübik 2.0 ask? Answers are compared and the best one wins. More models = slower.', m_auto:'Automatic',
    m_cok_az:'Very few (1 AI)', m_az:'Few (2 AI)', m_orta:'Medium (3 AI)', m_fazla:'Many (4 AI)', m_cok_fazla:'Very many (5 AI)', m_ultraai:'UltraAI (6 AI)', m_hepsi:'All',
    language:'Language', language_help:'Interface and reply language.', plans:'Plans', plans_help:'Plans are free: send a request and your plan changes once it is approved. Limits reset every day at midnight.',
    members:'Plan holders', hibernating_banner:'Server is hibernating. To continue chatting, write something in the chat window.', hibernating:'hibernating',
    ready:'ready', selected:'selected', not_installed:'not installed', unreachable:'Server unreachable', ask:'Ask Kübik anything…', ask_code:'What do you want to build?',
    commands:'Commands', commands_help:'To use a single model, start your message with a command:', unlimited:'Unlimited (until tokens run out)', per_day:'/ day',
    storage:'storage', tokens:'tokens', messages_l:'messages', photos:'photos', wait_yes:'Waits in queue when busy', wait_no:'No waiting', code_yes:'Kübik Code included', code_no:'No Code',
    current_plan:'Your current plan', request:'Send request', requested:'Request sent · withdraw', need_account:'Create an account first', accept:'Accept', reject:'Reject',
    no_requests:'No pending requests.', no_members:'No plan holders yet.', request_sent:'Request sent. Your plan changes once it is approved.', ultrapack_locked:'Ultrapack',
    consulted:'models compared', chose:'chosen', welcome_eyebrow:'ONE STEP FURTHER, TOGETHER', welcome_title:'What’s on your mind?', welcome_sub:'Start with a question. Grow an idea. Discover something new.',
    code_eyebrow:'FROM IDEA TO FIRST LINE', code_title:'What shall we build?', code_sub:'Describe an idea. Kübik plans, codes and tests it.', today:'Today'}
};
const t = key => T[lang]?.[key] ?? T.tr[key] ?? key;
const ULTRAPACK = {effort:['ultra'], searchEffort:['supersearch'], quality:['ultrareliable'], multitask:['ultraai','hepsi']};
function applyLanguage() {
  document.documentElement.lang = lang;
  document.querySelectorAll('[data-i18n]').forEach(element => { element.dataset.base = t(element.dataset.i18n); element.textContent = element.dataset.base; });
  document.querySelectorAll('[data-i18n-ph]').forEach(element => { element.placeholder = t(element.dataset.i18nPh); });
  document.querySelectorAll('[data-lang]').forEach(element => { element.hidden = element.dataset.lang !== lang; });
  $('#prompt').placeholder = mode === 'code' ? t('ask_code') : t('ask');
  lockUltrapack();
}
function lockUltrapack() {
  const allowed = !!account?.plan?.ultrapack;
  for (const [id, values] of Object.entries(ULTRAPACK)) {
    const select = $('#' + id); if (!select) continue;
    for (const option of select.options) if (values.includes(option.value)) {
      option.disabled = !allowed; option.textContent = (option.dataset.base || option.textContent) + (allowed ? '' : ' · ' + t('ultrapack_locked'));
    }
    if (select.selectedOptions[0]?.disabled) select.value = {effort:'balanced', quality:'balanced', searchEffort:'off', multitask:'auto'}[id];
  }
}
const modelName = id => id === 'kubik-2.0' || id === 'auto' ? 'Kübik 2.0' : (siteStatus.models?.find(model => model.id === id)?.name || (id === 'apex-5.5' ? 'Apex 5.5' : 'Kübik 1.0'));
const formatBytes = value => value >= 1e9 ? (value / 1e9).toLocaleString(lang, {maximumFractionDigits: 1}) + ' GB' : value >= 1e6 ? (value / 1e6).toLocaleString(lang, {maximumFractionDigits: 1}) + ' MB' : (value / 1e3).toLocaleString(lang, {maximumFractionDigits: 1}) + ' KB';
const delay = ms => new Promise(resolve => setTimeout(resolve, ms));
function toast(message) { $('#toast').textContent = message; $('#toast').classList.add('show'); setTimeout(() => $('#toast').classList.remove('show'), 5000); }
async function api(path, options = {}) {
  const headers = {'X-Kubik-Request': '1', ...(options.headers || {})};
  if (options.body && typeof options.body !== 'string') { headers['Content-Type'] = 'application/json'; options.body = JSON.stringify(options.body); }
  const response = await kubikTransport.fetch('/api' + path, {...options, headers});
  const value = await response.json();
  if (!response.ok) { const text = typeof value.detail === 'string' ? value.detail : 'Bilgileri kontrol ederek tekrar dene.'; throw new Error(text); }
  if (value.token) kubikTransport.setToken(value.token);
  return value;
}
function node(tag, className, text) { const element = document.createElement(tag); if (className) element.className = className; if (text != null) element.textContent = text; return element; }
function welcome() {
  $('#messages').replaceChildren();
  const box = node('div', 'welcome');
  box.append(node('span', 'cube large'), node('p', 'eyebrow', t(mode === 'code' ? 'code_eyebrow' : 'welcome_eyebrow')));
  box.append(node('h1', '', t(mode === 'code' ? 'code_title' : 'welcome_title')));
  box.lastChild.style.whiteSpace = 'pre-line';
  box.append(node('p', 'muted', t(mode === 'code' ? 'code_sub' : 'welcome_sub')));
  const suggestions = node('div', 'suggestions');
  for (const [title, subtitle, prompt] of (lang === 'en' ? [
    ['Develop an idea', 'Let’s start from the first step', 'Where should I start to turn an idea into a project?'],
    ['Understand a topic', 'Make the hard parts simple', 'Teach me a new topic with simple examples.'],
    ['Make a plan', 'Draw a path through scattered ideas', 'Help me prepare a practical plan to reach my goal.'],
    ['Write together', 'Find your words, sharpen your writing', 'Help me make a text I wrote clearer and more natural.']
  ] : [
    ['Bir fikir geliştirelim', 'İlk adımdan birlikte başlayalım', 'Bir fikri projeye dönüştürmek için nereden başlamalıyım?'],
    ['Bir konuyu anlayalım', 'Zor görüneni birlikte sadeleştirelim', 'Bana yeni bir konuyu basit örneklerle öğret.'],
    ['Bir plan hazırlayalım', 'Dağınık fikirlerine bir yol çiz', 'Hedefime ulaşmam için uygulanabilir bir plan hazırlamama yardım et.'],
    ['Birlikte yazalım', 'Kelimelerini bul, anlatımını güçlendir', 'Yazdığım bir metni daha anlaşılır ve doğal hâle getirmeme yardım et.']
  ])) { const button = node('button'); button.append(node('span', '', title), node('small', '', subtitle)); button.onclick = () => { $('#prompt').value = prompt; $('#prompt').focus(); }; suggestions.append(button); }
  if (mode === 'code') { suggestions.replaceChildren(); for (const [title, prompt] of [['Bir web sitesi yap', 'Sade ve kullanışlı bir web sitesi oluştur. Dosyalarını yaz ve kontrol et.'], ['Bir projeyi tamamla', 'Projem için önce kısa bir plan çıkar, sonra dosyaları oluşturup çalışmasını kontrol et.']]) { const button=node('button'); button.append(node('span','',title), node('small','','Planla, geliştir, dene')); button.onclick=()=>{$('#prompt').value=prompt;$('#prompt').focus();}; suggestions.append(button); } }
  box.append(suggestions); $('#messages').append(box);
}
function updateWorkspaceLabel() { const label = $('.terminal-heading span'); if (label) label.textContent = '~/workspace/' + (current || 'default'); }
function addMessage(role, text, sources = [], extra = {}) {
  $('#welcome')?.remove(); $('.messages > .welcome')?.remove();
  const row = node('article', 'message ' + role);
  if (role === 'assistant') {
    const label = node('div', 'message-label'); label.append(node('span', 'cube'), node('span', '', extra.model_id ? modelName(extra.model_id) : 'KÜBİK'));
    if (extra.consulted?.length > 1) label.append(node('span', 'consulted', `· ${extra.consulted.length} ${t('consulted')}, ${t('chose')}: ${extra.answered_by}`));
    row.append(label);
  }
  if (role === 'user') row.textContent = text;
  else {
    const regex = /```([\w+-]*)\n?([\s\S]*?)```/g; let end = 0, match;
    while ((match = regex.exec(text))) {
      if (match.index > end) row.append(node('div', 'message-text', text.slice(end, match.index)));
      const pre = node('pre'); pre.append(node('code', '', match[2])); row.append(pre);
      const actions = node('div', 'code-actions'), copy = node('button', '', 'Kopyala');
      const code = match[2], language = match[1].toLowerCase();
      copy.onclick = async () => { try { await navigator.clipboard.writeText(code); toast('Kod kopyalandı.'); } catch { toast('Kod bloğunu seçerek kopyalayabilirsin.'); } };
      actions.append(copy);
      if (mode === 'code') {
        const prior = text.slice(0, match.index).split(/\r?\n/).reverse().find(line => /^\s*Dosya:\s*\S+/.test(line));
        const suggested = prior ? prior.replace(/^\s*Dosya:\s*/, '').trim().replace(/^[`*]+|[`*]+$/g, '') : (language === 'python' || language === 'py' ? 'main.py' : language === 'java' ? 'Main.java' : 'app/' + (language || 'txt') + '-file');
        const save = node('button', '', 'VM’ye kaydet');
        save.onclick = async () => { const path = window.prompt('Bu kod bloğunu hangi dosya yoluna kaydedeyim?', suggested); if (!path) return; save.disabled = true; try { await api('/vm/files', {method:'POST',body:{workspace_id:current||'default',path:path.trim(),content:code}}); toast('Dosya VM’ye kaydedildi. Kod çalıştırılmadı.'); } catch(error) { toast(error.message); } finally { save.disabled = false; } };
        actions.append(save);
      }
      row.append(actions); end = regex.lastIndex;
    }
    if (end < text.length) row.append(node('div', 'message-text', text.slice(end)));
    if (sources.length) {
      const links = node('div', 'sources');
      sources.forEach((source, i) => { try { const url = new URL(source.url); if (!['https:', 'http:'].includes(url.protocol)) return; const link = node('a', '', `${i + 1}. ${source.title || url.hostname}`); link.href = url.href; link.target = '_blank'; link.rel = 'noopener noreferrer'; link.title = source.title; links.append(link); } catch {} });
      row.append(links);
    }
    if (extra.search_note) row.append(node('div', 'message-note', extra.search_note));
    if (extra.corrected) row.append(node('div', 'message-note', 'Anlaşılan metin: ' + extra.corrected));
    if (Array.isArray(extra.command_log) && extra.command_log.length) {
      $('#terminalOutput').textContent = extra.command_log.map((entry, i) =>
        `[${i + 1}] $ ${entry.command}\n${entry.output}${entry.exit_code ? `\nÇıkış kodu: ${entry.exit_code}` : ''}`
      ).join('\n\n');
    }
  }
  decorateMessage(row, role, text, extra);
  $('#messages').append(row); $('#messages').scrollTop = $('#messages').scrollHeight;
}
async function refreshAccount() {
  account = await api('/me');
  const guest = account.role === 'guest';
  $('#memoryButton').hidden = guest;
  $('#memoryUsage').textContent = formatBytes(account.memory_bytes) + ' / ' + formatBytes(account.memory_limit);
  const codeAllowed = !guest && account.plan?.code;
  $('#mode').querySelector('[value=code]').disabled = !codeAllowed;
  document.querySelector('[data-mode=code]').hidden = !codeAllowed;
  $('#planName').textContent = account.plan?.name + (account.pending_tier ? ' · ⏳' : '');
  $('#requestsButton').hidden = !account.owner;
  if (account.owner) api('/subscription/requests').then(data => { $('#requestCount').textContent = data.pending.length ? data.pending.length + ' ⏳' : ''; }).catch(() => {});
  lockUltrapack();
  $('#logout').title = guest ? 'Hesabına giriş yap' : 'Çıkış yap';
  $('#accountName').textContent = guest ? 'Misafir' : account.username; $('#avatar').textContent = account.username[0].toUpperCase();
  $('#accountRole').textContent = account.role === 'admin' ? 'Yönetici' : guest ? 'Code ve hafıza için giriş yap' : 'Kübik hesabı'; $('#adminLink').hidden = account.role !== 'admin';
  const used = account.usage, limits = account.limits;
  const messageLimit = limits.daily_messages >= 1e9 ? '∞' : limits.daily_messages;
  $('#quota').textContent = account.role === 'admin' ? `${t('today')} ${used.messages} ${t('messages_l')} · ${used.tokens.toLocaleString(lang)} ${t('tokens')}` : `${t('today')} ${used.messages} / ${messageLimit} ${t('messages_l')}\n${used.tokens.toLocaleString(lang)} / ${limits.daily_tokens.toLocaleString(lang)} ${t('tokens')}\n${used.photos || 0} / ${limits.daily_photos} ${t('photos')}`;
  $('#quota').style.whiteSpace = 'pre-line';
}
async function refreshConversations() {
  const rows = await api('/conversations'); $('#conversationList').replaceChildren();
  const visible = rows.filter(row => row.mode === mode);
  const empty = node('div', 'empty-list', visible.length ? '' : 'İlk sohbetin burada görünecek.'); empty.id = 'conversationEmpty'; empty.hidden = Boolean(visible.length); $('#conversationList').append(empty);
  for (const row of visible) {
    const entry = node('div', 'conversation-entry');
    const button = node('button', row.id === current ? 'conversation-open active' : 'conversation-open', row.title); button.title = row.title;
    button.onclick = async () => { if (job) return toast('Önce devam eden yanıtı durdur.'); try { const data = await api('/conversations/' + row.id); current = row.id; updateWorkspaceLabel(); $('#messages').replaceChildren(); renderHistory(data); refreshConversations(); $('.sidebar').classList.remove('open'); } catch (error) { toast(error.message); } };
    const remove = node('button', 'conversation-delete', '×'); remove.type = 'button'; remove.title = 'Sohbeti ve dosyalarını sil'; remove.setAttribute('aria-label', row.title + ' sohbetini sil');
    remove.onclick = async () => { if (job) return toast('Önce devam eden yanıtı durdur.'); if (!confirm('Bu sohbet, mesajları ve ona ait proje dosyaları kalıcı olarak silinsin mi?')) return; try { await api('/conversations/' + row.id, {method:'DELETE'}); if (current === row.id) { current = null; welcome(); } await refreshConversations(); toast('Sohbet ve bağlı dosyaları silindi.'); } catch (error) { toast(error.message); } };
    entry.append(button, remove); $('#conversationList').append(entry);
  }
  filterConversations();
}
function filterConversations() {
  const query = ($('#conversationSearch')?.value || '').trim().toLocaleLowerCase('tr-TR');
  let shown = 0;
  document.querySelectorAll('#conversationList .conversation-entry').forEach(entry => {
    const match = entry.querySelector('.conversation-open').textContent.toLocaleLowerCase('tr-TR').includes(query);
    entry.hidden = !match; if (match) shown++;
  });
  const empty = $('#conversationEmpty');
  if (empty) { empty.hidden = shown > 0; empty.textContent = query ? 'Eşleşen sohbet bulunamadı.' : 'İlk sohbetin burada görünecek.'; }
}
$('#conversationSearch')?.addEventListener('input', filterConversations);
async function refreshSite() {
  try {
    siteStatus = await api('/status');
    $('#searchControl').hidden = !siteStatus.search_enabled || siteStatus.search_max_effort === 'off';
    if (siteStatus.admin_url) $('#adminLink').href = siteStatus.admin_url;
    $('#accessBadge').textContent = siteStatus.access_mode === 'beta' ? 'BETA · ERKEN ERİŞİM' : 'HERKESE AÇIK';
    $('#keyLabel').hidden = siteStatus.access_mode !== 'beta';
    $('#maintenance').hidden = !siteStatus.maintenance;
    $('#maintenance').textContent = siteStatus.maintenance_message;
    // Tek model görünür: Kübik 2.0. Tek tek modeller yalnız /komut/ ile kullanılır.
    selectedModel = 'kubik-2.0';
    const picker = $('#modelPicker'), sleeping = !!siteStatus.hibernating;
    const option = node('option', '', (sleeping ? '❄ ' : '') + 'Kübik 2.0' + (sleeping ? ' · ' + t('hibernating') : ''));
    option.value = selectedModel; if (sleeping) option.style.color = '#b58700';
    picker.replaceChildren(option);
    picker.value = selectedModel;
    picker.disabled = !!job;
    const routedId = selectedModel === 'kubik-2.0' ? siteStatus.model_routing?.[mode] : selectedModel;
    const chosen = siteStatus.models?.find(model => model.id === routedId);
    const status = $('#modelStatus'); status.replaceChildren();
    if (sleeping) { status.append(node('span', 'snowflake', '❄'), node('span', 'model-hibernating', ' ' + modelName(selectedModel) + ' · ' + t('hibernating'))); }
    else status.textContent = !chosen?.installed ? modelName(selectedModel) + ' · ' + t('not_installed') : modelName(selectedModel) + ' · ' + (chosen.loaded ? t('ready') : t('selected'));
    $('#hibernation').hidden = !sleeping || !!job; $('#hibernationText').textContent = t('hibernating_banner');
    $('#send').disabled = !!job || siteStatus.maintenance || !chosen?.installed;
    $('#startVM').disabled = siteStatus.maintenance;
    const ranks = ['off', 'quick', 'balanced', 'deep', 'deepsearch', 'supersearch'];
    const maxRank = siteStatus.search_max_effort === 'deep' ? ranks.length - 1 : ranks.indexOf(siteStatus.search_max_effort);
    [...$('#searchEffort').options].forEach(option => { option.disabled = option.value !== 'off' && (!siteStatus.search_enabled || ranks.indexOf(option.value) > maxRank); });
    // Lisans gereği: hangi açık modellerin çalıştığı ve Llama için "Built with Llama" görünür olmalı
    const used = (siteStatus.models || []).filter(m => m.installed && m.extra).map(m => m.name);
    $('#modelCredits').textContent = used.length ? '· ' + t('built_on') + ' ' + used.join(', ') + (used.some(n => /llama/i.test(n)) ? ' · Built with Llama' : '') : '';
    lockUltrapack();
    if ($('#searchEffort').selectedOptions[0].disabled) $('#searchEffort').value = 'off';
    if (!account && siteStatus.maintenance) $('#authError').textContent = siteStatus.maintenance_message;
  } catch { $('#modelStatus').textContent = t('unreachable'); }
}
$('#modelPicker').onchange = () => {
  if (job) { $('#modelPicker').value = selectedModel; return toast('Önce devam eden yanıtı durdur.'); }
  selectedModel = $('#modelPicker').value;
  store.set('kubik-model-v3', selectedModel);
  refreshSite();
};
async function enter() { await refreshAccount(); $('#auth').hidden = true; $('#workspace').hidden = false; applyLanguage(); welcome(); await refreshConversations(); await refreshSite(); }
$('#authToggle').onclick = () => { registerMode = !registerMode; $('#authTitle').textContent = registerMode ? 'Kübik’e hoş geldin.' : 'Tekrar hoş geldin.'; $('#authSubmit').textContent = registerMode ? 'Hesap oluştur' : 'Giriş yap'; $('#authToggle').textContent = registerMode ? 'Zaten hesabın var mı? Giriş yap' : 'Hesabın yok mu? Kayıt ol'; $('#password').autocomplete = registerMode ? 'new-password' : 'current-password'; $('#authError').textContent = ''; };
$('#authForm').onsubmit = async event => { event.preventDefault(); $('#authError').textContent = ''; $('#authSubmit').disabled = true; try { await api('/auth/' + (registerMode ? 'register' : 'login'), {method:'POST', body:{username:$('#username').value, password:$('#password').value, access_key:$('#accessKey').value.trim()}}); $('#password').value = ''; await enter(); } catch(error) { $('#authError').textContent = error.message; } finally { $('#authSubmit').disabled = false; } };
$('#logout').onclick = async () => { await api('/auth/logout', {method:'POST'}); kubikTransport.clearToken(); location.reload(); };
$('#newChat').onclick = () => { if (job) return toast('Önce devam eden yanıtı durdur.'); current = null; updateWorkspaceLabel(); welcome(); refreshConversations(); $('#prompt').focus(); };
async function changeMode(next) {
  if (job) return toast('Önce devam eden yanıtı durdur.');
  if (next === 'code' && account?.role === 'guest') return toast('Code için hesap oluşturman gerekiyor.');
  if (next === mode) return;
  $('#modeTransition').classList.add('active');
  await delay(matchMedia('(prefers-reduced-motion: reduce)').matches ? 0 : 150);
  mode = next; $('#mode').value = mode; current = null;
  document.body.classList.toggle('code-theme', mode === 'code');
  document.querySelectorAll('[data-mode]').forEach(button => { button.classList.toggle('selected', button.dataset.mode === mode); button.setAttribute('aria-pressed', String(button.dataset.mode === mode)); });
  $('#modeTitle').textContent = mode === 'code' ? 'Kübik Code' : 'Kübik Chat';
  $('#codePanel').hidden = mode !== 'code';
  $('#prompt').placeholder = mode === 'code' ? t('ask_code') : t('ask');
  $('.sidebar').classList.remove('open');
  welcome();
  try { await refreshConversations(); await refreshSite(); if (mode === 'code') refreshVM(); }
  catch (error) { toast(error.message); }
  finally { $('#modeTransition').classList.remove('active'); }
}
$('#mode').onchange = () => changeMode($('#mode').value);
document.querySelectorAll('[data-mode]').forEach(button => button.onclick = () => changeMode(button.dataset.mode));
$('#guestContinue').onclick = async () => {
  $('#guestContinue').disabled = true;
  try { await api('/auth/guest', {method:'POST', body:{access_key:$('#accessKey').value.trim()}}); await enter(); }
  catch (error) { $('#authError').textContent=error.message; }
  finally { $('#guestContinue').disabled=false; }
};
async function loadMemory() {
  const data = await api('/memory');
  $('#memoryMeter').max = data.limit; $('#memoryMeter').value = data.storage_used ?? data.bytes;
  $('#memoryMeterLabel').textContent = formatBytes(data.storage_used ?? data.bytes) + ' / ' + formatBytes(data.limit) + ' ' + t('storage');
  $('#memoryList').replaceChildren();
  for (const item of data.items) {
    const row=node('div','memory-item'); row.append(node('p','',item.content));
    const remove=node('button','text-button','Sil'); remove.onclick=async()=>{try{await api('/memory/'+item.id,{method:'DELETE'});await loadMemory();await refreshAccount();}catch(error){toast(error.message);}};
    row.append(remove); $('#memoryList').append(row);
  }
  if (!data.items.length) $('#memoryList').append(node('p','muted','Hafızan henüz boş. İlk notunu ekleyebilirsin.'));
}
$('#memoryButton').onclick=async()=>{try{await loadMemory();$('#memoryDialog').showModal();}catch(error){toast(error.message);}};
$('#closeMemory').onclick=()=>$('#memoryDialog').close();
$('#memoryForm').onsubmit=async event=>{event.preventDefault();const button=event.target.querySelector('button');button.disabled=true;try{await api('/memory',{method:'POST',body:{content:$('#memoryContent').value}});$('#memoryContent').value='';await loadMemory();await refreshAccount();}catch(error){toast(error.message);}finally{button.disabled=false;}};
$('#clearMemory').onclick=async()=>{if(!confirm('Kaydettiğin tüm hafıza notları silinsin mi?'))return;try{await api('/memory/all',{method:'DELETE'});await loadMemory();await refreshAccount();}catch(error){toast(error.message);}};
$('#mobileMenu').onclick = () => $('.sidebar').classList.toggle('open');
$('#prompt').addEventListener('keydown', event => { if (event.key === 'Enter' && !event.shiftKey && !event.isComposing) { event.preventDefault(); if (!$('#send').disabled) $('#chatForm').requestSubmit(); } });
async function sendTurn(message, extras = {}) {
  if (job) return toast('Önce devam eden yanıtı durdur.');
  if (uploadBusy) return toast('Dosya yüklemesinin bitmesini bekle.');
  if (!message.trim()) return;
  const editing = Boolean(extras.edit_message_id);
  $('#send').disabled = true; $('#modelPicker').disabled = true;
  try {
    $('#hibernation').hidden = true; $('#commandHint').hidden = true;
    const started = await api('/chat', {method:'POST',body:{message, conversation_id:current, mode, model_id:selectedModel, effort:$('#effort').value, search_effort:$('#searchEffort').value, quality:$('#quality').value, multitask:$('#multitask').value, language:lang, attachment_ids:editing?[]:pendingAttachments.map(a=>a.id), ...extras}});
    current=started.conversation_id; job=started.job_id; $('#modelPicker').disabled=true;
    if(editing) $('#messages').replaceChildren();
    addMessage('user',message,[],{attachments:editing?[]:pendingAttachments});
    if(!editing){pendingAttachments=[];renderAttachmentDrafts();$('#prompt').value='';}
    if(mode==='code') $('#terminalOutput').textContent='Kübik Code görevi inceliyor…';
    $('#stopReply').hidden=false;$('#send').hidden=true;
    await refreshConversations();
    for (;;) {
      const progress=await api('/jobs/'+job);$('#jobStatus').textContent=progress.status;
      if(progress.state==='done') {
        const history=await api('/conversations/'+current);renderHistory(history,progress.result);
        if(mode==='code'&&$('#previewDialog').open) refreshProjectPreview().catch(error=>toast(error.message));
        break;
      }
      if(['error','canceled'].includes(progress.state)){toast(progress.status);if(current){const history=await api('/conversations/'+current);renderHistory(history);}break;}
      await delay(1200);
    }
  } catch(error){toast(error.message);}
  finally{job=null;$('#stopReply').hidden=true;$('#send').hidden=false;$('#jobStatus').textContent='';await refreshSite();if(account)refreshAccount().catch(()=>{});}
}
$('#chatForm').onsubmit=event=>{event.preventDefault();const message=$('#prompt').value.trim()||(pendingAttachments.length?'Eklediğim dosyayı inceleyip kısaca özetle.':'');sendTurn(message);};
$('#stopReply').onclick = async () => { if (job) { await api('/jobs/' + job + '/cancel', {method:'POST'}); $('#jobStatus').textContent = 'Durduruluyor'; } };
async function refreshVM() { if (!account || mode !== 'code') return; try { const vm = await api('/vm'); const labels = {stopped:'Ortam kapalı', starting:'Ortam hazırlanıyor · ilk açılış birkaç dakika sürebilir', running:`Çalışıyor · ${vm.memory_mb} MB RAM`, queued:`Sıradasın · ${vm.position}. sırada`}; $('#vmStatus').textContent = labels[vm.state] || vm.state; $('#vmError').textContent = vm.error || (vm.available === false ? 'VM bileşenleri eksik. Pi’deki güncelleme ve kurulum kayıtlarını yönetici kontrol etmeli.' : ''); $('#startVM').disabled = !['stopped'].includes(vm.state) || siteStatus.maintenance || vm.available === false; if (vm.error && !refreshVM.lastErrorShown) { toast(vm.error); refreshVM.lastErrorShown = true; } if (!vm.error) refreshVM.lastErrorShown = false; } catch(error) { $('#vmStatus').textContent = error.message; } }
$('#startVM').onclick = async () => { try { await api('/vm/start', {method:'POST'}); await refreshVM(); } catch(error) { toast(error.message); } };
$('#stopVM').onclick = async () => { try { await api('/vm/stop', {method:'POST'}); await refreshVM(); } catch(error) { toast(error.message); } };
$('#downloadForm')?.addEventListener('submit', async event => { event.preventDefault(); const path = $('#downloadPath').value.trim(); if (!path) return; const query = new URLSearchParams({workspace_id:current||'default',path}); const headers={'X-Kubik-Request':'1'};  try { const response=await kubikTransport.fetch('/api/vm/files?'+query,{headers}); if(!response.ok){let message='Dosya indirilemedi.';try{const value=await response.json();message=value.detail||message;}catch{} throw new Error(message);} const blob=await response.blob(), url=URL.createObjectURL(blob), link=node('a'); link.href=url; link.download=path.split('/').pop()||'kubik-file'; link.click(); URL.revokeObjectURL(url); toast('Dosya indirildi.'); } catch(error) { toast(error.message); } });
document.querySelectorAll('[data-prompt]').forEach(button => button.onclick = () => { $('#prompt').value = button.dataset.prompt; $('#prompt').focus(); });

setInterval(refreshSite, 10000); setInterval(refreshVM, 5000);

let pendingAttachments=[],uploadBusy=false,editorState=null;
function renderAttachmentDrafts(){
  $('#attachmentDrafts').replaceChildren();
  for(const file of pendingAttachments){const chip=node('span','attachment-chip',file.name+(file.truncated?' · kısaltıldı':'')),remove=node('button','','×');remove.type='button';remove.setAttribute('aria-label',file.name+' ekini kaldır');remove.onclick=async()=>{try{await api('/attachments/'+file.id,{method:'DELETE'});pendingAttachments=pendingAttachments.filter(a=>a.id!==file.id);renderAttachmentDrafts();}catch(error){toast(error.message);}};chip.append(remove);$('#attachmentDrafts').append(chip);}
}
$('#attachFile').onclick=()=>$('#fileInput').click();
$('#fileInput').onchange=async()=>{
  if(uploadBusy||job)return;
  uploadBusy=true;$('#attachFile').disabled=true;$('#attachmentStatus').textContent=[...$('#fileInput').files].some(f=>/\.(png|jpe?g|webp)$/i.test(f.name))?'Fotoğraf inceleniyor…':'Dosya okunuyor…';
  try{for(const file of [...$('#fileInput').files]){
    if(pendingAttachments.length>=3){toast('Bir mesajda en fazla 3 dosya ekleyebilirsin.');break;}
    if(file.size>5*1024*1024)throw new Error('Dosya en fazla 5 MB olabilir.');
    const response=await kubikTransport.fetch('/api/attachments?name='+encodeURIComponent(file.name),{method:'POST',headers:{'X-Kubik-Request':'1','Content-Type':'application/octet-stream'},body:file});
    const value=await response.json();if(!response.ok)throw new Error(value.detail||'Dosya yüklenemedi.');pendingAttachments.push(value);renderAttachmentDrafts();
  }}catch(error){toast(error.message);}finally{uploadBusy=false;$('#attachFile').disabled=false;$('#attachmentStatus').textContent='';$('#fileInput').value='';}
};
function decorateMessage(row,role,text,extra){
  if(extra.attachments?.length){const chips=node('div','message-files');for(const file of extra.attachments){const chip=node('span','attachment-chip',file.name);if(file.id){const remove=node('button','','×');remove.type='button';remove.setAttribute('aria-label',file.name+' dosyasını sohbetten kaldır');remove.onclick=async()=>{try{await api('/attachments/'+file.id,{method:'DELETE'});chip.remove();toast('Dosya sohbet bağlamından kaldırıldı.');}catch(error){toast(error.message);}};chip.append(remove);}chips.append(chip);}row.append(chips);}
  const id=extra.message_id;if(!id)return;
  const actions=node('div','message-actions');
  if(role==='user'){
    const edit=node('button','','Düzenle');edit.type='button';edit.onclick=()=>openMessageEditor('edit',id,text);actions.append(edit);
  }else{
    const retry=node('button','','Yeniden yanıtla');retry.type='button';retry.onclick=async()=>{if(job)return toast('Önce devam eden yanıtı durdur.');try{const data=await api('/conversations/'+current),index=data.messages.findIndex(m=>m.id===id),prior=data.messages.slice(0,index).reverse().find(m=>m.role==='user');if(!prior)throw new Error('Soru bulunamadı.');if(index<data.messages.length-1&&!confirm('Bu yanıttan sonraki mesajlar, yeni yanıt başarılı olunca kaldırılacak. Devam edilsin mi?'))return;await sendTurn(prior.content,{edit_message_id:prior.id});}catch(error){toast(error.message);}};actions.append(retry);
    if(account?.role!=='guest'){const teach=node('button','','Düzelt ve öğret');teach.type='button';teach.onclick=()=>openMessageEditor('teach',id,text);actions.append(teach);}
  }
  row.append(actions);
}
function renderHistory(data,lastResult=null){
  $('#messages').replaceChildren();
  if(!data.messages.length){welcome();return;}
  for(const message of data.messages)addMessage(message.role,message.content,message.sources,{message_id:message.id,attachments:message.attachments,model_id:message.model_id,...(lastResult&&message.id===lastResult.assistant_message_id?lastResult:{})});
}
function openMessageEditor(kind,id,text){
  if(job)return toast('Önce devam eden yanıtı durdur.');
  editorState={kind,id};$('#editorTitle').textContent=kind==='teach'?'Doğru yanıtı öğret':'Mesajını düzenle';
  $('#editorNote').textContent=kind==='teach'?'Soru ve düzelttiğin yanıt yöneticiye gönderilir. Yönetici onayladıktan sonra sonraki eğitimin havuzuna eklenir.':'Bu mesaj ve sonraki yanıtlar, yeni yanıt başarıyla tamamlandığında yenilenir.';
  $('#editorContent').maxLength=kind==='teach'?8000:4000;$('#editorContent').value=text;
  $('#editorSubmit').textContent=kind==='teach'?'Yönetici onayına gönder':'Düzenle ve yeniden yanıtla';$('#messageEditor').showModal();
}
$('#closeEditor').onclick=()=>$('#messageEditor').close();
$('#editorForm').onsubmit=async event=>{event.preventDefault();if(!editorState)return;const text=$('#editorContent').value.trim(),{kind,id}=editorState;$('#editorSubmit').disabled=true;try{if(kind==='teach'){await api('/messages/'+id+'/correction',{method:'POST',body:{answer:text}});toast('Düzeltmen yönetici onayına gönderildi.');$('#messageEditor').close();}else{$('#messageEditor').close();await sendTurn(text,{edit_message_id:id});}}catch(error){toast(error.message);}finally{$('#editorSubmit').disabled=false;}};
async function refreshProjectPreview(){
  if(!current||mode!=='code')throw new Error('Önce bir Code projesi oluştur.');
  $('#previewStatus').textContent='Proje hazırlanıyor…';
  const result=await api('/vm/preview',{method:'POST',body:{workspace_id:current,entry:$('#previewEntry').value.trim()||'index.html'}});
  $('#previewFrame').src=kubikTransport.url(result.path);$('#previewStatus').textContent='Proje değiştiğinde veya önizleme süresi dolduğunda Yenile’ye bas.';
}
$('#openPreview').onclick=async()=>{const button=$('#openPreview');button.disabled=true;try{await refreshProjectPreview();$('#previewDialog').showModal();}catch(error){toast(error.message);}finally{button.disabled=false;}};
$('#refreshPreview').onclick=async()=>{try{await refreshProjectPreview();}catch(error){$('#previewStatus').textContent=error.message;}};
$('#closePreview').onclick=()=>{$('#previewDialog').close();$('#previewFrame').removeAttribute('src');};
$('#downloadProject').onclick=async()=>{if(!current)return toast('Önce bir Code projesi oluştur.');const button=$('#downloadProject');button.disabled=true;try{const response=await kubikTransport.fetch('/api/vm/project.zip?'+new URLSearchParams({workspace_id:current}));if(!response.ok){const error=await response.json();throw new Error(error.detail||'Proje indirilemedi.');}const url=URL.createObjectURL(await response.blob()),a=node('a');a.href=url;a.download='kubik-project.zip';a.click();setTimeout(()=>URL.revokeObjectURL(url),30000);}catch(error){toast(error.message);}finally{button.disabled=false;}};
// ---------------- Kübik 2.0: ayarlar, planlar, abonelik istekleri, slash komutları
$('#quality').value = store.get('kubik-quality', 'balanced'); $('#multitask').value = store.get('kubik-multitask', 'auto'); $('#language').value = lang;
$('#quality').onchange = () => store.set('kubik-quality', $('#quality').value);
$('#multitask').onchange = () => store.set('kubik-multitask', $('#multitask').value);
$('#language').onchange = () => { lang = $('#language').value; store.set('kubik-lang', lang); applyLanguage(); if (!current) welcome(); refreshSite(); if (account) refreshAccount().catch(() => {}); };
function commandList() { return ['kubik2', ...(siteStatus.models || []).filter(m => m.installed).map(m => m.command || m.id)]; }
$('#openSettings').onclick = () => { $('#commandsHelp').textContent = t('commands_help') + ' ' + commandList().map(c => '/' + c + '/').join('  '); $('#settingsDialog').showModal(); };
$('#closeSettings').onclick = () => $('#settingsDialog').close();
$('#prompt').addEventListener('input', () => {
  const value = $('#prompt').value, hint = $('#commandHint');
  if (!/^\/[\w.-]*$/.test(value)) { hint.hidden = true; return; }
  const typed = value.slice(1).toLowerCase(), matches = commandList().filter(c => c.startsWith(typed));
  hint.replaceChildren(node('span', '', t('commands') + ':'), ...matches.map(c => { const b = node('button', '', '/' + c + '/'); b.type = 'button'; b.onclick = () => { $('#prompt').value = '/' + c + '/ '; hint.hidden = true; $('#prompt').focus(); }; return b; }));
  hint.hidden = !matches.length;
});
async function renderPlans() {
  const tiers = siteStatus.tiers || {}, cards = $('#planCards'); cards.replaceChildren();
  for (const [id, plan] of Object.entries(tiers)) {
    const card = node('div', 'plan-card' + (account?.tier === id ? ' current' : '')), list = node('ul');
    card.append(node('h3', '', plan.name));
    if (plan.ultrapack) card.append(node('span', 'tag', 'ULTRAPACK'));
    for (const line of [`${formatBytes(plan.storage)} ${t('storage')}`, `${plan.tokens.toLocaleString(lang)} ${t('tokens')} ${t('per_day')}`,
      plan.messages ? `${plan.messages} ${t('messages_l')} ${t('per_day')}` : t('unlimited'), `${plan.photos} ${t('photos')} ${t('per_day')}`,
      t(plan.wait ? 'wait_yes' : 'wait_no'), t(plan.code ? 'code_yes' : 'code_no')]) list.append(node('li', '', line));
    card.append(list);
    const button = node('button', 'secondary');
    if (account?.tier === id) { button.textContent = t('current_plan'); button.disabled = true; }
    else if (account?.role === 'guest') { button.textContent = t('need_account'); button.disabled = true; }
    else if (account?.pending_tier === id) { button.textContent = t('requested'); button.onclick = async () => { try { await api('/subscription/request', {method:'DELETE'}); await refreshAccount(); renderPlans(); } catch (error) { toast(error.message); } }; }
    else { button.textContent = t('request'); button.className = 'primary'; button.onclick = async () => { button.disabled = true; try { await api('/subscription/request', {method:'POST', body:{tier:id}}); toast(t('request_sent')); await refreshAccount(); renderPlans(); } catch (error) { toast(error.message); button.disabled = false; } }; }
    card.append(button); cards.append(card);
  }
}
// Pencere önce açılır; sunucu cevap vermezse içinde hata yazar (eskiden sessizce açılmıyordu).
$('#planButton').onclick = async () => {
  if (!$('#plansDialog').open) $('#plansDialog').showModal();
  if (siteStatus.tiers) await renderPlans(); else $('#planCards').replaceChildren(node('p', 'muted', '…'));
  try { if (!siteStatus.tiers) await refreshSite(); await refreshAccount(); await renderPlans();
    if (!siteStatus.tiers) $('#planCards').replaceChildren(node('p', 'muted', t('unreachable'))); }
  catch (error) { $('#planCards').replaceChildren(node('p', 'muted', error.message)); }
};
$('#closePlans').onclick = () => $('#plansDialog').close();
async function renderRequests() {
  const data = await api('/subscription/requests'), tiers = siteStatus.tiers || {};
  $('#requestList').replaceChildren(...(data.pending.length ? data.pending.map(item => {
    const row = node('div', 'request-row'), buttons = node('div', 'button-row');
    row.append(node('span', '', `${item.name}: ${tiers[item.current]?.name || item.current} → ${tiers[item.tier]?.name || item.tier}`));
    for (const [decision, label, cls] of [['accept', t('accept'), 'primary'], ['reject', t('reject'), 'secondary']]) {
      const b = node('button', cls, label); b.onclick = async () => { b.disabled = true; try { await api('/subscription/requests/' + item.id, {method:'POST', body:{decision}}); await renderRequests(); refreshAccount().catch(() => {}); } catch (error) { toast(error.message); b.disabled = false; } }; buttons.append(b);
    }
    row.append(buttons); return row;
  }) : [node('p', 'muted', t('no_requests'))]));
  $('#memberList').replaceChildren(...(data.members.length ? data.members.map(m => node('div', 'request-row', `${m.name} · ${tiers[m.tier]?.name || m.tier}`)) : [node('p', 'muted', t('no_members'))]));
}
$('#requestsButton').onclick = async () => { try { if (!siteStatus.tiers) await refreshSite(); await renderRequests(); $('#requestsDialog').showModal(); } catch (error) { toast(error.message); } };
$('#closeRequests').onclick = () => $('#requestsDialog').close();
// KVKK: gizlilik metni ve hesabı kalıcı silme
document.querySelectorAll('.privacy-link').forEach(link => link.onclick = () => { $('#deleteAccountForm').hidden = !account || account.role === 'admin' || account.owner; $('#deletePassword').hidden = account?.role === 'guest'; $('#privacyDialog').showModal(); });
$('#closePrivacy').onclick = () => $('#privacyDialog').close();
$('#deleteAccountForm').onsubmit = async event => { event.preventDefault(); if (!confirm(t('delete_confirm'))) return; try { await api('/account/delete', {method:'POST', body:{password:$('#deletePassword').value}}); kubikTransport.clearToken(); location.reload(); } catch (error) { toast(error.message); } };
applyLanguage();
refreshSite();enter().catch(()=>{account=null;});

'use strict';
const $ = s => document.querySelector(s);
let account = null, registerMode = false, current = null, job = null, mode = 'chat', siteStatus = {};
let selectedModel = null;
const store = { get(k, d) { try { return localStorage.getItem(k) ?? d; } catch { return d; } }, set(k, v) { try { localStorage.setItem(k, v); } catch {} } };
let lang = store.get('kubik-lang', 'tr');
const T = {
  tr: {new_chat:'Yeni sohbet', your_chats:'SOHBETLERİN', search_chats:'Sohbetlerde ara', my_plan:'Planım', plan_requests:'Küp yönetimi', memory:'Hafızam',
    attach:'+ Dosya / fotoğraf', thinking:'Düşünme', fast:'Hızlı', balanced:'Dengeli', deep:'Derin', ultra_think:'Ultra Think', web_search:'Web araması',
    web_off:'Web kapalı', web_quick:'Web · Hızlı', web_balanced:'Web · Dengeli', web_deep:'Web · Derin', deepsearch:'Deep Search', supersearch:'Super Search',
    settings:'Ayarlar', stop:'Durdur', footnote:'Kübik hata yapabilir. Önemli bilgileri kaynaklarıyla kontrol et.', built_on:'Kübik 2.0 şu açık modellerle çalışır:', privacy_title:'Gizlilik, çerezler ve koşullar', privacy_link:'Gizlilik ve koşullar', delete_title:'Hesabımı ve verilerimi sil', delete_help:'Sohbetlerin, hafızan, dosyaların ve Code projelerin kalıcı olarak silinir. Geri alınamaz.', password_ph:'Parolan', delete_button:'Kalıcı olarak sil', delete_confirm:'Hesabın ve tüm verilerin kalıcı olarak silinecek. Emin misin?', quality:'Verimlilik',
    quality_help:'Hız ile doğruluk arasındaki denge. Ultra Reliable her soruyu birkaç kez çözüp en tutarlı cevabı seçer.', q_efficient:'Verimli (en hızlı)',
    q_reliable:'Güvenilir (kontrol turu)', q_ultrareliable:'Ultra Reliable', multitask:'Yapay zeka multitask',
    multitask_help:'Kübik 2.0 bir soruyu kaç modele sorsun? Cevaplar karşılaştırılır, en iyisi seçilir. Çok model = daha yavaş.', m_auto:'Otomatik',
    m_cok_az:'Çok az (1 YZ)', m_az:'Az (2 YZ)', m_orta:'Orta (3 YZ)', m_fazla:'Fazla (4 YZ)', m_cok_fazla:'Çok fazla (5 YZ)', m_ultraai:'UltraAI (6 YZ)', m_hepsi:'Hepsi',
    language:'Dil', language_help:'Arayüz ve cevap dili.', plans:'Planlar', plans_help:'Planlar parayla değil Küp ile alınır ve 30 gün sürer. Günlük haklar her gece yarısı yenilenir.', kup:'Küp', buy:'Al', need_kup:'Küp yetmiyor', days_left:'gün kaldı', earn_title:'Küp nasıl kazanılır?', earn_welcome:'Hesap açınca', earn_daily:'Her gün ilk mesajında', earn_message:'Her mesaj (günde en fazla 20)', earn_teach:'Onaylanan her “Düzelt ve öğret”', earn_feedback:'Yanıtlara 👍/👎 (günde en fazla 4)', earn_buy:'1000 Küp ve üstü her alışverişte geri', earn_invite:'Davet ettiğin arkadaş kayıt olunca (ikinize de Max+)', earn_note:'Küp parayla satın alınamaz, devredilemez ve nakde çevrilemez.', bought:'Plan alındı!', buy_confirm:'Küp ile 30 günlük alınsın mı? Başka bir planın varsa onun yerine geçer.', kup_admin:'Küp yönetimi', grant:'Küp ver', grant_user:'Kullanıcı adı', grant_amount:'Miktar', pack_locked:'Paket gerekir', files:'Proje dosyaları', session:'5 saatlik oturum', weekly:'Haftalık', resets_in:'sonra yenilenir', resets_at:'yenilenir', refill:'Token yenile', refill_help:'5 saatlik limitini hemen sıfırlar (haftalık limit aynı kalır).', refilled:'Oturum yenilendi!', work_share:'Bu linki paylaş; yazanlara yapay zekâ senin talimatlarınla cevap verir.', copy:'Kopyala', copied:'Kopyalandı', work_auto:'Yapay zekâ cevaplasın', work_write:'Mesaj yaz…', work_settings:'Work ayarları', work_title:'Görünen ad', save:'Kaydet', work_requests:'Work istekleri', work_buy:'Kübik Work', work_help:'WhatsApp gibi: sana yazanlara yapay zekâ cevap verir, sen istediğin an araya girersin.', work_request:'Work isteği gönder', work_requested:'İsteğin gönderildi', work_open:'Work açık', work_need_max:'Max ve üstü planlarda istenebilir', work_empty:'Henüz kimse yazmadı. Linkini paylaş!', work_pick:'Soldan bir sohbet seç.', you:'Sen', no_requests_w:'Bekleyen Work isteği yok.', refresh:'Yenile', download:'İndir', no_files:'Henüz dosya yok. Kübik dosya oluşturunca burada görünür.',
    members:'Plan sahipleri', hibernating_banner:'Sunucu uyku modunda. Sohbete devam etmek için sohbet penceresine bir şey yaz.', hibernating:'uyku modunda',
    ready:'hazır', selected:'seçildi', not_installed:'kurulu değil', unreachable:'Sunucuya ulaşılamıyor', ask:'Kübik’e bir şey sor…', ask_code:'Ne geliştirmek istiyorsun?',
    commands:'Komutlar', commands_help:'Bir modeli tek başına kullanmak için mesajın başına komut yaz:', unlimited:'Sınırsız (token bitene kadar)', per_day:'/ gün',
    storage:'depolama', tokens:'token', messages_l:'mesaj', photos:'fotoğraf', wait_yes:'Yoğunlukta sıra beklenir', wait_no:'Bekleme yok', code_yes:'Kübik Code var', code_no:'Code yok',
    current_plan:'Mevcut planın', request:'İstek gönder', requested:'İstek gönderildi · geri çek', need_account:'Plan için hesap aç', accept:'Kabul', reject:'Reddet',
    no_requests:'Bekleyen istek yok.', no_members:'Henüz plan sahibi yok.', request_sent:'İsteğin gönderildi. Onaylanınca planın değişir.', ultrapack_locked:'Ultrapack',
    consulted:'model karşılaştırıldı', chose:'seçilen', welcome_eyebrow:'BİRLİKTE BİR ADIM İLERİ', welcome_title:'Aklında ne var?', welcome_sub:'Bir soruyla başla. Bir fikri geliştir. Yeni bir şey keşfet.',
    code_eyebrow:'FİKRİNDEN İLK SATIRA', code_title:'Birlikte ne üretelim?', code_sub:'Bir fikir anlat. Kübik planlasın, kodlasın ve denesin.', today:'Bugün'},
  en: {new_chat:'New chat', your_chats:'YOUR CHATS', search_chats:'Search chats', my_plan:'My plan', plan_requests:'Küp management', memory:'Memory',
    attach:'+ File / photo', thinking:'Thinking', fast:'Fast', balanced:'Balanced', deep:'Deep', ultra_think:'Ultra Think', web_search:'Web search',
    web_off:'Web off', web_quick:'Web · Fast', web_balanced:'Web · Balanced', web_deep:'Web · Deep', deepsearch:'Deep Search', supersearch:'Super Search',
    settings:'Settings', stop:'Stop', footnote:'Kübik can make mistakes. Check important information with its sources.', built_on:'Kübik 2.0 runs on these open models:', privacy_title:'Privacy, cookies and terms', privacy_link:'Privacy and terms', delete_title:'Delete my account and data', delete_help:'Your chats, memory, files and Code projects are permanently deleted. This cannot be undone.', password_ph:'Your password', delete_button:'Delete permanently', delete_confirm:'Your account and all your data will be permanently deleted. Are you sure?', quality:'Efficiency',
    quality_help:'Balance between speed and accuracy. Ultra Reliable solves each question several times and picks the most consistent answer.', q_efficient:'Efficient (fastest)',
    q_reliable:'Reliable (review pass)', q_ultrareliable:'Ultra Reliable', multitask:'AI multitask',
    multitask_help:'How many models should Kübik 2.0 ask? Answers are compared and the best one wins. More models = slower.', m_auto:'Automatic',
    m_cok_az:'Very few (1 AI)', m_az:'Few (2 AI)', m_orta:'Medium (3 AI)', m_fazla:'Many (4 AI)', m_cok_fazla:'Very many (5 AI)', m_ultraai:'UltraAI (6 AI)', m_hepsi:'All',
    language:'Language', language_help:'Interface and reply language.', plans:'Plans', plans_help:'Plans are bought with Küp, not money, and last 30 days. Daily limits reset every midnight.', kup:'Küp', buy:'Buy', need_kup:'Not enough Küp', days_left:'days left', earn_title:'How to earn Küp', earn_welcome:'When you sign up', earn_daily:'Your first message each day', earn_message:'Each message (max 20 a day)', earn_teach:'Each approved “Correct and teach”', earn_feedback:'Rating a reply 👍/👎 (max 4 a day)', earn_buy:'Back on every purchase of 1000 Küp or more', earn_invite:'When a friend you invite signs up (Max+ for both)', earn_note:'Küp cannot be bought with money, transferred or cashed out.', bought:'Plan bought!', buy_confirm:'Buy for 30 days with Küp? It replaces any other plan you have.', kup_admin:'Küp management', grant:'Give Küp', grant_user:'Username', grant_amount:'Amount', pack_locked:'Pack required', files:'Project files', session:'5-hour session', weekly:'Weekly', resets_in:'until reset', resets_at:'resets', refill:'Refill tokens', refill_help:'Resets your 5-hour limit right away (weekly limit stays).', refilled:'Session refilled!', work_share:'Share this link; the AI answers with your instructions.', copy:'Copy', copied:'Copied', work_auto:'AI replies', work_write:'Write a message…', work_settings:'Work settings', work_title:'Display name', save:'Save', work_requests:'Work requests', work_buy:'Kübik Work', work_help:'Like WhatsApp: the AI answers people who message you, and you can step in any time.', work_request:'Request Work', work_requested:'Request sent', work_open:'Work is on', work_need_max:'Available to request on Max and above', work_empty:'Nobody has written yet. Share your link!', work_pick:'Pick a chat on the left.', you:'You', no_requests_w:'No pending Work requests.', refresh:'Refresh', download:'Download', no_files:'No files yet. Files Kübik creates appear here.',
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
const PACK_FIELDS = {effort:'effort', searchEffort:'search_effort', quality:'quality', multitask:'multitask'};
function applyLanguage() {
  document.documentElement.lang = lang;
  document.querySelectorAll('[data-i18n]').forEach(element => { element.dataset.base = t(element.dataset.i18n); element.textContent = element.dataset.base; });
  document.querySelectorAll('[data-i18n-ph]').forEach(element => { element.placeholder = t(element.dataset.i18nPh); });
  document.querySelectorAll('[data-lang]').forEach(element => { element.hidden = element.dataset.lang !== lang; });
  $('#prompt').placeholder = mode === 'code' ? t('ask_code') : t('ask');
  lockUltrapack();
}
function lockUltrapack() {
  // Planın paketi hangi ayar seviyelerini açıyorsa onlar seçilebilir; üstündekiler kilitli, hangi paketin açtığı yazar.
  const packs = siteStatus.packs, orders = siteStatus.orders;
  if (!packs || !orders) return;
  const mine = packs[account?.plan?.pack || 'free'];
  for (const [id, field] of Object.entries(PACK_FIELDS)) {
    const select = $('#' + id), order = orders[field]; if (!select || !order) continue;
    for (const option of select.options) {
      const level = order.indexOf(option.value); if (level < 0) continue;
      const locked = level > order.indexOf(mine[field]);
      const opener = locked ? Object.values(packs).find(p => order.indexOf(p[field]) >= level)?.name : '';
      option.disabled = locked || option.dataset.serverOff === '1';
      option.textContent = (option.dataset.base || option.textContent.split(' · 🔒')[0]) + (locked ? ' · 🔒 ' + opener : '');
    }
    if (select.selectedOptions[0]?.disabled) select.value = {effort:'balanced', quality:'balanced', searchEffort:'off', multitask:'auto'}[id];
    store.set('kubik-' + id, select.value);
  }
}
const modelName = id => id === 'kubik-2.0' || id === 'auto' ? (siteStatus.ensemble?.name || 'Kübik 2.0') : (siteStatus.models?.find(model => model.id === id)?.name || (id === 'apex-5.5' ? 'Apex 5.5' : 'Kübik 1.0'));
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
function choiceBox({question, options}) {
  const box = node('div', 'choice-box'), list = node('div', 'choice-options'), other = node('form', 'choice-other'), input = node('input');
  if (question) box.append(node('strong', '', question));
  const pick = value => { if (!value || job) return; box.classList.add('answered'); box.querySelectorAll('button,input').forEach(e => e.disabled = true);
    $('#prompt').value = value; $('#chatForm').requestSubmit(); };
  options.forEach((option, index) => { const b = node('button', 'choice', `${index + 1}. ${option}`); b.type = 'button'; b.onclick = () => { b.classList.add('picked'); pick(option); }; list.append(b); });
  input.placeholder = lang === 'en' ? 'Other…' : 'Diğer…'; input.maxLength = 500; other.append(input, node('button', 'secondary', '→'));
  other.onsubmit = event => { event.preventDefault(); pick(input.value.trim()); };
  box.append(list, other); return box;
}
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
    // <secenekler soru="...">A | B | C</secenekler>: tıklanabilir seçenek kutusu (Claude'daki gibi)
    text = text.replace(/<pc>([\s\S]*?)<\/pc>/gi, (_, body) => { try { const task = JSON.parse(body); return `\n[PC: ${task.islem} ${task.yol || task.komut || ''}]\n`; } catch { return ''; } });
    const choices = []; text = text.replace(/<secenekler(?:\s+soru="([^"]*)")?\s*>([\s\S]*?)<\/secenekler>/gi, (_, question, body) => {
      choices.push({question: question || '', options: body.split(/\||\n/).map(o => o.trim()).filter(Boolean).slice(0, 6)}); return ''; }).trim();
    queueMicrotask(() => choices.forEach(choice => row.append(choiceBox(choice))));
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
  $('#planName').textContent = account.plan?.name + (guest ? '' : ' · ' + (account.kup ?? 0) + ' Küp');
  $('#requestsButton').hidden = !account.owner;
  document.querySelector('[data-mode=work]').hidden = guest || !(account.work || (siteStatus.work_min_tiers || []).includes(account.tier));
  renderUsage();
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
    const option = node('option', '', (sleeping ? '❄ ' : '') + modelName('kubik-2.0') + (sleeping ? ' · ' + t('hibernating') : ''));
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
    [...$('#searchEffort').options].forEach(option => { option.dataset.serverOff = option.value !== 'off' && (!siteStatus.search_enabled || ranks.indexOf(option.value) > maxRank) ? '1' : ''; });
    // Lisans gereği: hangi açık modellerin çalıştığı ve Llama için "Built with Llama" görünür olmalı
    const used = (siteStatus.models || []).filter(m => m.installed && m.extra).map(m => m.id.startsWith('x-kubik3') ? 'Kübik 3.0 (Qwen3-30B-A3B YOYO-V4 tabanlı)' : m.name);
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
async function enter() { await refreshAccount(); $('#auth').hidden = true; $('#workspace').hidden = false; applyLanguage(); welcome(); await refreshConversations(); await refreshSite(); dispatchEvent(new Event('kubik:enter')); }
$('#authToggle').onclick = () => { registerMode = !registerMode; $('#authTitle').textContent = registerMode ? 'Kübik’e hoş geldin.' : 'Tekrar hoş geldin.'; $('#authSubmit').textContent = registerMode ? 'Hesap oluştur' : 'Giriş yap'; $('#authToggle').textContent = registerMode ? 'Zaten hesabın var mı? Giriş yap' : 'Hesabın yok mu? Kayıt ol'; $('#password').autocomplete = registerMode ? 'new-password' : 'current-password'; $('#authError').textContent = ''; $('#termsRow').hidden = !registerMode; };
$('#authForm').onsubmit = async event => { event.preventDefault(); $('#authError').textContent = ''; if (registerMode && !$('#termsAccept').checked) { $('#authError').textContent = 'Kayıt olmak için kullanım koşullarını ve KVKK metnini onaylaman gerekiyor.'; return; } $('#authSubmit').disabled = true; try { await api('/auth/' + (registerMode ? 'register' : 'login'), {method:'POST', body:{username:$('#username').value, password:$('#password').value, access_key:$('#accessKey').value.trim(), invite:registerMode && typeof inviteCode === 'function' ? inviteCode() : ''}}); $('#password').value = ''; await enter(); } catch(error) { $('#authError').textContent = error.message; } finally { $('#authSubmit').disabled = false; } };
$('#logout').onclick = async () => { await api('/auth/logout', {method:'POST'}); kubikTransport.clearToken(); location.reload(); };
$('#newChat').onclick = () => { if (job) return toast('Önce devam eden yanıtı durdur.'); current = null; updateWorkspaceLabel(); welcome(); refreshConversations(); $('#prompt').focus(); };
// Code'a geçiş: hiper uzay. Yıldızlar ortadan kenarlara hızlanarak çizgi olur.
function warp(veil, duration) {
  const canvas = node('canvas'), ctx = canvas.getContext('2d'), w = innerWidth, h = innerHeight, cx = w / 2, cy = h / 2;
  canvas.width = w; canvas.height = h; veil.append(canvas);
  const stars = Array.from({length: 320}, () => ({a: Math.random() * Math.PI * 2, d: Math.random() * 40 + 2, s: Math.random() * .6 + .4}));
  const start = performance.now(); let done = false;
  return new Promise(resolve => {
    function frame(now) {
      const p = Math.min(1, (now - start) / duration), speed = 2 + p * p * 70;
      ctx.fillStyle = `rgba(7,8,14,${.35 + p * .4})`; ctx.fillRect(0, 0, w, h);
      for (const star of stars) {
        const from = star.d; star.d += speed * star.s * (star.d / 60 + .2);
        if (star.d > Math.hypot(w, h)) star.d = Math.random() * 20 + 2;
        const cos = Math.cos(star.a), sin = Math.sin(star.a);
        ctx.strokeStyle = `rgba(${190 + 60 * p | 0},${205 + 40 * p | 0},255,${.4 + p * .6})`; ctx.lineWidth = .6 + p * 1.6;
        ctx.beginPath(); ctx.moveTo(cx + cos * from, cy + sin * from); ctx.lineTo(cx + cos * star.d, cy + sin * star.d); ctx.stroke();
      }
      if (p > .82) { ctx.fillStyle = `rgba(200,215,255,${(p - .82) * 3})`; ctx.fillRect(0, 0, w, h); }
      // süre dolunca geçiş devam eder ama yıldızlar perde kapanana kadar akmaya devam eder (son karede donmasın)
      if (p >= 1 && !done) { done = true; setTimeout(() => canvas.remove(), 600); resolve(); }
      if (canvas.isConnected) requestAnimationFrame(frame);
    }
    requestAnimationFrame(frame);
  });
}
async function changeMode(next) {
  if (job) return toast('Önce devam eden yanıtı durdur.');
  if (next === 'code' && account?.role === 'guest') return toast('Code için hesap oluşturman gerekiyor.');
  if (next === mode) return;
  const veil = $('#modeTransition'), still = matchMedia('(prefers-reduced-motion: reduce)').matches;
  const origin = document.querySelector(`[data-mode=${next}]`)?.getBoundingClientRect();
  veil.style.setProperty('--x', origin ? origin.x + origin.width / 2 + 'px' : '50%');
  veil.style.setProperty('--y', origin ? origin.y + origin.height / 2 + 'px' : '50%');
  veil.className = 'mode-transition to-' + next;
  void veil.offsetWidth; veil.classList.add('active');
  await (still ? delay(0) : next === 'code' ? warp(veil, 760) : delay(420));
  mode = next; $('#mode').value = mode; current = null;
  document.body.classList.toggle('code-theme', mode === 'code');
  document.querySelectorAll('[data-mode]').forEach(button => { button.classList.toggle('selected', button.dataset.mode === mode); button.setAttribute('aria-pressed', String(button.dataset.mode === mode)); });
  $('#modeTitle').textContent = {code:'Kübik Code', work:'Kübik Work'}[mode] || 'Kübik Chat';
  $('#codePanel').hidden = mode !== 'code';
  $('#workPanel').hidden = mode !== 'work'; document.querySelector('.chat-panel').hidden = mode === 'work';
  if (mode === 'work') openWork(); else stopWork();
  $('#prompt').placeholder = mode === 'code' ? t('ask_code') : t('ask');
  $('.sidebar').classList.remove('open');
  welcome();
  // Geçiş sunucuyu beklemez: perde hemen açılır, liste ve durum arkadan gelir (Pi yavaşken animasyon donmasın)
  veil.classList.add('leaving');
  (async () => {
    try { if (mode !== 'work') await refreshConversations(); await refreshSite(); if (mode === 'code') { refreshVM(); refreshFiles(); } }
    catch (error) { toast(error.message); }
  })();
  await delay(still ? 0 : 380); veil.className = 'mode-transition';
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
    const started = await api('/chat', {method:'POST',body:{message, conversation_id:current, mode, model_id:selectedModel, effort:$('#effort').value, search_effort:$('#searchEffort').value, quality:$('#quality').value, multitask:$('#multitask').value, language:lang, attachment_ids:editing?[]:pendingAttachments.map(a=>a.id), pc_mode:pc.mode&&pc.mode!=='kapali'?pc.mode:'', ...extras}});
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
        pc.pending = pc.mode && pc.mode !== 'kapali' ? pcTasks(progress.result?.text || '') : [];
        if(mode==='code'&&$('#previewDialog').open) refreshProjectPreview().catch(error=>toast(error.message));
        break;
      }
      if(['error','canceled'].includes(progress.state)){toast(progress.status);if(current){const history=await api('/conversations/'+current);renderHistory(history);}break;}
      await delay(1200);
    }
  } catch(error){toast(error.message);}
  finally{job=null;$('#stopReply').hidden=true;$('#send').hidden=false;$('#jobStatus').textContent='';await refreshSite();if(account)refreshAccount().catch(()=>{});if(mode==='code')refreshFiles();}
  if (pc.pending?.length) { const tasks = pc.pending; pc.pending = []; await runPcTasks(tasks); }
}
$('#chatForm').onsubmit=event=>{event.preventDefault();pc.steps=0;const message=$('#prompt').value.trim()||(pendingAttachments.length?'Eklediğim dosyayı inceleyip kısaca özetle.':'');sendTurn(message);};
$('#stopReply').onclick = async () => { if (job) { await api('/jobs/' + job + '/cancel', {method:'POST'}); $('#jobStatus').textContent = 'Durduruluyor'; } };
async function refreshVM() { if (!account || mode !== 'code') return; try { const vm = await api('/vm'); const labels = {stopped:'Ortam kapalı', starting:'Ortam hazırlanıyor · ilk açılış birkaç dakika sürebilir', running:`Çalışıyor · ${vm.memory_mb} MB RAM`, queued:`Sıradasın · ${vm.position}. sırada`}; $('#vmStatus').textContent = labels[vm.state] || vm.state; $('#vmError').textContent = vm.error || (vm.available === false ? 'VM bileşenleri eksik. Pi’deki güncelleme ve kurulum kayıtlarını yönetici kontrol etmeli.' : ''); $('#startVM').disabled = !['stopped'].includes(vm.state) || siteStatus.maintenance || vm.available === false; if (vm.error && !refreshVM.lastErrorShown) { toast(vm.error); refreshVM.lastErrorShown = true; } if (!vm.error) refreshVM.lastErrorShown = false; } catch(error) { $('#vmStatus').textContent = error.message; } }
$('#startVM').onclick = async () => { try { await api('/vm/start', {method:'POST'}); await refreshVM(); } catch(error) { toast(error.message); } };
$('#stopVM').onclick = async () => { try { await api('/vm/stop', {method:'POST'}); await refreshVM(); } catch(error) { toast(error.message); } };
async function downloadFile(path) {
  const query = new URLSearchParams({workspace_id:current||'default', path});
  const response = await kubikTransport.fetch('/api/vm/files?' + query, {headers:{'X-Kubik-Request':'1'}});
  if (!response.ok) { let message = 'Dosya indirilemedi.'; try { message = (await response.json()).detail || message; } catch {} throw new Error(message); }
  const url = URL.createObjectURL(await response.blob()), link = node('a'); link.href = url; link.download = path.split('/').pop() || 'kubik-file'; link.click();
  setTimeout(() => URL.revokeObjectURL(url), 30000);
}
$('#downloadForm')?.addEventListener('submit', async event => { event.preventDefault(); const path = $('#downloadPath').value.trim(); if (!path) return; try { await downloadFile(path); toast('Dosya indirildi.'); } catch (error) { toast(error.message); } });
async function refreshFiles() {
  const box = $('#projectFiles'); if (!box || mode !== 'code') return;
  if (!current) { box.replaceChildren(node('p', 'fine', t('no_files'))); return; }
  try {
    const {files} = await api('/vm/list?' + new URLSearchParams({workspace_id:current}));
    box.replaceChildren(...(files.length ? files.map(file => {
      const row = node('div', 'file-row'), button = node('button', 'file-download', '⬇ ' + t('download'));
      button.type = 'button'; button.title = file.path;
      button.onclick = async () => { button.disabled = true; try { await downloadFile(file.path); } catch (error) { toast(error.message); } finally { button.disabled = false; } };
      row.append(node('span', 'file-name', file.path), node('small', '', formatBytes(file.bytes)), button); return row;
    }) : [node('p', 'fine', t('no_files'))]));
  } catch (error) { box.replaceChildren(node('p', 'fine', error.message)); }
}
$('#refreshFiles').onclick = refreshFiles;
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
    if(account?.role!=='guest'){for(const [value,label] of [[1,'👍'],[-1,'👎']]){const b=node('button','feedback',label);b.type='button';b.title=value>0?'İyi yanıt':'Kötü yanıt';b.onclick=async()=>{try{const r=await api('/messages/'+id+'/feedback',{method:'POST',body:{value}});actions.querySelectorAll('.feedback').forEach(x=>x.classList.toggle('picked',x===b));if(r.kup)toast(`Teşekkürler! +${r.kup} Küp`);}catch(error){toast(error.message);}};actions.append(b);}}
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
  const tiers = siteStatus.tiers || {}, packs = siteStatus.packs || {}, cards = $('#planCards'); cards.replaceChildren();
  const guest = account?.role === 'guest', balance = account?.kup ?? 0;
  $('#kupBalance').textContent = guest ? '' : `${balance.toLocaleString(lang)} ${t('kup')}`;
  for (const [id, plan] of Object.entries(tiers)) {
    const card = node('div', 'plan-card' + (account?.tier === id ? ' current' : '') + (id === 'hyper' ? ' hyper' : '')), list = node('ul');
    card.append(node('h3', '', plan.name));
    if (plan.pack !== 'free') card.append(node('span', 'tag', (packs[plan.pack]?.name || '').toUpperCase()));
    card.append(node('div', 'price', plan.price ? `${plan.price.toLocaleString(lang)} ${t('kup')} / 30 gün` : '—'));
    for (const line of [`${formatBytes(plan.storage)} ${t('storage')}`, `${plan.tokens.toLocaleString(lang)} ${t('tokens')} ${t('per_day')}`,
      plan.messages ? `${plan.messages} ${t('messages_l')} ${t('per_day')}` : t('unlimited'), `${plan.photos} ${t('photos')} ${t('per_day')}`,
      t(plan.wait ? 'wait_yes' : 'wait_no'), t(plan.code ? 'code_yes' : 'code_no')]) list.append(node('li', '', line));
    card.append(list);
    const button = node('button', 'secondary');
    if (account?.tier === id) {
      const left = account.tier_until ? Math.max(0, Math.ceil((account.tier_until * 1000 - Date.now()) / 864e5)) : null;
      button.textContent = t('current_plan') + (left !== null ? ` · ${left} ${t('days_left')}` : ''); button.disabled = !plan.price || left === null;
      if (!button.disabled) { button.textContent += ' · +30'; button.onclick = () => buyPlan(id, plan, button); }
    } else if (!plan.price) { button.textContent = '—'; button.disabled = true; }
    else if (guest) { button.textContent = t('need_account'); button.disabled = true; }
    else if (balance < plan.price) { button.textContent = `${t('need_kup')} (${plan.price - balance})`; button.disabled = true; }
    else { button.textContent = `${t('buy')} · ${plan.price} ${t('kup')}`; button.className = 'primary'; button.onclick = () => buyPlan(id, plan, button); }
    card.append(button); cards.append(card);
  }
  renderPlanExtras();
  const rules = siteStatus.kup_rules || {};
  $('#kupEarn').replaceChildren(node('h3', '', t('earn_title')), ...[['hosgeldin', 'earn_welcome'], ['gunluk', 'earn_daily'], ['mesaj', 'earn_message'], ['feedback', 'earn_feedback'], ['ogretme', 'earn_teach'], ['satin_alma', 'earn_buy'], ['davet', 'earn_invite']]
    .filter(([key]) => rules[key]).map(([key, label]) => node('div', 'earn-row', `+${rules[key][0]} ${t('kup')} · ${t(label)}`)), node('p', 'fine', t('earn_note')));
}
async function buyPlan(id, plan, button) {
  if (!confirm(`${plan.name}: ${plan.price} ${t('kup')}. ${t('buy_confirm')}`)) return;
  button.disabled = true;
  try { await api('/kup/buy', {method:'POST', body:{tier:id}}); toast(t('bought')); await refreshAccount(); await renderPlans(); }
  catch (error) { toast(error.message); button.disabled = false; }
}
// Pencere önce açılır; sunucu cevap vermezse içinde hata yazar.
$('#planButton').onclick = async () => {
  if (!$('#plansDialog').open) $('#plansDialog').showModal();
  if (siteStatus.tiers) await renderPlans(); else $('#planCards').replaceChildren(node('p', 'muted', '…'));
  try { if (!siteStatus.tiers) await refreshSite(); await refreshAccount(); await renderPlans();
    if (!siteStatus.tiers) $('#planCards').replaceChildren(node('p', 'muted', t('unreachable'))); }
  catch (error) { $('#planCards').replaceChildren(node('p', 'muted', error.message)); }
};
$('#closePlans').onclick = () => $('#plansDialog').close();
async function renderRequests() {
  const [data, requests] = await Promise.all([api('/kup/members'), api('/work/requests')]), tiers = siteStatus.tiers || {};
  $('#workRequests').replaceChildren(...(requests.pending.length ? requests.pending.map(item => {
    const row = node('div', 'request-row'), buttons = node('div', 'button-row');
    row.append(node('span', '', `${item.name} · ${tiers[item.tier]?.name || item.tier}`));
    for (const [decision, label, cls] of [['accept', t('accept'), 'primary'], ['reject', t('reject'), 'secondary']]) {
      const b = node('button', cls, label); b.onclick = async () => { b.disabled = true; try { await api('/work/requests/' + item.id, {method:'POST', body:{decision}}); await renderRequests(); } catch (error) { toast(error.message); b.disabled = false; } }; buttons.append(b);
    }
    row.append(buttons); return row;
  }) : [node('p', 'muted', t('no_requests_w'))]));
  $('#memberList').replaceChildren(...(data.members.length ? data.members.map(m => node('div', 'request-row',
    `${m.name} · ${tiers[m.tier]?.name || m.tier} · ${m.kup} ${t('kup')}`)) : [node('p', 'muted', t('no_members'))]));
}
$('#grantForm').onsubmit = async event => {
  event.preventDefault();
  try { await api('/kup/grant', {method:'POST', body:{username:$('#grantUser').value.trim(), amount:Number($('#grantAmount').value)}}); $('#grantAmount').value = ''; await renderRequests(); }
  catch (error) { toast(error.message); }
};
$('#requestsButton').onclick = async () => { try { if (!siteStatus.tiers) await refreshSite(); await renderRequests(); $('#requestsDialog').showModal(); } catch (error) { toast(error.message); } };
$('#closeRequests').onclick = () => $('#requestsDialog').close();
// Code arka planı: yavaş kayan yıldızlar, yakın olanlar ve fareye yakın olanlar çizgiyle bağlanır.
(() => {
  const canvas = $('#stars'); if (!canvas) return;
  const ctx = canvas.getContext('2d'), still = matchMedia('(prefers-reduced-motion: reduce)').matches;
  let stars = [], width = 0, height = 0, mouse = null, frame = 0;
  function resize() {
    const ratio = Math.min(devicePixelRatio || 1, 2);
    width = innerWidth; height = innerHeight; canvas.width = width * ratio; canvas.height = height * ratio;
    canvas.style.width = width + 'px'; canvas.style.height = height + 'px'; ctx.setTransform(ratio, 0, 0, ratio, 0, 0);
    stars = Array.from({length: Math.min(140, Math.round(width * height / 11000))}, () => ({
      x: Math.random() * width, y: Math.random() * height, vx: (Math.random() - .5) * .12, vy: (Math.random() - .5) * .12,
      r: Math.random() * 1.3 + .3, twinkle: Math.random() * Math.PI * 2}));
  }
  function draw() {
    ctx.clearRect(0, 0, width, height);
    for (const star of stars) {
      if (!still) { star.x = (star.x + star.vx + width) % width; star.y = (star.y + star.vy + height) % height; star.twinkle += .02; }
      ctx.globalAlpha = .45 + Math.sin(star.twinkle) * .3; ctx.fillStyle = '#cfd8ff';
      ctx.beginPath(); ctx.arc(star.x, star.y, star.r, 0, Math.PI * 2); ctx.fill();
    }
    ctx.lineWidth = .6;
    for (let i = 0; i < stars.length; i++) for (let j = i + 1; j < stars.length; j++) {
      const a = stars[i], b = stars[j], d = Math.hypot(a.x - b.x, a.y - b.y);
      if (d < 115) { ctx.globalAlpha = (1 - d / 115) * .32; ctx.strokeStyle = '#8fa7ff'; ctx.beginPath(); ctx.moveTo(a.x, a.y); ctx.lineTo(b.x, b.y); ctx.stroke(); }
    }
    if (mouse) for (const star of stars) {
      const d = Math.hypot(star.x - mouse.x, star.y - mouse.y);
      if (d < 170) { ctx.globalAlpha = (1 - d / 170) * .55; ctx.strokeStyle = '#b8c6ff'; ctx.beginPath(); ctx.moveTo(star.x, star.y); ctx.lineTo(mouse.x, mouse.y); ctx.stroke(); }
    }
    ctx.globalAlpha = 1;
  }
  function loop() { frame = 0; if (document.body.classList.contains('code-theme') && !document.hidden) { draw(); if (!still) frame = requestAnimationFrame(loop); } }
  addEventListener('resize', () => { resize(); draw(); });
  addEventListener('pointermove', event => { mouse = {x: event.clientX, y: event.clientY}; if (still) draw(); });
  addEventListener('pointerleave', () => { mouse = null; });
  document.addEventListener('visibilitychange', () => { if (!frame) loop(); });
  new MutationObserver(() => { if (!frame) loop(); }).observe(document.body, {attributes: true, attributeFilter: ['class']});
  resize(); loop();
})();
// Claude benzeri kullanım göstergesi: 5 saatlik oturum + haftalık
function timeLeft(seconds) {
  const minutes = Math.max(1, Math.round(seconds / 60)), h = Math.floor(minutes / 60);
  return (h ? `${h} sa ` : '') + `${minutes % 60} dk`;
}
function renderUsage() {
  const meter = $('#usageMeter'), windows = account?.windows;
  if (!windows || account.role === 'admin') { meter.hidden = true; return; }
  const s = windows.session, w = windows.week;
  const sessionPct = Math.min(100, Math.round(100 * Math.max(s.tokens / Math.max(1, s.limit), s.message_limit < 1e9 ? s.messages / Math.max(1, s.message_limit) : 0)));
  const weekPct = Math.min(100, Math.round(100 * w.tokens / Math.max(1, w.limit))), pct = Math.max(sessionPct, weekPct);
  meter.hidden = false; $('#usagePct').textContent = '%' + pct;
  const ring = $('#usageRing'), length = 2 * Math.PI * 8;
  ring.style.strokeDasharray = `${length * pct / 100} ${length}`;
  meter.classList.toggle('warn', pct >= 75); meter.classList.toggle('full', pct >= 100);
  const bar = (label, value, note) => { const row = node('div', 'usage-row'), track = node('div', 'usage-track'), fill = node('span');
    fill.style.width = value + '%'; track.append(fill); row.append(node('div', 'usage-label', `${label} · %${value}`), track, node('small', '', note)); return row; };
  const sessionNote = s.reset_at ? `${timeLeft(s.reset_at - Date.now() / 1000)} ${t('resets_in')}` : '—';
  const weekNote = `${new Date(w.reset_at * 1000).toLocaleString(lang, {weekday:'long', hour:'2-digit', minute:'2-digit'})} ${t('resets_at')}`;
  const refill = node('button', 'secondary', `${t('refill')} · ${siteStatus.refill_price || 1000} ${t('kup')}`);
  refill.type = 'button'; refill.onclick = refillSession; refill.hidden = account.role === 'guest';
  $('#usagePop').replaceChildren(bar(t('session'), sessionPct, sessionNote), bar(t('weekly'), weekPct, weekNote), refill);
}
async function refillSession() {
  try { await api('/kup/refill', {method:'POST'}); toast(t('refilled')); await refreshAccount(); }
  catch (error) { toast(error.message); }
}
$('#usageMeter').onclick = event => { event.stopPropagation(); const pop = $('#usagePop'); pop.hidden = !pop.hidden; $('#usageMeter').setAttribute('aria-expanded', String(!pop.hidden)); };
document.addEventListener('click', event => { if (!event.target.closest('.usage-wrap')) $('#usagePop').hidden = true; });

// Planlar penceresindeki ekler: token yenileme ve Kübik Work
function renderPlanExtras() {
  const box = $('#planExtras'); if (!box || !account || account.role === 'guest') { box?.replaceChildren(); return; }
  const refill = node('div', 'extra-card'), work = node('div', 'extra-card work');
  const refillButton = node('button', 'secondary', `${t('refill')} · ${siteStatus.refill_price} ${t('kup')}`); refillButton.type = 'button';
  refillButton.disabled = (account.kup ?? 0) < siteStatus.refill_price; refillButton.onclick = async () => { await refillSession(); renderPlans(); };
  refill.append(node('h3', '', '⚡ ' + t('refill')), node('p', 'fine', t('refill_help')), refillButton);
  const addon = siteStatus.work_addon || {}, workButton = node('button', 'secondary'); workButton.type = 'button';
  if (account.work) { workButton.textContent = t('work_open'); workButton.disabled = true; }
  else if (addon.visible) { workButton.textContent = `${t('buy')} · ${addon.price} ${t('kup')} / 30 gün`; workButton.disabled = (account.kup ?? 0) < addon.price;
    workButton.onclick = async () => { try { await api('/kup/work', {method:'POST'}); await refreshAccount(); renderPlans(); } catch (error) { toast(error.message); } }; }
  else if ((siteStatus.work_min_tiers || []).includes(account.tier)) { workButton.textContent = t('work_request');
    workButton.onclick = async () => { workButton.disabled = true; try { await api('/work/request', {method:'POST'}); workButton.textContent = t('work_requested'); } catch (error) { toast(error.message); workButton.disabled = false; } }; }
  else { workButton.textContent = t('work_need_max'); workButton.disabled = true; }
  work.append(node('h3', '', '💬 ' + t('work_buy')), node('p', 'fine', t('work_help')), workButton);
  const shop = node('div', 'extra-card shop'), shopButton = node('button', 'primary', 'Mağazayı aç');
  shopButton.type = 'button'; shopButton.onclick = () => openShop();
  shop.append(node('h3', '', '🛍 Mağaza · Kübit kostümleri'), node('p', 'fine', 'Kübit’e Küp ile kostüm al: Aslan, Galaksi, Kral…'), shopButton);
  box.replaceChildren(refill, work, shop);
}

// Kübik Work: WhatsApp benzeri gelen kutusu (karşı taraf solda, sen ve yapay zekâ sağda)
const work = {chat: null, last: 0, timer: null, profile: null, tab: 'ai'};
const PUBLIC_SITE = 'https://asrinata.github.io/kubikai/';   // her ağdan açılan genel adres (GitHub Pages)
function workLink(slug) {
  return new URL('work.html', window.KUBIK_PAGES ? location.href : PUBLIC_SITE).href + '?w=' + encodeURIComponent(slug);
}
async function openWork() {
  try {
    const data = await api('/work'); work.profile = data.profile;
    $('#workLink').value = workLink(data.profile.slug);
    const shown = data.chats.filter(chat => (work.tab === 'ai') === !!chat.auto);
    $('#workChats').replaceChildren(...(shown.length ? shown.map(chat => {
      const button = node('button', 'work-chat-item' + (chat.id === work.chat ? ' active' : '')); button.type = 'button';
      button.append(node('strong', '', chat.visitor), node('span', '', (chat.last || '').slice(0, 60)));
      if (chat.unread) button.append(node('b', 'unread', String(chat.unread)));
      button.onclick = () => { work.chat = chat.id; work.last = 0; $('#workMessages').replaceChildren(); openWork(); };
      return button;
    }) : [node('p', 'fine', t('work_empty'))]));
    if (work.chat) await loadWorkChat(); else $('#workMessages').replaceChildren(node('p', 'work-pick', t('work_pick')));
  } catch (error) { $('#workChats').replaceChildren(node('p', 'fine', error.message)); }
  clearTimeout(work.timer); if (mode === 'work') work.timer = setTimeout(openWork, 4000);
}
function stopWork() { clearTimeout(work.timer); work.timer = null; }
async function loadWorkChat() {
  const data = await api(`/work/chats/${work.chat}?after=${work.last}`);
  $('#workVisitor').textContent = data.chat.visitor;
  document.querySelectorAll('[data-who]').forEach(b => b.classList.toggle('selected', (b.dataset.who === 'ai') === !!data.chat.auto));
  const box = $('#workMessages'), atBottom = box.scrollHeight - box.scrollTop - box.clientHeight < 60;
  box.querySelector('.work-pick')?.remove();
  for (const message of data.messages) {
    const bubble = node('div', 'bubble ' + {contact:'them', ai:'ai', owner:'me'}[message.sender]);
    if (message.sender !== 'contact') bubble.append(node('small', '', message.sender === 'ai' ? 'Kübik' : t('you')));
    bubble.append(node('p', '', message.content)); box.append(bubble); work.last = message.id;
  }
  if (atBottom || data.messages.length) box.scrollTop = box.scrollHeight;
}
$('#workForm').onsubmit = async event => {
  event.preventDefault(); const value = $('#workInput').value.trim(); if (!value || !work.chat) return;
  $('#workInput').value = '';
  try { await api(`/work/chats/${work.chat}`, {method:'POST', body:{content:value}}); await loadWorkChat(); } catch (error) { toast(error.message); }
};
document.querySelectorAll('[data-who]').forEach(button => button.onclick = async () => {
  if (!work.chat) return;
  try { await api(`/work/chats/${work.chat}`, {method:'PATCH', body:{auto:button.dataset.who === 'ai'}}); work.tab = button.dataset.who; markWorkTab(); openWork(); }
  catch (error) { toast(error.message); }
});
function markWorkTab() { document.querySelectorAll('[data-work-tab]').forEach(b => b.classList.toggle('selected', b.dataset.workTab === work.tab)); }
document.querySelectorAll('[data-work-tab]').forEach(button => button.onclick = () => { work.tab = button.dataset.workTab; markWorkTab(); openWork(); });
$('#copyWorkLink').onclick = async () => { try { await navigator.clipboard.writeText($('#workLink').value); toast(t('copied')); } catch { $('#workLink').select(); } };
$('#workSettings').onclick = () => { $('#workTitle').value = work.profile?.title || ''; $('#workInstructions').value = work.profile?.instructions || ''; $('#workDialog').showModal(); };
$('#closeWork').onclick = () => $('#workDialog').close();
$('#workProfileForm').onsubmit = async event => {
  event.preventDefault();
  try { await api('/work/profile', {method:'PUT', body:{title:$('#workTitle').value, instructions:$('#workInstructions').value}}); $('#workDialog').close(); openWork(); }
  catch (error) { toast(error.message); }
};

// Giriş ekranı: ızgara çizgileri fare üstündeyken kararır, çekilince yavaşça söner
(() => {
  const grid = document.querySelector('.art-grid'); if (!grid) return;
  const ns = 'http://www.w3.org/2000/svg', svg = document.createElementNS(ns, 'svg');
  function build() {
    const w = grid.clientWidth, h = grid.clientHeight; if (!w || !h) return;
    svg.setAttribute('viewBox', `0 0 ${w} ${h}`); svg.replaceChildren();
    const add = (x1, y1, x2, y2) => { const g = document.createElementNS(ns, 'g'); g.setAttribute('class', 'grid-line');
      for (const cls of ['hit', 'ink']) { const l = document.createElementNS(ns, 'line'); l.setAttribute('class', cls);
        Object.entries({x1, y1, x2, y2}).forEach(([k, v]) => l.setAttribute(k, v)); g.append(l); }
      svg.append(g); };
    for (let x = 13; x < w; x += 26) add(x, 0, x, h);
    for (let y = 13; y < h; y += 26) add(0, y, w, y);
  }
  grid.append(svg); build(); addEventListener('resize', build);
  new MutationObserver(build).observe(document.querySelector('#auth'), {attributes: true, attributeFilter: ['hidden']});
})();
// Kübik masaüstü uygulaması: PC erişimi (yetkiyi uygulamanın Python tarafı denetler, burası yalnız arayüz)
const pc = {mode: '', pending: [], steps: 0, info: null};
function pcTasks(text) {
  const tasks = [];
  for (const match of text.matchAll(/<pc>([\s\S]*?)<\/pc>/gi)) { try { tasks.push(JSON.parse(match[1])); } catch {} }
  return tasks.slice(0, 3);
}
async function runPcTasks(tasks) {
  if (++pc.steps > 6) { toast('PC işlemleri durdu: arka arkaya 6 adım sınırı.'); return; }
  const lines = [], attachments = [];
  for (const task of tasks) {
    $('#jobStatus').textContent = `PC: ${task.islem} ${task.yol || task.komut || ''}`;
    const result = await window.pywebview.api.pc_islem(task);
    lines.push(`• ${task.islem} ${task.yol || task.komut || ''}: ${result.ok ? 'tamam' : 'hata'}\n${(result.sonuc || result.hata || '').slice(0, 1500)}`);
    if (result.resim && attachments.length < 3) {
      const blob = await (await fetch('data:image/jpeg;base64,' + result.resim)).blob();
      const response = await kubikTransport.fetch('/api/attachments?name=ekran.jpg', {method:'POST', headers:{'X-Kubik-Request':'1', 'Content-Type':'application/octet-stream'}, body:blob});
      const value = await response.json(); if (response.ok) attachments.push(value);
    }
  }
  $('#jobStatus').textContent = '';
  pendingAttachments = attachments; renderAttachmentDrafts();
  await sendTurn(('PC sonuçları:\n' + lines.join('\n')).slice(0, 3900));
}
function renderPc() {
  const info = pc.info; if (!info) return;
  pc.mode = info.mode;
  const select = $('#pcMode'); select.replaceChildren(...Object.entries(info.modes).map(([value, name]) => { const o = node('option', '', name); o.value = value; return o; }));
  select.value = info.mode; $('#pcFolder').textContent = 'Klasör: ' + info.folder_path;
  const badge = $('#pcBadge'); badge.hidden = info.mode === 'kapali'; badge.textContent = 'PC · ' + info.mode_name; badge.classList.toggle('danger', info.mode === 'tam');
}
async function pcRefresh() { pc.info = await window.pywebview.api.pc_durum(); renderPc(); }
window.kubikPcChanged = () => pcRefresh();
window.addEventListener('pywebviewready', async () => {
  $('#pcSettings').hidden = false; document.querySelectorAll('.desktop-download').forEach(e => e.hidden = true);
  await pcRefresh();
  $('#pcMode').onchange = async () => { pc.info = await window.pywebview.api.pc_mod($('#pcMode').value); renderPc(); };
  $('#pcFolderButton').onclick = async () => { pc.info = await window.pywebview.api.pc_klasor(); renderPc(); };
});
if (/Windows/.test(navigator.userAgent) && !window.pywebview) document.querySelectorAll('.desktop-download').forEach(e => e.hidden = false);

// KVKK: gizlilik metni ve hesabı kalıcı silme
document.querySelectorAll('.privacy-link').forEach(link => link.onclick = () => { $('#deleteAccountForm').hidden = !account || account.role === 'admin' || account.owner; $('#deletePassword').hidden = account?.role === 'guest'; $('#privacyDialog').showModal(); });
$('#closePrivacy').onclick = () => $('#privacyDialog').close();
$('#deleteAccountForm').onsubmit = async event => { event.preventDefault(); if (!confirm(t('delete_confirm'))) return; try { await api('/account/delete', {method:'POST', body:{password:$('#deletePassword').value}}); kubikTransport.clearToken(); location.reload(); } catch (error) { toast(error.message); } };
applyLanguage();
refreshSite();enter().catch(()=>{account=null;});

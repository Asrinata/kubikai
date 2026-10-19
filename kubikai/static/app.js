'use strict';
const $ = s => document.querySelector(s);
let account = null, registerMode = false, current = null, job = null, mode = 'chat', siteStatus = {};
const isPages = window.KUBIK_PAGES === true;
let apiBase = isPages ? (localStorage.getItem('kubik.pi') || '') : '';
const accessToken = () => isPages ? sessionStorage.getItem('kubik.token') : '';
if (isPages) $('#connectionSetup').hidden = Boolean(apiBase);
$('#connectionSetup')?.addEventListener('submit', event => { event.preventDefault(); const value = $('#apiAddress').value.trim().replace(/\/$/, ''); if (!/^https:\/\/[a-z0-9-]+\.trycloudflare\.com$/i.test(value)) { $('#connectionError').textContent = 'Pi adresi https:// ile başlayan trycloudflare.com adresi olmalı.'; return; } localStorage.setItem('kubik.pi', value); location.reload(); });
const delay = ms => new Promise(resolve => setTimeout(resolve, ms));
function toast(message) { $('#toast').textContent = message; $('#toast').classList.add('show'); setTimeout(() => $('#toast').classList.remove('show'), 5000); }
async function api(path, options = {}) {
  const headers = {'X-Kubik-Request': '1', ...(options.headers || {})};
  const token = accessToken(); if (token) headers.Authorization = 'Bearer ' + token;
  if (options.body && typeof options.body !== 'string') { headers['Content-Type'] = 'application/json'; options.body = JSON.stringify(options.body); }
  const response = await fetch((apiBase || '') + '/api' + path, {...options, headers});
  const value = await response.json();
  if (!response.ok) { const text = typeof value.detail === 'string' ? value.detail : 'Bilgileri kontrol ederek tekrar dene.'; throw new Error(text); }
  return value;
}
function node(tag, className, text) { const element = document.createElement(tag); if (className) element.className = className; if (text != null) element.textContent = text; return element; }
function welcome() {
  $('#messages').replaceChildren();
  const box = node('div', 'welcome');
  box.append(node('span', 'cube large'), node('p', 'eyebrow', mode === 'code' ? 'FİKRİNDEN İLK SATIRA' : 'BİRLİKTE BİR ADIM İLERİ'));
  box.append(node('h1', '', mode === 'code' ? 'Bugün ne\nyapıyoruz?' : 'Bugün neyi\nkeşfediyoruz?'));
  box.lastChild.style.whiteSpace = 'pre-line';
  box.append(node('p', 'muted', mode === 'code' ? 'Kodunu yaz, ortamını başlat ve sonucu gör.' : 'Aklındaki soruyu, fikri ya da çözmek istediğin şeyi yaz.'));
  const suggestions = node('div', 'suggestions');
  for (const [title, subtitle, prompt] of [
    ['Bir fikir geliştirelim', 'İlk adımdan birlikte başlayalım', 'Bir fikri projeye dönüştürmek için nereden başlamalıyım?'],
    ['Bir şey kodlayalım', 'Fikrini çalışan koda dönüştür', "Python'da basit bir yapılacaklar listesi yaz."]
  ]) { const button = node('button'); button.append(node('span', '', title), node('small', '', subtitle)); button.onclick = () => { $('#prompt').value = prompt; $('#prompt').focus(); }; suggestions.append(button); }
  box.append(suggestions); $('#messages').append(box);
}
function updateWorkspaceLabel() { const label = $('.terminal-heading span'); if (label) label.textContent = '~/workspace/' + (current || 'default'); }
function addMessage(role, text, sources = [], extra = {}) {
  $('#welcome')?.remove(); $('.messages > .welcome')?.remove();
  const row = node('article', 'message ' + role);
  if (role === 'assistant') { const label = node('div', 'message-label'); label.append(node('span', 'cube'), node('span', '', 'KÜBİK')); row.append(label); }
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
  }
  $('#messages').append(row); $('#messages').scrollTop = $('#messages').scrollHeight;
}
async function refreshAccount() {
  account = await api('/me');
  $('#accountName').textContent = account.username; $('#avatar').textContent = account.username[0].toUpperCase();
  $('#accountRole').textContent = account.role === 'admin' ? 'Yönetici' : 'Kübik hesabı'; $('#adminLink').hidden = account.role !== 'admin';
  const used = account.usage, limits = account.limits;
  $('#quota').textContent = account.role === 'admin' ? `Bugün ${used.messages} mesaj · ${used.tokens.toLocaleString('tr')} token` : `Bugün ${used.messages} / ${limits.daily_messages} mesaj\n${used.tokens.toLocaleString('tr')} / ${limits.daily_tokens.toLocaleString('tr')} token`;
  $('#quota').style.whiteSpace = 'pre-line';
}
async function refreshConversations() {
  const rows = await api('/conversations'); $('#conversationList').replaceChildren();
  const visible = rows.filter(row => row.mode === mode);
  if (!visible.length) $('#conversationList').append(node('div', 'empty-list', 'İlk sohbetin burada görünecek.'));
  for (const row of visible) {
    const entry = node('div', 'conversation-entry');
    const button = node('button', row.id === current ? 'conversation-open active' : 'conversation-open', row.title); button.title = row.title;
    button.onclick = async () => { if (job) return toast('Önce devam eden yanıtı durdur.'); try { const data = await api('/conversations/' + row.id); current = row.id; updateWorkspaceLabel(); $('#messages').replaceChildren(); data.messages.forEach(m => addMessage(m.role, m.content, m.sources)); refreshConversations(); $('.sidebar').classList.remove('open'); } catch (error) { toast(error.message); } };
    const remove = node('button', 'conversation-delete', '×'); remove.type = 'button'; remove.title = 'Sohbeti ve dosyalarını sil'; remove.setAttribute('aria-label', row.title + ' sohbetini sil');
    remove.onclick = async () => { if (job) return toast('Önce devam eden yanıtı durdur.'); if (!confirm('Bu sohbet, mesajları ve ona ait proje dosyaları kalıcı olarak silinsin mi?')) return; try { await api('/conversations/' + row.id, {method:'DELETE'}); if (current === row.id) { current = null; welcome(); } await refreshConversations(); toast('Sohbet ve bağlı dosyaları silindi.'); } catch (error) { toast(error.message); } };
    entry.append(button, remove); $('#conversationList').append(entry);
  }
}
async function refreshSite() {
  try {
    siteStatus = await api('/status');
    $('#searchControl').hidden = !siteStatus.search_enabled || siteStatus.search_max_effort === 'off';
    if (siteStatus.admin_url) $('#adminLink').href = siteStatus.admin_url;
    $('#accessBadge').textContent = siteStatus.access_mode === 'beta' ? 'BETA · ERKEN ERİŞİM' : 'HERKESE AÇIK';
    $('#keyLabel').hidden = siteStatus.access_mode !== 'beta';
    $('#maintenance').hidden = !siteStatus.maintenance;
    $('#maintenance').textContent = siteStatus.maintenance_message;
    $('#modelStatus').textContent = siteStatus.model_ready ? 'Apex 5 · hazır' : 'Apex 5 · hazırlanıyor';
    $('#send').disabled = !!job || siteStatus.maintenance || !siteStatus.model_ready;
    $('#startVM').disabled = siteStatus.maintenance;
    const ranks = ['off', 'quick', 'balanced', 'deep'];
    [...$('#searchEffort').options].forEach(option => { option.disabled = option.value !== 'off' && (!siteStatus.search_enabled || ranks.indexOf(option.value) > ranks.indexOf(siteStatus.search_max_effort)); });
    if ($('#searchEffort').selectedOptions[0].disabled) $('#searchEffort').value = 'off';
    if (!account && siteStatus.maintenance) $('#authError').textContent = siteStatus.maintenance_message;
  } catch { $('#modelStatus').textContent = 'Sunucuya ulaşılamıyor'; }
}
async function enter() { await refreshAccount(); $('#auth').hidden = true; $('#workspace').hidden = false; welcome(); await refreshConversations(); await refreshSite(); }
$('#authToggle').onclick = () => { registerMode = !registerMode; $('#authTitle').textContent = registerMode ? 'Kübik’e hoş geldin.' : 'Tekrar hoş geldin.'; $('#authSubmit').textContent = registerMode ? 'Hesap oluştur' : 'Giriş yap'; $('#authToggle').textContent = registerMode ? 'Zaten hesabın var mı? Giriş yap' : 'Hesabın yok mu? Kayıt ol'; $('#password').autocomplete = registerMode ? 'new-password' : 'current-password'; $('#authError').textContent = ''; };
$('#authForm').onsubmit = async event => { event.preventDefault(); $('#authError').textContent = ''; $('#authSubmit').disabled = true; try { const result = await api('/auth/' + (registerMode ? 'register' : 'login'), {method:'POST', body:{username:$('#username').value, password:$('#password').value, access_key:$('#accessKey').value.trim()}}); if (isPages && result.token) sessionStorage.setItem('kubik.token', result.token); $('#password').value = ''; await enter(); } catch(error) { $('#authError').textContent = error.message; } finally { $('#authSubmit').disabled = false; } };
$('#logout').onclick = async () => { await api('/auth/logout', {method:'POST'}); sessionStorage.removeItem('kubik.token'); location.reload(); };
$('#newChat').onclick = () => { if (job) return toast('Önce devam eden yanıtı durdur.'); current = null; updateWorkspaceLabel(); welcome(); refreshConversations(); $('#prompt').focus(); };
$('#mode').onchange = () => { if (job) { $('#mode').value = mode; return toast('Önce devam eden yanıtı durdur.'); } mode = $('#mode').value; current = null; $('#modeTitle').textContent = mode === 'code' ? 'Kübik Code' : 'Kübik Chat'; $('#codePanel').hidden = mode !== 'code'; welcome(); refreshConversations(); if (mode === 'code') refreshVM(); };
$('#mobileMenu').onclick = () => $('.sidebar').classList.toggle('open');
$('#prompt').addEventListener('keydown', event => { if (event.key === 'Enter' && !event.shiftKey && !event.isComposing) { event.preventDefault(); if (!$('#send').disabled) $('#chatForm').requestSubmit(); } });
$('#chatForm').onsubmit = async event => {
  event.preventDefault(); if (job) return; const message = $('#prompt').value.trim(); if (!message) return;
  $('#send').disabled = true;
  try {
    const started = await api('/chat', {method:'POST',body:{message, conversation_id:current, mode, effort:$('#effort').value, search_effort:$('#searchEffort').value}});
    current = started.conversation_id; job = started.job_id; addMessage('user', message); $('#prompt').value = ''; $('#stopReply').hidden = false; $('#send').hidden = true;
    await refreshConversations();
    for (;;) { const progress = await api('/jobs/' + job); $('#jobStatus').textContent = progress.status;
      if (progress.state === 'done') { addMessage('assistant', progress.result.text, progress.result.sources, progress.result); break; }
      if (['error','canceled'].includes(progress.state)) { toast(progress.status); break; }
      await delay(1200);
    }
  } catch(error) { toast(error.message); }
  finally { job = null; $('#stopReply').hidden = true; $('#send').hidden = false; $('#jobStatus').textContent = ''; await refreshSite(); if (account) refreshAccount().catch(() => {}); }
};
$('#stopReply').onclick = async () => { if (job) { await api('/jobs/' + job + '/cancel', {method:'POST'}); $('#jobStatus').textContent = 'Durduruluyor'; } };
async function refreshVM() { if (!account || mode !== 'code') return; try { const vm = await api('/vm'); const labels = {stopped:'Ortam kapalı', starting:'Ortam hazırlanıyor · ilk açılış birkaç dakika sürebilir', running:`Çalışıyor · ${vm.memory_mb} MB RAM`, queued:`Sıradasın · ${vm.position}. sırada`}; $('#vmStatus').textContent = labels[vm.state] || vm.state; $('#vmError').textContent = vm.error || (vm.available === false ? 'VM bileşenleri eksik. Pi’deki güncelleme ve kurulum kayıtlarını yönetici kontrol etmeli.' : ''); $('#startVM').disabled = !['stopped'].includes(vm.state) || siteStatus.maintenance || vm.available === false; if (vm.error && !refreshVM.lastErrorShown) { toast(vm.error); refreshVM.lastErrorShown = true; } if (!vm.error) refreshVM.lastErrorShown = false; } catch(error) { $('#vmStatus').textContent = error.message; } }
$('#startVM').onclick = async () => { try { await api('/vm/start', {method:'POST'}); await refreshVM(); } catch(error) { toast(error.message); } };
$('#stopVM').onclick = async () => { try { await api('/vm/stop', {method:'POST'}); await refreshVM(); } catch(error) { toast(error.message); } };
async function execute(command) { if (!command.trim()) return; updateWorkspaceLabel(); $('#terminalOutput').textContent = '$ ' + command.slice(0,200) + '\nÇalışıyor…'; try { const result = await api('/vm/execute', {method:'POST',body:{command,workspace_id:current||'default'}}); $('#terminalOutput').textContent = result.output || '(Çıktı yok)'; } catch(error) { $('#terminalOutput').textContent = error.message; } }
$('#terminalForm').onsubmit = event => { event.preventDefault(); execute($('#command').value); };
$('#downloadForm')?.addEventListener('submit', async event => { event.preventDefault(); const path = $('#downloadPath').value.trim(); if (!path) return; const query = new URLSearchParams({workspace_id:current||'default',path}); const headers={'X-Kubik-Request':'1'}; const token=accessToken(); if(token) headers.Authorization='Bearer '+token; try { const response=await fetch((apiBase||'')+'/api/vm/files?'+query,{headers}); if(!response.ok){let message='Dosya indirilemedi.';try{const value=await response.json();message=value.detail||message;}catch{} throw new Error(message);} const blob=await response.blob(), url=URL.createObjectURL(blob), link=node('a'); link.href=url; link.download=path.split('/').pop()||'kubik-file'; link.click(); URL.revokeObjectURL(url); toast('Dosya indirildi.'); } catch(error) { toast(error.message); } });
document.querySelectorAll('[data-prompt]').forEach(button => button.onclick = () => { $('#prompt').value = button.dataset.prompt; $('#prompt').focus(); });
refreshSite(); enter().catch(() => { account = null; });
setInterval(refreshSite, 10000); setInterval(refreshVM, 5000);

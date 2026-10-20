'use strict';
const $ = s => document.querySelector(s);
let account = null, registerMode = false, current = null, job = null, mode = 'chat', siteStatus = {};
let selectedModel = null;
const modelName = id => id === 'auto' ? 'Otomatik' : id === 'apex-5.5' ? 'Apex 5.5' : 'Kübik 1.0';
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
  box.append(node('span', 'cube large'), node('p', 'eyebrow', mode === 'code' ? 'FİKRİNDEN İLK SATIRA' : 'BİRLİKTE BİR ADIM İLERİ'));
  box.append(node('h1', '', mode === 'code' ? 'Birlikte ne üretelim?' : 'Aklında ne var?'));
  box.lastChild.style.whiteSpace = 'pre-line';
  box.append(node('p', 'muted', mode === 'code' ? 'Bir fikir anlat. Kübik planlasın, kodlasın ve denesin.' : 'Bir soruyla başla. Bir fikri geliştir. Yeni bir şey keşfet.'));
  const suggestions = node('div', 'suggestions');
  for (const [title, subtitle, prompt] of [
    ['Bir fikir geliştirelim', 'İlk adımdan birlikte başlayalım', 'Bir fikri projeye dönüştürmek için nereden başlamalıyım?'],
    ['Bir konuyu anlayalım', 'Zor görüneni birlikte sadeleştirelim', 'Bana yeni bir konuyu basit örneklerle öğret.'],
    ['Bir plan hazırlayalım', 'Dağınık fikirlerine bir yol çiz', 'Hedefime ulaşmam için uygulanabilir bir plan hazırlamama yardım et.'],
    ['Birlikte yazalım', 'Kelimelerini bul, anlatımını güçlendir', 'Yazdığım bir metni daha anlaşılır ve doğal hâle getirmeme yardım et.']
  ]) { const button = node('button'); button.append(node('span', '', title), node('small', '', subtitle)); button.onclick = () => { $('#prompt').value = prompt; $('#prompt').focus(); }; suggestions.append(button); }
  if (mode === 'code') { suggestions.replaceChildren(); for (const [title, prompt] of [['Bir web sitesi yap', 'Sade ve kullanışlı bir web sitesi oluştur. Dosyalarını yaz ve kontrol et.'], ['Bir projeyi tamamla', 'Projem için önce kısa bir plan çıkar, sonra dosyaları oluşturup çalışmasını kontrol et.']]) { const button=node('button'); button.append(node('span','',title), node('small','','Planla, geliştir, dene')); button.onclick=()=>{$('#prompt').value=prompt;$('#prompt').focus();}; suggestions.append(button); } }
  box.append(suggestions); $('#messages').append(box);
}
function updateWorkspaceLabel() { const label = $('.terminal-heading span'); if (label) label.textContent = '~/workspace/' + (current || 'default'); }
function addMessage(role, text, sources = [], extra = {}) {
  $('#welcome')?.remove(); $('.messages > .welcome')?.remove();
  const row = node('article', 'message ' + role);
  if (role === 'assistant') { const label = node('div', 'message-label'); label.append(node('span', 'cube'), node('span', '', extra.model_id ? modelName(extra.model_id) : 'KÜBİK')); row.append(label); }
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
  $('#memoryUsage').textContent = (account.memory_bytes/1000).toFixed(1) + ' KB / 1 MB';
  $('#mode').querySelector('[value=code]').disabled = guest;
  document.querySelector('[data-mode=code]').hidden = guest;
  $('#logout').title = guest ? 'Hesabına giriş yap' : 'Çıkış yap';
  $('#accountName').textContent = guest ? 'Misafir' : account.username; $('#avatar').textContent = account.username[0].toUpperCase();
  $('#accountRole').textContent = account.role === 'admin' ? 'Yönetici' : guest ? 'Code ve hafıza için giriş yap' : 'Kübik hesabı'; $('#adminLink').hidden = account.role !== 'admin';
  const used = account.usage, limits = account.limits;
  $('#quota').textContent = account.role === 'admin' ? `Bugün ${used.messages} mesaj · ${used.tokens.toLocaleString('tr')} token` : `Bugün ${used.messages} / ${limits.daily_messages} mesaj\n${used.tokens.toLocaleString('tr')} / ${limits.daily_tokens.toLocaleString('tr')} token`;
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
    if (!selectedModel) {
      try { selectedModel = localStorage.getItem('kubik-model-v2'); } catch {}
      if (selectedModel !== 'auto' && !siteStatus.models?.some(model => model.id === selectedModel && model.installed)) selectedModel = siteStatus.default_model;
    }
    const picker = $('#modelPicker');
    const routedId = selectedModel === 'auto' ? siteStatus.model_routing?.[mode] : selectedModel;
    for (const option of picker.options) {
      if (option.value === 'auto') { option.disabled = false; option.textContent = 'Otomatik'; continue; }
      const info = siteStatus.models?.find(model => model.id === option.value);
      option.disabled = !info?.installed;
      option.textContent = modelName(option.value) + (info?.installed ? '' : ' · kurulu değil');
    }
    picker.value = selectedModel;
    picker.disabled = !!job;
    const chosen = siteStatus.models?.find(model => model.id === routedId);
    $('#modelStatus').textContent = !chosen?.installed ? modelName(routedId) + ' · kurulu değil' : modelName(routedId) + (chosen.loaded ? ' · hazır' : ' · seçildi');
    $('#send').disabled = !!job || siteStatus.maintenance || !chosen?.installed;
    $('#startVM').disabled = siteStatus.maintenance;
    const ranks = ['off', 'quick', 'balanced', 'deep'];
    [...$('#searchEffort').options].forEach(option => { option.disabled = option.value !== 'off' && (!siteStatus.search_enabled || ranks.indexOf(option.value) > ranks.indexOf(siteStatus.search_max_effort)); });
    if ($('#searchEffort').selectedOptions[0].disabled) $('#searchEffort').value = 'off';
    if (!account && siteStatus.maintenance) $('#authError').textContent = siteStatus.maintenance_message;
  } catch { $('#modelStatus').textContent = 'Sunucuya ulaşılamıyor'; }
}
$('#modelPicker').onchange = () => {
  if (job) { $('#modelPicker').value = selectedModel; return toast('Önce devam eden yanıtı durdur.'); }
  selectedModel = $('#modelPicker').value;
  try { localStorage.setItem('kubik-model-v2', selectedModel); } catch {}
  refreshSite();
};
async function enter() { await refreshAccount(); $('#auth').hidden = true; $('#workspace').hidden = false; welcome(); await refreshConversations(); await refreshSite(); }
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
  $('#prompt').placeholder = mode === 'code' ? 'Ne geliştirmek istiyorsun?' : 'Kübik’e bir şey sor…';
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
  $('#memoryMeter').value = data.bytes;
  $('#memoryMeterLabel').textContent = (data.bytes/1000).toLocaleString('tr', {maximumFractionDigits:1}) + ' KB / 1 MB';
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
    const started = await api('/chat', {method:'POST',body:{message, conversation_id:current, mode, model_id:selectedModel, effort:$('#effort').value, search_effort:$('#searchEffort').value, attachment_ids:editing?[]:pendingAttachments.map(a=>a.id), ...extras}});
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
  uploadBusy=true;$('#attachFile').disabled=true;$('#attachmentStatus').textContent='Dosya okunuyor…';
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
refreshSite();enter().catch(()=>{account=null;});

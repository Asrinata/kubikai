'use strict';
const $ = s => document.querySelector(s);
const node = (tag, cls, text) => { const n = document.createElement(tag); if (cls) n.className = cls; if (text != null) n.textContent = text; return n; };
function toast(message) { $('#toast').textContent = message; $('#toast').classList.add('show'); setTimeout(() => $('#toast').classList.remove('show'), 6000); }
async function api(path, options = {}) { const headers = {'X-Kubik-Request':'1'}; if (options.body) { headers['Content-Type']='application/json'; options.body=JSON.stringify(options.body); } const r=await kubikTransport.fetch('/api'+path,{...options,headers}); const v=await r.json(); if(v.token)kubikTransport.setToken(v.token); if(!r.ok)throw new Error(typeof v.detail==='string'?v.detail:'Bilgileri kontrol et.'); return v; }
function action(button, fn) { button.addEventListener('click', async event => { event.preventDefault(); button.disabled=true; try { await fn(); } catch(error) { toast(error.message); } finally { button.disabled=false; } }); }
function submit(form, fn) { form.addEventListener('submit', async event => { event.preventDefault(); const button=form.querySelector('button'); button.disabled=true; try { await fn(); } catch(error) { toast(error.message); } finally { button.disabled=false; } }); }
const gb = n => (n / 1e9).toFixed(2) + ' GB';
function modelRow(value, archived) { const row=node('div','model-row'), details=node('div'); details.append(node('strong','',value.version||value.id),node('p','',`${gb(value.bytes)} · Kübik 1.0`)); row.append(details); if(archived) { const restore=node('button','secondary','Bu sürüme dön'); action(restore,async()=>{if(!confirm((value.version||value.id)+' sürümüne dönülsün mü? İşlem sırasında bakım mesajı gösterilir.'))return;await api('/admin/models/'+value.id+'/restore',{method:'POST'});await load(false);toast('Önceki model geri yüklendi.');});row.append(restore); const button=node('button','danger','Arşivi sil'); action(button,async()=>{if(!confirm('Bu eski model kalıcı olarak silinsin mi?'))return;await api('/admin/models/'+value.id,{method:'DELETE'});await load(false);toast('Arşiv silindi.');});row.append(button);}return row; }
async function load(fill=true) {
  const data=await api('/admin/overview'), s=data.settings;
  $('#userCount').textContent=data.users;$('#messageCount').textContent=data.usage.messages;$('#tokenCount').textContent=data.usage.tokens.toLocaleString('tr')+' token';$('#diskFree').textContent=gb(data.models.free_bytes);
  $('#serverState').textContent=data.publishing?'Yeni model yayınlanıyor; kullanıcılar bakım mesajını görüyor.':s.maintenance?s.maintenance_message:data.models.ready?'Model hazır. Sunucu istek kabul ediyor.':'Model henüz hazır değil. '+(data.model_error||'');
  $('#publishState').textContent=data.publish_status;
  $('#vmSummary').textContent=data.vms.available?`${data.vms.used_mb} / ${s.vm_pool_mb} MB RAM · ${data.vms.queued} kişi sırada · ${gb(data.vms.disk_bytes)} VM diski`:'VM özelliği ARM64 Raspberry Pi kurulumunda etkinleşir.';
  $('#vmList').replaceChildren();for(const vm of data.vms.sessions){const row=node('div','model-row');row.append(node('span','',`${vm.id} · ${vm.memory_mb} MB · ${vm.state}`));const stop=node('button','secondary','Durdur');action(stop,async()=>{await api('/admin/vms/'+vm.user_id+'/stop',{method:'POST'});await load(false);});row.append(stop);$('#vmList').append(row);}
  $('#modelCatalog').replaceChildren();
  for (const model of data.models.catalog || []) {
    const card=node('div'), link=node('a','',model.base_model);link.href=model.source_url;link.target='_blank';link.rel='noopener noreferrer';
    card.append(node('strong','',model.name + (model.specialty === 'code' ? ' · Code desteği' : ' · Ana sohbet modeli')),node('p','',!model.installed?'Kurulu değil':model.loaded?'Bellekte hazır':'Kurulu · seçildiğinde yüklenir'),link);$('#modelCatalog').append(card);
  }
  $('#activeModel').replaceChildren(data.models.current?modelRow(data.models.current,false):node('p','','Henüz model yok.'));
  $('#archivedModels').replaceChildren();data.models.archive.forEach(m=>$('#archivedModels').append(modelRow(m,true)));if(!data.models.archive.length)$('#archivedModels').append(node('p','','Eski model arşivi yok.'));
  await loadTraining();
  if(fill){$('#accessMode').value=s.access_mode;$('#registrationOpen').checked=s.registration_open;$('#maintenanceOn').checked=s.maintenance;$('#maintenanceMessage').value=s.maintenance_message;
    document.querySelectorAll('[data-setting]').forEach(input=>input.value=s[input.dataset.setting]);
    $('#systemPrompt').value=s.system_prompt;
    await Promise.all([loadKeys(),loadUsers(),loadCorrections()]);}
}
async function loadKeys(){const keys=await api('/admin/keys');$('#keysList').replaceChildren();for(const key of keys){const row=node('div','model-row'), info=node('div');info.append(node('strong','',key.label),node('p','',`${key.uses}/${key.max_uses} kullanım · ${new Date(key.expires*1000).toLocaleDateString('tr')} · ${key.revoked?'İptal edildi':'Etkin'}`));row.append(info);if(!key.revoked){const b=node('button','danger','İptal et');action(b,async()=>{await api('/admin/keys/'+key.id,{method:'DELETE'});await loadKeys();});row.append(b);}$('#keysList').append(row);}}
async function loadUsers(){const users=await api('/admin/users');$('#usersList').replaceChildren();const table=node('table','data-table'),head=node('tr');['Kullanıcı','Durum','Bugünkü mesaj','Özel limitler','Erişim'].forEach(t=>head.append(node('th','',t)));table.append(head);for(const user of users){const row=node('tr');row.append(node('td','',user.name),node('td','',user.disabled?'Kapalı':user.role==='admin'?'Yönetici':user.beta?'Beta':'Public'),node('td','',String(user.usage.messages)));const limits=node('td'),actions=node('td');if(user.role!=='admin'){const inputs={}, wrap=node('div','user-limits');for(const [key,title]of[['daily_messages','Mesaj'],['daily_tokens','Token'],['daily_searches','Arama'],['daily_vm_minutes','Code dakika']]){const input=node('input');input.type='number';input.min='0';input.placeholder=title;input.title=title;input.setAttribute('aria-label',user.name+' '+title);input.value=user.limits[key]??'';inputs[key]=input;wrap.append(input);}const save=node('button','secondary','Kaydet');action(save,async()=>{const custom={};for(const[key,input]of Object.entries(inputs))if(input.value!=='')custom[key]=Number(input.value);await api('/admin/users/'+user.id,{method:'PATCH',body:{limits:custom}});toast('Kullanıcı limitleri kaydedildi.');});wrap.append(save);limits.append(wrap);const ban=node('button','secondary',user.disabled?'Hesabı aç':'Hesabı kapat');action(ban,async()=>{await api('/admin/users/'+user.id,{method:'PATCH',body:{disabled:!user.disabled}});await loadUsers();});const beta=node('button','secondary',user.beta?'Beta erişimini kaldır':'Beta erişimi ver');action(beta,async()=>{await api('/admin/users/'+user.id,{method:'PATCH',body:{beta:!user.beta}});await loadUsers();});actions.append(ban,beta);}row.append(limits,actions);table.append(row);}$('#usersList').append(table);}
async function enter(){const user=await api('/me');if(user.role!=='admin')throw new Error('Bu hesap yönetici değil.');await load();$('#adminLogin').hidden=true;$('#adminApp').hidden=false;}
submit($('#adminLogin'),async()=>{try{await api('/auth/login',{method:'POST',body:{username:$('#adminUser').value.trim().toLowerCase(),password:$('#adminPassword').value}});$('#adminPassword').value='';await enter();}catch(error){$('#loginError').textContent=error.message;}});
document.querySelectorAll('[data-tab]').forEach(button=>button.onclick=()=>{document.querySelectorAll('[data-tab]').forEach(b=>b.classList.toggle('active',b===button));document.querySelectorAll('.admin-section').forEach(s=>s.classList.toggle('active',s.id==='tab-'+button.dataset.tab));$('#pageTitle').textContent=button.textContent;});
submit($('#accessForm'),async()=>{await api('/admin/settings',{method:'PATCH',body:{access_mode:$('#accessMode').value,registration_open:$('#registrationOpen').checked,maintenance:$('#maintenanceOn').checked,maintenance_message:$('#maintenanceMessage').value}});toast('Erişim ve bakım ayarları kaydedildi.');await load(false);});
submit($('#keyForm'),async()=>{const result=await api('/admin/keys',{method:'POST',body:{label:$('#keyName').value,max_uses:Number($('#keyUses').value),days:Number($('#keyDays').value)}});$('#newKey').hidden=false;$('#newKey').textContent=result.key+'\nBu anahtarı şimdi kopyala; tekrar gösterilmez.';await loadKeys();});
submit($('#limitsForm'),async()=>{const body={};document.querySelectorAll('[data-setting]').forEach(input=>body[input.dataset.setting]=Number(input.value));await api('/admin/settings',{method:'PATCH',body});toast('Kullanım sınırları kaydedildi.');});
submit($('#filtersForm'),async()=>{await api('/admin/settings',{method:'PATCH',body:{system_prompt:$('#systemPrompt').value}});toast('Temel talimat kaydedildi.');});
submit($('#passwordForm'),async()=>{await api('/admin/password',{method:'POST',body:{current:$('#oldPassword').value,password:$('#newPassword').value}});$('#oldPassword').value='';$('#newPassword').value='';toast('Yönetici parolası değiştirildi.');});
action($('#publishKey'),async()=>{if(!confirm('Yeni anahtar oluşturulunca Studio’daki eski anahtar çalışmaz. Devam edilsin mi?'))return;const result=await api('/admin/publish-key',{method:'POST'});$('#newPublishKey').hidden=false;$('#newPublishKey').textContent=result.key+'\nBu anahtarı Studio > Raspberry Pi yayını bölümüne gir.';});
action($('#refresh'),()=>load());action($('#adminLogout'),async()=>{await api('/auth/logout',{method:'POST'});kubikTransport.clearToken();location.reload();});

api('/status').then(status=>{if(status.chat_url)document.querySelectorAll('a[href="/"]').forEach(link=>link.href=status.chat_url);}).catch(()=>{});
setInterval(()=>{if(!$('#adminApp').hidden)load(false).catch(()=>{});},15000);

const trainingLabels = {queued:'Sırada',running:'Öğreniyor',paused:'Duraklatıldı',saving:'Kaydediliyor',preparing:'Model hazırlanıyor',ready:'Yayın bekliyor',publishing:'Kalite kontrolü ve yayın',published:'Yayında',error:'Kontrol gerekli',canceled:'İptal edildi'};
async function loadTraining() {
  const info=await api('/admin/training'), job=info.job, worker=info.worker;
  $('#workerBadge').textContent=worker.online?(worker.capable?'Bağlı · hazır':'Bağlı · kurulum gerekli'):'Çevrimdışı';
  $('#workerInfo').textContent=worker.online?worker.name+' · '+worker.detail:'Eğitim bilgisayarı bağlı değil. Bilgisayardaki eğitim köprüsünü aç.';
  $('#modelSource').href=info.source_url;
  $('#trainingState').textContent=job?(trainingLabels[job.state]||job.state):'Hazır';
  $('#trainingProgress').value=job?.progress||0;
  $('#trainingMessage').textContent=job?.message||'Henüz eğitim başlatılmadı.';
  $('#trainingStats').replaceChildren();
  if(job) for(const [title,value] of [['Çalışma',((job.elapsed||0)/60).toFixed(1)+' / '+job.minutes+' dk'],['Adım',job.step||0],['Yayın',job.publish_at?new Date(job.publish_at*1000).toLocaleString('tr-TR',{timeZone:'Europe/Istanbul'}):job.auto_publish?'Hazır olunca':'Elle'],['Son kayıt',job.checkpoint?'Kaydedildi':'Henüz yok']]) {const item=node('div');item.append(node('small','',title),node('strong','',String(value)));$('#trainingStats').append(item);}
  $('#trainingQuality').hidden=!job?.quality;
  if(job?.quality) {const q=job.quality; $('#trainingQuality').textContent=q.score!=null?`Yayın kontrolü: ${q.score}/${q.total} · önceki: ${q.baseline_score??'yok'}\n${q.scope}`:`Ayrı doğrulama kaybı: ${Number(q.baseline_loss).toFixed(4)} → ${Number(q.candidate_loss).toFixed(4)}\n${q.passed?'Eğitim doğrulaması geçti':'Eğitim doğrulaması geçmedi'}`;}
  document.querySelectorAll('[data-training-action]').forEach(button=>{const action=button.dataset.trainingAction;button.disabled=!job||job.state==='publishing'||['published','canceled'].includes(job.state)||(job.state==='error'&&action!=='publish');});
  $('#trainingForm button').disabled=!!job&&!['published','error','canceled'].includes(job.state);
}
submit($('#trainingForm'),async()=>{const minutes=Number($('#trainingMinutes').value),publish_at=$('#trainingPublishAt').value||null;await api('/admin/training',{method:'POST',body:{minutes,auto_publish:$('#trainingAuto').checked,publish_at}});await loadTraining();toast('Eğitim görevi kaydedildi. Bilgisayar bağlıysa başlayacak.');});
document.querySelectorAll('[data-training-action]').forEach(button=>action(button,async()=>{await api('/admin/training/control',{method:'POST',body:{action:button.dataset.trainingAction}});await loadTraining();}));
submit($('#scheduleForm'),async()=>{await api('/admin/training/control',{method:'POST',body:{action:'schedule',publish_at:$('#rescheduleAt').value}});await loadTraining();toast('Yayın zamanı Türkiye saatine göre kaydedildi.');});
action($('#downloadModel'),async()=>{const response=await kubikTransport.fetch('/api/admin/model/download');if(!response.ok)throw new Error('Model indirilemedi.');const url=URL.createObjectURL(await response.blob()),link=node('a');link.href=url;link.download='kubik-1.0.gguf';link.click();setTimeout(()=>URL.revokeObjectURL(url),30000);});
setInterval(()=>{if(!$('#adminApp').hidden&&$('#tab-training').classList.contains('active'))loadTraining().catch(()=>{});},5000);



async function loadCorrections(){
  const data=await api('/admin/corrections');$('#correctionCounts').textContent=`${data.counts.pending||0} bekleyen · ${data.counts.approved||0} onaylanan · ${data.counts.rejected||0} reddedilen`;
  $('#correctionsList').replaceChildren();
  for(const item of data.items){const card=node('div','card correction-card');card.append(node('span','badge',({pending:'Onay bekliyor',approved:'Onaylandı',rejected:'Reddedildi'})[item.status]),node('h3','','Soru'),node('p','correction-question',item.question));
    const original=node('details'),summary=node('summary','','Eski yanıtı göster');original.append(summary,node('p','correction-original',item.original));card.append(original);
    const label=node('label','','Eğitimde kullanılacak doğru yanıt'),answer=node('textarea');answer.rows=6;answer.maxLength=8000;answer.value=item.answer;answer.setAttribute('aria-label','Eğitimde kullanılacak doğru yanıt');label.append(answer);card.append(label);
    const controls=node('div','button-row');for(const [status,title]of[['approved','Onayla ve eğitim havuzuna ekle'],['rejected','Reddet / havuzdan çıkar']]){const button=node('button',status==='approved'?'primary':'secondary',title);action(button,async()=>{await api('/admin/corrections/'+item.id,{method:'POST',body:{status,answer:answer.value}});await loadCorrections();toast(status==='approved'?'Sonraki eğitim için onaylandı.':'Eğitim havuzundan çıkarıldı.');});controls.append(button);}card.append(controls);$('#correctionsList').append(card);
  }
  if(!data.items.length)$('#correctionsList').append(node('p','muted','Henüz gönderilen bir düzeltme yok.'));
}
action($('#refreshCorrections'),()=>loadCorrections());
document.querySelector('[data-tab=corrections]').addEventListener('click',()=>loadCorrections().catch(error=>toast(error.message)));
enter().catch(()=>{});

'use strict';
const slug = new URLSearchParams(location.search).get('w') || '', key = 'kubik.work.' + slug, $ = s => document.querySelector(s);
let token = localStorage.getItem(key) || '', last = 0;
async function call(path, body) {
  const response = await kubikTransport.fetch('/api/w/' + encodeURIComponent(slug) + path,
    body ? {method:'POST', headers:{'Content-Type':'application/json', 'X-Kubik-Request':'1'}, body:JSON.stringify(body)} : {headers:{'X-Kubik-Request':'1'}});
  const data = await response.json().catch(() => ({}));
  if (!response.ok) throw new Error(data.detail || 'Bağlantı hatası');
  return data;
}
function show(message) {
  const bubble = document.createElement('div'); bubble.className = 'bubble ' + (message.sender === 'contact' ? 'me' : 'them');
  if (message.sender !== 'contact') { const who = document.createElement('small'); who.textContent = message.sender === 'ai' ? 'Asistan' : $('#title').textContent; bubble.append(who); }
  bubble.append(document.createTextNode(message.content)); $('#log').append(bubble); last = message.id;
}
async function poll() {
  try { const {messages} = await call('/messages?token=' + encodeURIComponent(token) + '&after=' + last); messages.forEach(show); if (messages.length) $('#log').scrollTop = $('#log').scrollHeight; }
  catch (error) { if (/bulunamadı/.test(error.message)) { localStorage.removeItem(key); location.reload(); return; } }
  setTimeout(poll, 3000);
}
function chat() { $('#start').remove(); $('#sendForm').hidden = false; $('#message').focus(); poll(); }
$('#nameForm').onsubmit = async event => {
  event.preventDefault();
  try { token = (await call('/start', {name:$('#name').value.trim()})).token; localStorage.setItem(key, token); chat(); }
  catch (error) { $('#error').textContent = error.message; }
};
$('#sendForm').onsubmit = async event => {
  event.preventDefault(); const content = $('#message').value.trim(); if (!content) return; $('#message').value = '';
  try { await call('/send', {token, content}); } catch (error) { $('#message').placeholder = error.message; }
};
call('').then(data => { $('#title').textContent = data.title; $('#initial').textContent = data.title[0].toUpperCase(); document.title = data.title + ' · Kübik Work'; if (token) chat(); })
  .catch(error => { $('#error').textContent = error.message; $('#nameForm').hidden = true; });

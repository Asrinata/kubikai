'use strict';
// Model ömrü sayfası: çıkış / kapanış tarihleri ve kapanışa canlı geri sayım
(async () => {
  const box = document.querySelector('#lifespan');
  const el = (tag, cls, text) => { const n = document.createElement(tag); if (cls) n.className = cls; if (text != null) n.textContent = text; return n; };
  const date = value => value ? new Date(value + 'T00:00:00+03:00') : null;
  const show = value => value ? date(value).toLocaleDateString('tr', {day: 'numeric', month: 'long', year: 'numeric'}) : 'Belirsiz';
  let models;
  try { models = (await (await kubikTransport.fetch('/api/models/lifespan')).json()).models; }
  catch { box.replaceChildren(el('p', 'muted', 'Sunucuya ulaşılamadı. Biraz sonra tekrar dene.')); return; }
  const timers = [];
  box.replaceChildren(...models.slice().reverse().map(model => {
    const card = el('article', 'life-card'), end = date(model.shutdown), start = date(model.released), now = Date.now();
    const state = end && end <= now ? ['Kapandı', 'off'] : end ? ['Kapanacak', 'soon'] : start && start > now ? ['Yakında', 'new'] : ['Aktif', 'on'];
    const head = el('div', 'life-head'); head.append(el('h2', '', model.name), el('span', 'life-state ' + state[1], state[0]));
    const facts = el('dl'); for (const [k, v] of [['Çıkış', show(model.released)], ['Kapanış', model.shutdown ? show(model.shutdown) : 'Planlanmadı']]) facts.append(el('dt', '', k), el('dd', '', v));
    card.append(head, facts);
    if (end && end > now) { const clock = el('div', 'life-clock'); card.append(clock); timers.push([clock, end]); }
    if (start && end) {   // ömür çubuğu
      const bar = el('div', 'life-bar'), fill = el('span'); fill.style.width = Math.max(0, Math.min(100, 100 * (now - start) / (end - start))) + '%'; bar.append(fill); card.append(bar);
    }
    return card;
  }));
  const tick = () => { for (const [clock, end] of timers) { const s = Math.max(0, (end - Date.now()) / 1000);
    clock.textContent = `${Math.floor(s / 86400)} gün ${Math.floor(s % 86400 / 3600)} sa ${Math.floor(s % 3600 / 60)} dk ${Math.floor(s % 60)} sn`; } };
  tick(); setInterval(tick, 1000);
})();

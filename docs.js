'use strict';
// Docs: plan tablosu sunucudaki güncel fiyatlardan (sunucuya ulaşılamazsa sabit not kalır)
(async () => {
  const box = document.querySelector('#planTable'); if (!box || !window.kubikTransport) return;
  try {
    const status = await (await kubikTransport.fetch('/api/status')).json(), table = document.createElement('table');
    table.className = 'data-table';
    const row = (cells, tag = 'td') => { const tr = document.createElement('tr'); cells.forEach(text => { const c = document.createElement(tag); c.textContent = text; tr.append(c); }); table.append(tr); };
    row(['Plan', 'Küp / 30 gün', '5 saatte token', 'Haftalık token', '5 saatte mesaj', 'Code'], 'th');
    for (const plan of Object.values(status.tiers).filter(p => p.visible))
      row([plan.name, plan.price ? plan.price.toLocaleString('tr') : 'Ücretsiz', plan.tokens.toLocaleString('tr'), plan.weekly.toLocaleString('tr'), plan.messages ?? 'Sınırsız', plan.code ? 'Var' : '—']);
    box.replaceChildren(table);
  } catch {}
})();

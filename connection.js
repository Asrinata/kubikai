'use strict';
(() => {
  const pages = window.KUBIK_PAGES === true;
  const configured = typeof window.KUBIK_API_BASE === 'string' ? window.KUBIK_API_BASE.replace(/\/$/, '') : '';
  const base = pages ? (configured || localStorage.getItem('kubik.pi') || '') : '';
  // Android uygulamasında oturum, çıkış yapana kadar kalsın (uygulama kapanınca sessionStorage silinir)
  const tokens = /KubikApp/.test(navigator.userAgent) ? localStorage : sessionStorage;
  window.kubikTransport = {
    url(path) { return base + path; },
    async fetch(path, options = {}) {
      if (pages && !base) throw new Error('Önce Pi bağlantısını kaydet.');
      const headers = new Headers(options.headers || {});
      const token = pages ? tokens.getItem('kubik.token') : '';
      if (token) headers.set('Authorization', 'Bearer ' + token);
      return fetch(base + path, {...options, headers});
    },
    setToken(token) { if (pages) tokens.setItem('kubik.token', token); },
    clearToken() { tokens.removeItem('kubik.token'); }
  };
  const setup = document.querySelector('#connectionSetup');
  if (pages && !base && setup) setup.hidden = false;
  if (pages && !configured) {
    for (const target of document.querySelectorAll('#authForm, #adminLogin, .sidebar-bottom, .admin-nav')) {
      const button = document.createElement('button');
      button.type = 'button'; button.className = 'text-button connection-change';
      button.textContent = 'Sunucu bağlantısını değiştir';
      button.onclick = () => { document.querySelector('#apiAddress').value = base; setup.hidden = false; };
      target.append(button);
    }
    if (base) {
      const cancel = document.createElement('button');
      cancel.type = 'button'; cancel.className = 'text-button'; cancel.textContent = 'Vazgeç';
      cancel.onclick = () => { setup.hidden = true; };
      document.querySelector('#connectionForm').append(cancel);
    }
  }
  document.querySelector('#connectionForm')?.addEventListener('submit', event => {
    event.preventDefault();
    const value = document.querySelector('#apiAddress').value.trim().replace(/\/$/, '');
    if (!/^https:\/\/[a-z0-9-]+\.trycloudflare\.com$/i.test(value)) {
      document.querySelector('#connectionError').textContent = 'Geçerli https://…trycloudflare.com Pi adresini gir.';
      return;
    }
    if (localStorage.getItem('kubik.pi') !== value) sessionStorage.removeItem('kubik.token');
    localStorage.setItem('kubik.pi', value);
    location.reload();
  });
})();

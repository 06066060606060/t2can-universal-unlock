(() => {
  if (window.__tuNativeInstalled) return;
  window.__tuNativeInstalled = true;
  let port, awaiting, saving = false;
  const pages = [];
  const originalShowPage = window.showPage;
  if (typeof originalShowPage === 'function') {
    window.showPage = function (name, ...args) {
      const current = document.querySelector('.page.active')?.dataset.page;
      const result = originalShowPage.call(this, name, ...args);
      const actual = document.querySelector('.page.active')?.dataset.page;
      if (current && actual && current !== actual) pages.push(current);
      return result;
    };
  }
  window.__tuNativeBusy = () => (typeof otaUploading !== 'undefined' && !!otaUploading) || saving;
  window.__tuNativeBack = () => {
    if ((typeof otaUploading !== 'undefined' && otaUploading) || saving) return 'busy';
    if (document.querySelector('.warningModal.show')) return 'modal';
    const sheet = document.querySelector('#selectSheetBack.show') || document.querySelector('#sheetback.show');
    if (sheet) { sheet.click(); return 'handled'; }
    const panel = document.querySelector('.panel.show .backbtn');
    if (panel) { panel.click(); return 'handled'; }
    while (pages.length) {
      const previous = pages.pop();
      if (previous !== document.querySelector('.page.active')?.dataset.page) {
        originalShowPage(previous); return 'handled';
      }
    }
    if (document.querySelector('.page.active')?.dataset.page !== 'home' && originalShowPage) {
      originalShowPage('home'); return 'handled';
    }
    return 'exit';
  };
  window.__tuNativeTheme = dark => {
    let mode = 'system';
    try { mode = localStorage.getItem('t2canDashboardTheme') || 'system'; } catch (_) {}
    if (mode !== 'system') return;
    document.documentElement.dataset.theme = dark ? 'dark' : 'light';
  };
  let nativeNetworkConnected = true;
  const dashboardSetConn = window.setConn;
  if (typeof dashboardSetConn === 'function') {
    window.setConn = function (good, ...args) {
      return dashboardSetConn.call(this, nativeNetworkConnected && !!good, ...args);
    };
    window.__tuNativeConnectivity = connected => {
      nativeNetworkConnected = !!connected;
      dashboardSetConn.call(window, nativeNetworkConnected);
    };
  } else {
    window.__tuNativeConnectivity = () => {};
  }
  const request = value => new Promise((resolve, reject) => {
    if (!port) return reject(new Error('File saving is not ready. Try again.'));
    awaiting = { resolve, reject };
    port.postMessage(JSON.stringify(value));
  });
  window.addEventListener('message', event => {
    if (event.data !== 'T2CAN_NATIVE_PORT' || !event.ports?.[0]) return;
    if (port) port.close();
    port = event.ports[0];
    port.onmessage = event => {
      const reply = JSON.parse(event.data);
      const pending = awaiting;
      awaiting = null;
      if (pending) reply.ok ? pending.resolve(reply) : pending.reject(new Error(reply.error || 'Save cancelled'));
    };
    port.start();
  });
  // Firmware already fetched the log. Transfer that exact Blob in acknowledged
  // chunks instead of making a second request or passing one huge base64 string.
  window.saveDownloadedBlob = async (blob, name) => {
    if (saving) { alert('A file is already being saved.'); return; }
    saving = true;
    try {
      await request({ type: 'begin', name, size: blob.size });
      for (let offset = 0; offset < blob.size; offset += 49152) {
        const bytes = new Uint8Array(await blob.slice(offset, offset + 49152).arrayBuffer());
        let binary = '';
        for (const byte of bytes) binary += String.fromCharCode(byte);
        await request({ type: 'chunk', data: btoa(binary) });
      }
      await request({ type: 'end' });
    } catch (error) {
      if (port) port.postMessage(JSON.stringify({ type: 'abort' }));
      if (error.message !== 'Save cancelled') alert('Could not save file: ' + error.message);
    } finally { saving = false; }
  };
})();

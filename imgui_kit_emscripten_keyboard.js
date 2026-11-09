// imgui_kit: the phone's virtual keyboard in a browser: the page's side (the hidden field, the keyboard button).
// https://github.com/pthom/imgui_kit (MIT license)
// Embedded into imgui_kit_emscripten_keyboard.cpp by tools/embed_js.py: edit this file, then run the script.

if (!window.imguiKitKeyboard) {
  const K = window.imguiKitKeyboard = {want: 0, id: 0, dismissed: 0, x: 0, y: 0, h: 0, displayW: 0, mirror: '',
                                         mirrorCaret: 0, edits: [], caret: -1, keys: [],
                                         keysMode: false, actions: [], lastMove: null};
  const input = document.createElement('textarea');
  input.id = 'imguiKitTextInput'; input.autocapitalize = 'off'; input.autocomplete = 'off';
  input.setAttribute('autocorrect', 'on'); input.spellcheck = false; input.rows = 1;
  // Invisible, but laid out like the text ImGui draws (the widget's width, its line height): the caret moved by the
  // keyboard's trackpad travels pixels, in this layout; a 2 px wide field wrapped one character per line
  // Invisible through transparent colors, not opacity, and hit-testable while focused: iOS enters the keyboard's
  // trackpad mode by hit-testing the text under the caret, and a field it cannot hit sends the caret to the start.
  // Without the focus, the field takes no pointer events: the touches on the widget's line are ImGui's.
  input.style.cssText = 'position:fixed;left:0;top:0;width:200px;height:20px;opacity:1;color:transparent;'
    + 'background:transparent;caret-color:transparent;outline:none;border:0;padding:0;margin:0;font-size:16px;'
    + 'line-height:20px;z-index:1000;resize:none;white-space:pre;overflow:hidden;font-family:sans-serif;'
    + 'pointer-events:none;-webkit-text-fill-color:transparent;';
  const button = document.createElement('button');
  button.id = 'imguiKitKeyboardButton'; button.textContent = '⌨'; button.title = 'Keyboard';
  button.style.cssText = 'position:fixed;display:none;z-index:1001;font-size:24px;line-height:1;padding:6px 12px;'
    + 'border-radius:10px;border:1px solid #999;background:#333;color:#eee;';
  document.body.appendChild(input); document.body.appendChild(button);
  // A debug overlay on the page (the url with 'kbddebug'): the field's events, to see what a phone sends. Only in a
  // build with IMGUI_KIT_EMSCRIPTEN_KEYBOARD_DEBUG (window.imguiKitKeyboardDebugBuild, set by the C++ before this
  // script runs): the log holds what is typed, and a link could turn it on.
  const debugBuild = !!window.imguiKitKeyboardDebugBuild;
  const debug = debugBuild && (location.search.includes('kbddebug') || location.hash.includes('kbddebug'));
  let logEl = null;
  if (debug) {
    logEl = document.createElement('div');
    logEl.style.cssText = 'position:fixed;left:0;bottom:0;width:100%;max-height:45%;overflow:auto;'
      + 'background:rgba(0,0,0,0.8);color:#8f8;font:12px monospace;z-index:1002;padding:4px;white-space:pre-wrap;';
    document.body.appendChild(logEl);
    logEl.addEventListener('touchstart', (e) => { logEl.innerText = ''; e.stopPropagation(); }, {capture: true});
  }
  // The log also in the page's server output, as 404 requests "/kbdlog?<line>" (the url with 'kbddebug', or 'kbdlog'
  // without the overlay): to follow a phone from the computer that serves the page
  const serverLog = debug || (debugBuild && location.search.includes('kbdlog'));
  const log = (m) => {
    if (!logEl && !serverLog) return;
    m = new Date().toISOString().slice(14, 23) + ' ' + m;
    if (logEl) logEl.innerText = m + '\n' + logEl.innerText;
    if (serverLog) fetch('/kbdlog?' + encodeURIComponent(m)).catch(() => {}); };
  const canvasRect = () => {
    const c = document.getElementById('canvas'); return c ? c.getBoundingClientRect() : {left: 0, top: 0, width: 0}; };
  const scale = (r) => (K.displayW > 0 && r.width > 0) ? r.width / K.displayW : 1;
  // The field's place: ImGui gives the caret's position, which moves as the caret moves. While the field has the
  // focus, it stays where it was (the keyboard's trackpad hit-tests the finger against the field: a field that
  // moves with the caret moved the caret back and forth)
  // A multiline text: the field holds all its lines (the trackpad reaches the lines above the caret), its top
  // so that the caret's line is at ImGui's caret
  // Keys mode: a monospaced field, its caret over the widget's caret, so that a tap on the field moves the caret by
  // about as many characters in the widget
  let measureCtx = null;
  const charWidth = (fontPx) => {
    measureCtx = measureCtx || document.createElement('canvas').getContext('2d');
    measureCtx.font = fontPx + 'px monospace';
    return measureCtx.measureText(' ').width; };
  K.frozen = null;
  // The caret's line inside the visible area: a field under the keyboard may not be hittable (the trackpad).
  // Returns the field's top, for a caret's line `caretRows` lines below it.
  const clampToVisible = (y, caretRows, h) => {
    const v = window.visualViewport;
    if (!v || document.activeElement !== input) return y;
    const caretTop = y + caretRows * h, minTop = v.offsetTop, maxTop = v.offsetTop + v.height - h;
    return (caretTop > maxTop) ? y - (caretTop - maxTop) : (caretTop < minTop) ? y + (minTop - caretTop) : y; };
  // Keys mode: a box of one line over the widget's caret line, as wide as the canvas. The field's text (blanks) is
  // wider than the box, room for the trackpad; it is scrolled so that its caret is over the widget's caret. During the
  // trackpad's moves, the box stays put (a field that moves under the finger moves the caret); after typing, it
  // follows the widget's caret.
  K.moving = false;  // keys mode: the caret moved in the field since the last typing (the trackpad, maybe still on)
  // The keyboard button: under the field's line, or above it when the screen ends there
  const buttonTop = (lineTop, h) => (lineTop + h + 8 + 48 > window.innerHeight) ? lineTop - 48 : lineTop + h + 8;
  const placeKeysMode = (r, s, h, fontPx, col) => {
    const focused = document.activeElement === input;
    if (!(focused && K.frozen && K.moving)) {
      const y = clampToVisible(r.top + K.y * s, 0, h);
      K.frozen = {x: r.left, y: y};
      input.style.left = r.left + 'px'; input.style.top = y + 'px';
      input.style.width = r.width + 'px'; input.style.height = h + 'px';
      input.style.lineHeight = h + 'px'; input.style.fontSize = fontPx + 'px'; input.style.fontFamily = 'monospace';
      input.scrollLeft = col * charWidth(fontPx) - K.x * s; }
    button.style.left = (r.left + K.x * s) + 'px'; button.style.top = buttonTop(r.top + K.y * s, h) + 'px'; };
  const place = () => {
    const r = canvasRect(), s = scale(r), h = Math.max(20, K.h * s), fontPx = Math.max(16, h * 0.75);
    const before = K.mirror.slice(0, K.mirrorCaret), caretLine = before.split('\n').length - 1;
    if (K.keysMode) {
      placeKeysMode(r, s, h, fontPx, [...before.slice(before.lastIndexOf('\n') + 1)].length);
      return; }
    const lines = K.mirror.split('\n').length;
    let x = r.left + K.x * s, y = r.top + K.y * s - caretLine * h;
    if (document.activeElement === input && K.frozen) { x = K.frozen.x; y = K.frozen.y; }
    else {
      y = clampToVisible(y, caretLine, h);
      K.frozen = {x: x, y: y}; }
    input.style.left = x + 'px'; input.style.top = y + 'px';
    input.style.width = Math.max(120, r.left + r.width - x - 8) + 'px';
    input.style.height = (lines * h) + 'px';
    input.style.lineHeight = h + 'px';
    input.style.fontSize = fontPx + 'px';
    input.style.fontFamily = 'sans-serif';
    button.style.left = x + 'px'; button.style.top = buttonTop(y, h) + 'px'; };
  const refresh = () => {
    const focused = document.activeElement === input;
    input.style.pointerEvents = focused ? 'auto' : 'none';
    place();
    // No button for a widget whose keyboard the user hid: a tap on its line still opens the keyboard
    const showButton = K.want && !focused && K.id !== K.dismissed && window.imguiKitPointerType === 1;
    button.style.display = showButton ? 'block' : 'none';
    if (!K.want && focused) input.blur(); };
  K.set = (want, id, x, y, h, displayW, keysMode) => {
    keysMode = !!keysMode;
    const newWidget = want && (!K.want || id !== K.id || keysMode !== K.keysMode);
    if (newWidget || !want !== !K.want) log('set: want ' + want + ' id ' + id + (keysMode ? ' keys mode' : ''));
    if (K.keysMode && !keysMode) input.scrollLeft = 0;  // the mirror shows its text from its start
    K.want = want; K.id = id; K.x = x; K.y = y; K.h = h; K.displayW = displayW; K.keysMode = keysMode;
    if (!want) K.dismissed = 0;
    if (newWidget && K.keysMode) K.pad();
    refresh(); };
  // The mirror of ImGui's text and caret (UTF-16 units): the field follows, without events of its own
  K.syncing = false;
  K.mirrorTo = (text, caret) => {
    log((K.keysMode ? 'pad' : 'mirror: ' + JSON.stringify(text)) + ' caret ' + caret
        + ' (field had ' + input.selectionStart + ')');
    K.syncing = true;
    if (input.value !== text) input.value = text;
    if (input.selectionStart !== caret || input.selectionEnd !== caret) {
      try { input.setSelectionRange(caret, caret); } catch (e) {} }
    K.mirror = text; K.mirrorCaret = caret;
    K.syncing = false;
    place(); };
  // Keys mode: the field padded with blanks, its caret in the middle. What was typed before is dropped: the keyboard's
  // predictions start over.
  // keepPlace: the caret keeps its place on screen, the text scrolls instead (blanks everywhere: nothing changes under
  // a finger that may still be on the trackpad).
  const kPadCols = 150;  // wider than a tablet: the text scrolls under its caret
  K.pad = (keepPlace) => {
    const text = ' '.repeat(2 * kPadCols), caret = kPadCols;
    if (keepPlace && K.frozen && document.activeElement === input) {
      const cw = charWidth(parseFloat(input.style.fontSize) || 16);
      const dx = lineCol(K.mirror, K.mirrorCaret).col * cw - input.scrollLeft;
      K.mirrorTo(text, caret);
      input.scrollLeft = kPadCols * cw - dx;
      return; }
    K.frozen = null;
    K.moving = false;
    K.mirrorTo(text, caret); };
  // The line and the column (in characters) of a position (in UTF-16 units)
  const lineCol = (text, pos) => {
    const before = text.slice(0, pos), start = before.lastIndexOf('\n') + 1;
    return {line: before.split('\n').length - 1, col: [...before.slice(start)].length}; };
  // Arrow keys for a caret moved in the field. kind: 'K:' (a part of an edit), or 'M:' (a move of the caret alone,
  // which may be dropped)
  const pushMove = (text, from, to, kind = 'K:') => {
    const a = lineCol(text, from), b = lineCol(text, to);
    const vertical = kind + (b.line < a.line ? 'ArrowUp' : 'ArrowDown');
    const horizontal = kind + (b.col < a.col ? 'ArrowLeft' : 'ArrowRight');
    for (let i = 0; i < Math.abs(b.line - a.line); i++) K.actions.push(vertical);
    for (let i = 0; i < Math.abs(b.col - a.col); i++) K.actions.push(horizontal); };
  // The trackpad, and iOS's repeat while the finger holds the field's edge (20 characters a second), move the caret
  // faster than ImGui takes the arrows (one in two frames): the moves waiting are capped, so that the widget's caret
  // stops with the finger. (Every action is relative: a dropped move only shifts the field's caret against the
  // widget's.)
  const kMaxWaitingMoves = 2;
  const capMoves = () => {
    let n = 0;
    while (n < K.actions.length && K.actions[K.actions.length - 1 - n].startsWith('M:')) n++;
    if (n > kMaxWaitingMoves) K.actions.splice(K.actions.length - n, n - kMaxWaitingMoves); };
  // Text as keys: a newline (a paste) is Enter
  const pushText = (text) => {
    text.split('\n').forEach((part, i) => {
      if (i > 0) K.actions.push('K:Enter');
      if (part) K.actions.push('T:' + part); }); };
  // Once the field rests, it is padded again if its caret nears an edge
  let roomTimer = null;
  const checkRoomLater = () => {
    if (roomTimer) clearTimeout(roomTimer);
    roomTimer = setTimeout(() => {
      roomTimer = null;
      if (!K.keysMode || document.activeElement !== input) return;
      const lines = K.mirror.split('\n'), p = lineCol(K.mirror, K.mirrorCaret);
      // (several lines: a multi-line text was pasted)
      if (lines.length > 1 || p.col < 12 || [...lines[p.line]].length - p.col < 12) K.pad(true);
    }, 700); };
  const focusNow = (e) => {
    log('focus from the button (' + e.type + ')');
    e.preventDefault(); e.stopPropagation(); input.focus(); refresh(); };
  button.addEventListener('touchend', focusNow);
  button.addEventListener('mouseup', focusNow);
  button.addEventListener('click', (e) => e.preventDefault());
  // The button's touches are not ImGui's (the backends listen on the window: ImGui would click under the button)
  button.addEventListener('touchstart', (e) => e.stopPropagation());
  button.addEventListener('mousedown', (e) => e.stopPropagation());
  // A tap that opened the keyboard is the keyboard's: ImGui would get it a few frames later (the touch layer replays
  // it), once the app shrank above the keyboard, maybe on nothing (a click on nothing takes the focus from every
  // window). C++ drops the clicks meanwhile.
  K.tapOpenedAt = -1e9;
  K.dropTapClicks = () => (performance.now() - K.tapOpenedAt < 600) ? 1 : 0;
  // The second tap on the active widget's line (ImGui gives the line, not the widget's width)
  document.addEventListener('touchend', (e) => {
    if (!K.want || document.activeElement === input || e.target === button || !e.changedTouches.length) return;
    const r = canvasRect(), s = scale(r), top = r.top + K.y * s, h = K.h * s, y = e.changedTouches[0].clientY;
    log('touchend at y ' + Math.round(y) + ', the line at ' + Math.round(top));
    if (y >= top - h && y <= top + 2 * h) {
      log('focus from a tap on the line');
      K.tapOpenedAt = performance.now(); input.focus(); refresh(); }
  }, {capture: true});
  // Typing: the field's new value against the mirror: a removal at the caret, an insertion
  input.addEventListener('input', (e) => {
    log('input: ' + (e.inputType || '?') + (K.keysMode ? '' : ' value ' + JSON.stringify(input.value))
        + ' sel ' + input.selectionStart + '-' + input.selectionEnd + (K.syncing ? ' (syncing)' : ''));
    if (K.syncing) return;
    K.focusReset = false;
    const o = K.mirror, n = input.value;
    if (K.keysMode) {
      K.moving = false;  // typing: the finger left the trackpad, the box may follow the widget's caret again
      // The edit is anchored at the caret (in a run of blanks, a diff alone cannot tell where a blank was typed): it
      // starts before the old and the new caret, it ends after them
      const c0 = K.mirrorCaret, c1 = input.selectionStart;
      let p = 0; while (p < Math.min(c0, c1) && o[p] === n[p]) p++;
      let s = 0;
      while (s < o.length - Math.max(p, c0) && s < n.length - Math.max(p, c1)
             && o[o.length - 1 - s] === n[n.length - 1 - s])
        s++;
      const removed = o.slice(p, o.length - s), inserted = n.slice(p, n.length - s);
      log('edit: removed ' + JSON.stringify(removed) + ' inserted ' + JSON.stringify(inserted)
          + ' caret ' + c0 + ' -> ' + c1);
      K.mirror = n; K.mirrorCaret = c1;
      checkRoomLater();
      // Smart spacing, once an accepted suggestion added its own blank: iOS and Gboard delete the blank after the caret
      // (a Backspace deletes before it), or Gboard moves the caret over it, then deletes it backward (a few
      // milliseconds apart: not a user's Backspace). A blank of the padding, which the widget does not have: the
      // deletion is not sent, nor Gboard's move.
      const blanks = /^ +$/.test(removed) && !inserted;
      if (blanks && p >= c0) return;
      const m = K.lastMove;
      if (blanks && e.inputType === 'deleteContentBackward' && m && performance.now() - m.at < 50 && m.to === c0
          && m.cols === [...removed].length) {
        let undo = m.cols;
        while (undo > 0 && K.actions.length && K.actions[K.actions.length - 1] === 'M:ArrowRight') {
          K.actions.pop();
          undo--; }
        for (; undo > 0; undo--) K.actions.push('K:ArrowLeft');
        K.lastMove = null;
        return; }
      // The caret to the end of the removal, Backspaces, the text, then the caret to where the field has it
      pushMove(o, c0, p + removed.length);
      for (let i = 0; i < [...removed].length; i++) K.actions.push('K:Backspace');
      pushText(inserted);
      pushMove(n, p + inserted.length, c1);
      return; }
    let p = 0; while (p < o.length && p < n.length && o[p] === n[p]) p++;
    let s = 0; while (s < o.length - p && s < n.length - p && o[o.length - 1 - s] === n[n.length - 1 - s]) s++;
    K.edits.push({at: p, removed: o.length - p - s, inserted: n.slice(p, n.length - s)});
    K.mirror = n; K.mirrorCaret = input.selectionStart; });
  // The caret moved by itself (the keyboard's trackpad, a tap in the field)
  // On a focus, iOS puts the field's caret at zero, and reports it as a selection change a moment later: the
  // mirror's caret is put back, and that report is not a move of the user's
  K.focusReset = false;
  const restoreCaret = () => {
    K.syncing = true;
    try { input.setSelectionRange(K.mirrorCaret, K.mirrorCaret); } catch (e) {}
    K.syncing = false;
    const took = input.selectionStart === K.mirrorCaret;
    log('restore caret ' + K.mirrorCaret + (took ? ' took' : ' ignored (field at ' + input.selectionStart + ')'));
    return took; };
  // iOS ignores a setSelectionRange made right after a programmatic focus: retried until it takes
  let settleTimer = null;
  const settleCaret = () => {
    if (settleTimer) clearInterval(settleTimer);
    let tries = 0;
    settleTimer = setInterval(() => {
      tries++;
      if (document.activeElement !== input || restoreCaret() || tries > 20) {
        clearInterval(settleTimer); settleTimer = null; K.focusReset = false; }
    }, 30); };
  document.addEventListener('selectionchange', () => {
    log('selectionchange: sel ' + input.selectionStart + '-' + input.selectionEnd + ' mirrorCaret ' + K.mirrorCaret
        + (document.activeElement === input ? '' : ' (not focused)') + (K.syncing ? ' (syncing)' : '')
        + (K.focusReset ? ' (after focus)' : ''));
    if (K.syncing || document.activeElement !== input) return;
    if (settleTimer || K.focusReset) return;  // the field settling after a focus: not a move of the user's
    if (input.selectionStart !== K.mirrorCaret && input.selectionStart === input.selectionEnd) {
      if (K.keysMode) {
        const a = lineCol(K.mirror, K.mirrorCaret), b = lineCol(K.mirror, input.selectionStart);
        log('move: ' + JSON.stringify(a) + ' -> ' + JSON.stringify(b));
        K.moving = true;
        // (a move to the right, on the line: Gboard's smart spacing may undo it, see the input handler)
        const right = a.line === b.line && b.col > a.col;
        K.lastMove = right ? {at: performance.now(), to: input.selectionStart, cols: b.col - a.col} : null;
        pushMove(K.mirror, K.mirrorCaret, input.selectionStart, 'M:');
        capMoves();
        K.mirrorCaret = input.selectionStart;
        checkRoomLater();
        return; }
      K.mirrorCaret = input.selectionStart; K.caret = input.selectionStart; } });
  input.addEventListener('focus', () => {
    log('focus: value ' + JSON.stringify(input.value) + ' sel ' + input.selectionStart);
    K.focusReset = true;
    settleCaret(); });
  input.addEventListener('blur', (e) => {
    const to = e.relatedTarget ? ', the focus to ' + (e.relatedTarget.id || e.relatedTarget.tagName) : '';
    log('blur' + to + (K.want ? '' : ' (ImGui wants no text)')); });
  input.addEventListener('keydown', (e) => log('keydown: ' + e.key));
  input.addEventListener('compositionstart', () => log('compositionstart'));
  input.addEventListener('compositionend', (e) => log('compositionend: ' + JSON.stringify(e.data)));
  input.addEventListener('keydown', (e) => {
    const isKey = e.key === 'Enter' || e.key === 'Tab' || e.key === 'Escape';
    // Keys mode: also the arrows (a hardware keyboard, the Android keyboards that send them); after Enter, a new line
    if (K.keysMode && (isKey || e.key.startsWith('Arrow'))) {
      K.actions.push('K:' + e.key); e.preventDefault();
      if (e.key === 'Enter') K.pad(); }
    else if (isKey) { K.keys.push(e.key); e.preventDefault(); }
    e.stopPropagation(); });
  input.addEventListener('keyup', (e) => e.stopPropagation());
  input.addEventListener('keypress', (e) => e.stopPropagation());
  input.addEventListener('focus', refresh);
  // The keyboard covering the page. iOS scrolls the page to show the focused field: a fixed field, which no scroll
  // reveals, so the canvas would leave the screen. The page stays in place, and the kit learns which part of the
  // canvas is visible, in ImGui units from its top (its full-screen window shrinks to it). The canvas element's own
  // height is no guide: it may follow the keyboard while ImGui's display keeps its height.
  const vv = window.visualViewport;
  K.visible = '';
  const visibleArea = () => {
    if (document.activeElement !== input || !vv) return '';
    const r = canvasRect(), s = scale(r);
    return Math.round((vv.offsetTop - r.top) / s) + ',' + Math.round((vv.offsetTop + vv.height - r.top) / s); };
  const updateVisibleArea = () => {
    if (document.activeElement === input && window.scrollY) window.scrollTo(0, 0);
    const visible = visibleArea();
    // The visible area changed (the keyboard shown or hidden): the field moves into it once
    if (visible !== K.visible) K.frozen = null;
    K.visible = visible;
    log('viewport: scrollY ' + Math.round(window.scrollY) + ', visible in ImGui units: ' + visible);
    place(); };
  if (vv) { vv.addEventListener('resize', updateVisibleArea); vv.addEventListener('scroll', updateVisibleArea); }
  window.addEventListener('scroll', updateVisibleArea);
  input.addEventListener('focus', updateVisibleArea);
  input.addEventListener('blur', updateVisibleArea);
  // For C++: "top,bottom" of the visible area, while the keyboard is shown; '' otherwise
  K.visibleArea = () => visibleArea();
  // While the keyboard is open, a tap on the canvas leaves the focus to the field, so that the keyboard stays open as
  // in a native app: the GLFW port (emscripten-glfw) focuses the canvas on each press, which closed the keyboard. The
  // press still reaches ImGui; after a tap outside the text widget, the field blurs itself once ImGui stops wanting
  // text. The keyboard counts as open when it hides a fifth of the window: not iPadOS's bar once its keyboard is
  // hidden. After iPadOS's hide key or Android's back key, the field keeps the focus without a keyboard: the next
  // touch takes it
  // (the touchstart handler below).
  const keyboardOpen = () => !!vv && vv.height < 0.8 * window.innerHeight;
  const canvasEl = document.getElementById('canvas');
  if (canvasEl) {
    const focusCanvas = canvasEl.focus;
    canvasEl.focus = function(options) {
      if (K.want && document.activeElement === input && keyboardOpen()) {
        log('canvas focus skipped: the keyboard is open');
        return; }
      focusCanvas.call(this, options); };
  }
  // A blur while ImGui still wants text is the user's (the keyboard hidden, a touch elsewhere): the field blurs itself
  // only once ImGui stopped wanting text
  // Keys mode: the next keyboard session starts with blanks (the widget's caret may move meanwhile)
  // Keys mode: a blur during a touch on the page is a touch elsewhere, not a hidden keyboard (iPadOS's key that hides
  // the keyboard, and Android's back key, leave the field focused: the next touch blurs it): the button comes back at
  // the widget's next caret (reset after refresh(), which would show it at the old one)
  K.touchedAt = -1e9;
  input.addEventListener('blur', () => {
    if (K.want) K.dismissed = K.id;
    if (K.keysMode) K.pad();
    refresh();
    if (K.keysMode && performance.now() - K.touchedAt < 500) K.dismissed = 0; });
  // A touch on the page while the field has the focus without a keyboard (hidden by iPadOS's key or Android's back key)
  // takes the focus from the field, as the canvas did: the GLFW port no longer focuses the canvas once a focus was
  // skipped above (it counts its window as focused from then on).
  // Keys mode: a touch elsewhere on the page (the widget's caret placed by a tap) makes what the field holds obsolete;
  // without the keyboard, it brings the button back (the widget is still the same, but the user chose another place)
  document.addEventListener('touchstart', (e) => {
    if (e.target === input || e.target === button) return;
    K.touchedAt = performance.now();
    if (document.activeElement === input && !keyboardOpen()) input.blur();
    if (!K.keysMode) return;
    if (document.activeElement === input) K.pad();
    else K.dismissed = 0;
  }, {capture: true, passive: true});
  // The frame's state, in one call: "top,bottom" (the visible area, empty when the keyboard is hidden), "|", 1 to drop
  // the tap's clicks, "|", 1 when edits, a caret or keys wait
  K.poll = () => visibleArea() + '|' + K.dropTapClicks() + '|'
                 + ((K.edits.length || K.caret >= 0 || K.keys.length) ? 1 : 0);
  // The frame drains, in one call: the caret, "\n", the keys, "\n", the edits as "at|removed|inserted" lines
  K.drainAll = () => { const c = K.drainCaret(), k = K.drainKeys(); return c + '\n' + k + '\n' + K.drainEdits(); };
  K.drainEdits = () => {
    const e = K.edits.map(x => x.at + '|' + x.removed + '|' + x.inserted).join('\n'); K.edits = []; return e; };
  K.drainCaret = () => { const c = K.caret; K.caret = -1; if (c >= 0) log('caret -> ImGui: ' + c); return c; };
  K.drainKeys = () => { const k = K.keys.join(','); K.keys = []; return k; };
  // Keys mode: the next action, "K:<key name>", "M:<key name>" (a move) or "T:<text>"
  K.drainAction = () => K.actions.length ? K.actions.shift() : '';
}

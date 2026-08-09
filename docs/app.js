/* ===========================================================================
   Blackjack RL — policy inspector

   This file renders. It does not decide anything: every card, every action and
   every Q-value comes from the C++ server. The only blackjack knowledge held
   here is the published basic-strategy table, used purely as a reference
   overlay to compare the agent against.
   =========================================================================== */

'use strict';

const SUIT_GLYPH = { hearts: '♥', diamonds: '♦', clubs: '♣', spades: '♠' };
const SUMS_HARD = [21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4];
const SUMS_SOFT = [21, 20, 19, 18, 17, 16, 15, 14, 13, 12];
const UPCARDS = [2, 3, 4, 5, 6, 7, 8, 9, 10, 11];

const state = {
  agent: 'q',
  decider: 'agent',
  handType: 'hard',
  showBasic: false,
  hand: null,
  autoplay: false,
  autoTimer: null,
  tally: { hands: 0, wins: 0, losses: 0, pushes: 0, net: 0 },
  agree: { total: 0, same: 0 },
  policyCells: null,
  baselineEV: null,   // basic strategy's measured EV, the target to beat
  training: { cursor: 0, points: [], poll: null },
};

const $ = (id) => document.getElementById(id);

/* ----------------------------------------------------------- transport --
   Two ways to reach the same C++ code:

     http  — fetch() against bin/blackjack_server, for local development.
     wasm  — the identical api::handle compiled to WebAssembly, called
             in-process. This is what the hosted static build uses, so there is
             no backend to run and each visitor gets their own private agent.

   Everything below this block is written against api() and never needs to know
   which one is in play. */

const transport = { mode: 'http', module: null };

// Episodes per synchronous slice in WASM mode. Small enough that the browser
// keeps painting between slices, large enough that the call overhead vanishes.
const WASM_STEP_EPISODES = 3000;

async function initTransport() {
  if (!window.BLACKJACK_WASM || typeof BlackjackModule !== 'function') {
    return 'http';
  }
  transport.module = await BlackjackModule();
  transport.module.ccall('api_init', null, [], []);
  transport.mode = 'wasm';
  return 'wasm';
}

function splitPath(path) {
  const i = path.indexOf('?');
  return i < 0 ? [path, ''] : [path.slice(0, i), path.slice(i + 1)];
}

// Returns { status, body } with body as an unparsed string.
function wasmCall(path) {
  const [p, q] = splitPath(path);
  const raw = transport.module.ccall('api_request', 'string', ['string', 'string'], [p, q]);
  return JSON.parse(raw);
}

async function api(path, opts = {}) {
  if (transport.mode === 'wasm') {
    const env = wasmCall(path);
    let body;
    try {
      body = JSON.parse(env.body);
    } catch {
      body = { error: 'malformed response' };
    }
    if (env.status >= 400) throw new Error(body.error || `request failed (${env.status})`);
    return body;
  }

  const res = await fetch(path, opts);
  const body = await res.json().catch(() => ({ error: 'malformed response' }));
  if (!res.ok) throw new Error(body.error || `request failed (${res.status})`);
  return body;
}

// For endpoints that return something other than JSON (the CSV export).
async function apiText(path) {
  if (transport.mode === 'wasm') {
    const env = wasmCall(path);
    if (env.status >= 400) throw new Error(`request failed (${env.status})`);
    return env.body;
  }
  const res = await fetch(path);
  if (!res.ok) throw new Error(`request failed (${res.status})`);
  return res.text();
}

const post = (path) => api(path, { method: 'POST' });

function setEngine(live, text) {
  const dot = $('engine-dot');
  dot.classList.toggle('is-live', live);
  dot.classList.toggle('is-down', !live);
  $('engine-state').textContent = text;
}

function downloadText(filename, text, type) {
  const url = URL.createObjectURL(new Blob([text], { type }));
  const a = document.createElement('a');
  a.href = url;
  a.download = filename;
  document.body.appendChild(a);
  a.click();
  a.remove();
  URL.revokeObjectURL(url);
}

/* Basic strategy is not defined here. It arrives on every policy cell as
   `cell.basic`, computed by rules::basicStrategy in C++, so the overlay and the
   baseline simulation can never disagree. */

/* -------------------------------------------------------------- tooltip -- */

const tooltip = $('tooltip');

function showTip(html, evt) {
  tooltip.innerHTML = html;
  tooltip.hidden = false;
  const pad = 14;
  const r = tooltip.getBoundingClientRect();
  let x = evt.clientX + pad;
  let y = evt.clientY + pad;
  if (x + r.width > window.innerWidth - 8) x = evt.clientX - r.width - pad;
  if (y + r.height > window.innerHeight - 8) y = evt.clientY - r.height - pad;
  tooltip.style.left = `${Math.max(8, x)}px`;
  tooltip.style.top = `${Math.max(8, y)}px`;
}

const hideTip = () => { tooltip.hidden = true; };

/* ------------------------------------------------------------- readouts -- */

async function refreshStatus() {
  try {
    const s = await api('/api/status');
    const a = s[state.agent];
    $('rd-episodes').textContent = a.episodes.toLocaleString();
    $('rd-states').textContent = `${a.statesLearned} / ${a.statesPossible}`;
    $('rd-epsilon').textContent = a.epsilon.toFixed(4);
    $('rd-table').textContent = a.tableLoaded ? 'loaded' : 'untrained';
    setEngine(true, transport.mode === 'wasm' ? 'running in your browser' : 'engine live');
    return s;
  } catch (err) {
    setEngine(false, 'engine unreachable');
    throw err;
  }
}

/* ============================================================== PLAY ===== */

function cardEl(card) {
  const glyph = SUIT_GLYPH[card.suit] || '?';
  const red = card.suit === 'hearts' || card.suit === 'diamonds';

  const el = document.createElement('div');
  el.className = `pcard${red ? ' is-red' : ''}`;
  el.setAttribute('role', 'img');
  el.setAttribute('aria-label', `${card.rank} of ${card.suit}`);
  const corner = `<span class="pcard-corner">${card.rank}<em>${glyph}</em></span>`;
  el.innerHTML =
    corner +
    `<span class="pcard-pip" aria-hidden="true">${glyph}</span>` +
    corner.replace('pcard-corner', 'pcard-corner pcard-corner-br');
  return el;
}

function hiddenCardEl() {
  const el = document.createElement('div');
  el.className = 'pcard pcard-hidden';
  el.setAttribute('role', 'img');
  el.setAttribute('aria-label', 'face-down card');
  return el;
}

function renderHand(container, cards, hidden = 0) {
  container.replaceChildren();
  cards.forEach((c) => container.appendChild(cardEl(c)));
  for (let i = 0; i < hidden; i++) container.appendChild(hiddenCardEl());
}

function renderHandState(h) {
  state.hand = h;

  renderHand($('dealer-hand'), h.dealerHand, h.dealerHiddenCards);
  renderHand($('player-hand'), h.playerHand);
  $('player-total').textContent = h.playerValue;
  $('dealer-total').textContent = h.finished ? h.dealerValue : `${h.state.dealerUpcard} + ?`;
  $('dealer-note').textContent = h.finished
    ? (h.dealerBusted ? 'Dealer busts.' : 'Dealer stands.')
    : 'The agent conditions on the upcard only.';

  // The state tuple, exactly as the C++ side encodes it.
  $('tv-sum').textContent = h.state.playerSum;
  $('tv-up').textContent = h.state.dealerUpcard;
  $('tv-ace').textContent = h.state.usableAce ? 'T' : 'F';

  const p = h.policy;
  const best = p.action;
  const scale = Math.max(0.35, Math.abs(p.qHit), Math.abs(p.qStand));
  setQBar('hit', p.qHit, scale, best === 'hit');
  setQBar('stand', p.qStand, scale, best === 'stand');

  $('fallback-flag').hidden = p.learned;

  if (h.finished) {
    $('verdict').innerHTML = `Hand settled: <b>${outcomeWord(h)}</b>`;
    $('outcome-note').textContent = settleNote(h);
  } else {
    const cls = best === 'hit' ? 'v-hit' : 'v-stand';
    const margin = Math.abs(p.margin).toFixed(3);
    $('verdict').innerHTML =
      `Agent would <b class="${cls}">${best}</b> <span style="color:var(--ink-3)">· margin ${margin}</span>`;
    $('outcome-note').textContent = h.canAct ? 'Hand in progress.' : 'No further action possible — stand to settle.';
  }

  const acting = !h.finished;
  $('btn-step').disabled = !acting;
  $('btn-hit').disabled = !h.canAct;
  $('btn-stand').disabled = !acting;
}

function setQBar(action, value, scale, isBest) {
  const bar = document.querySelector(`.qbar[data-action="${action}"]`);
  const fill = $(`qfill-${action}`);
  const frac = Math.max(-1, Math.min(1, value / scale)) * 50;
  if (frac >= 0) {
    fill.style.left = '50%';
    fill.style.width = `${frac}%`;
  } else {
    fill.style.left = `${50 + frac}%`;
    fill.style.width = `${-frac}%`;
  }
  $(`qval-${action}`).textContent = value.toFixed(3);
  bar.classList.toggle('is-best', isBest);
}

function outcomeWord(h) {
  return { blackjack: 'blackjack', win: 'win', loss: 'loss', push: 'push', bust: 'bust' }[h.outcome] || h.outcome;
}

function settleNote(h) {
  const r = h.reward.toFixed(2);
  if (h.outcome === 'bust') return `Agent busted at ${h.playerValue}. Reward ${r}.`;
  if (h.outcome === 'blackjack') return `Natural blackjack. Reward ${r}.`;
  if (h.outcome === 'push') return `Both on ${h.playerValue}. Reward ${r}.`;
  return `${h.playerValue} against ${h.dealerValue}. Reward ${r}.`;
}

function recordResult(h) {
  const t = state.tally;
  t.hands++;
  t.net += h.reward;
  if (h.reward > 0) t.wins++;
  else if (h.reward < 0) t.losses++;
  else t.pushes++;

  $('t-hands').textContent = t.hands;
  $('t-wins').textContent = t.wins;
  $('t-losses').textContent = t.losses;
  $('t-pushes').textContent = t.pushes;
  $('t-net').textContent = t.net.toFixed(2);
}

async function deal() {
  try {
    const h = await api(`/api/hand/new?agent=${state.agent}`, { method: 'POST' });
    renderHandState(h);
    $('outcome-note').textContent = 'Fresh hand.';
  } catch (err) {
    setEngine(false, err.message);
  }
}

async function step(action) {
  if (!state.hand || state.hand.finished) return;

  // In "You" mode, record whether the human matched the learned policy before
  // the action is applied and the state moves on.
  if (state.decider === 'human' && action !== 'auto') {
    const rec = state.hand.policy.action;
    state.agree.total++;
    if (rec === action) state.agree.same++;
    const pct = Math.round((state.agree.same / state.agree.total) * 100);
    const note = $('agree-note');
    note.hidden = false;
    note.textContent =
      `${rec === action ? 'Agreed with' : 'Differed from'} the agent — ` +
      `${state.agree.same}/${state.agree.total} matched (${pct}%).`;
  }

  try {
    const id = state.hand.handId;
    const h = await api(`/api/hand/step?agent=${state.agent}&id=${id}&action=${action}`, { method: 'POST' });
    renderHandState(h);
    if (h.finished) recordResult(h);
  } catch (err) {
    setEngine(false, err.message);
    stopAutoplay();
  }
}

function setDecider(who) {
  state.decider = who;
  const agentMode = who === 'agent';
  $('btn-step').hidden = !agentMode;
  $('btn-hit').hidden = agentMode;
  $('btn-stand').hidden = agentMode;
  $('btn-auto').hidden = !agentMode;
  if (!agentMode) stopAutoplay();
}

function stopAutoplay() {
  state.autoplay = false;
  clearTimeout(state.autoTimer);
  $('btn-auto').classList.remove('is-active');
  $('btn-auto').textContent = 'Auto-play';
}

function startAutoplay() {
  state.autoplay = true;
  $('btn-auto').classList.add('is-active');
  $('btn-auto').textContent = 'Stop';
  autoTick();
}

async function autoTick() {
  if (!state.autoplay) return;
  if (!state.hand || state.hand.finished) await deal();
  else await step('auto');
  if (!state.autoplay) return;
  state.autoTimer = setTimeout(autoTick, Number($('speed').value));
}

/* ============================================================ POLICY ===== */

// Diverging scale: one hue per action, neutral at a zero margin. Intensity is
// normalised against the 95th percentile so a couple of extreme states don't
// flatten everything else.
function marginScale(cells) {
  const mags = cells.filter((c) => c.learned).map((c) => Math.abs(c.margin)).sort((a, b) => a - b);
  if (!mags.length) return 0.5;
  return Math.max(0.05, mags[Math.floor(mags.length * 0.95)] || 0.5);
}

function cellColor(cell, scale) {
  if (!cell.learned) return null;
  const t = Math.min(1, Math.abs(cell.margin) / scale);
  const pole = cell.margin >= 0 ? 'var(--hit)' : 'var(--stand)';
  const pct = (12 + t * 76).toFixed(0);
  return `color-mix(in oklab, ${pole} ${pct}%, var(--neutral))`;
}

function cellKey(sum, up, ace) { return `${sum}|${up}|${ace ? 1 : 0}`; }

function indexCells(cells) {
  const map = new Map();
  cells.forEach((c) => map.set(cellKey(c.playerSum, c.dealerUpcard, c.usableAce), c));
  return map;
}

function upcardLabel(u) { return u === 11 ? 'A' : String(u); }

function buildGrid(container, cells, opts) {
  const { mini = false, soft = false, showBasic = false } = opts || {};
  const map = indexCells(cells);
  const scale = marginScale(cells);
  const sums = soft ? SUMS_SOFT : SUMS_HARD;

  container.replaceChildren();
  container.style.gridTemplateColumns = mini
    ? `repeat(${UPCARDS.length}, 1fr)`
    : `42px repeat(${UPCARDS.length}, minmax(34px, 1fr))`;

  if (!mini) {
    container.appendChild(headCell(''));
    UPCARDS.forEach((u) => container.appendChild(headCell(upcardLabel(u), 'gcell-head')));
  }

  sums.forEach((sum) => {
    if (!mini) container.appendChild(headCell(String(sum), 'gcell-side'));
    UPCARDS.forEach((u) => {
      const cell = map.get(cellKey(sum, u, soft));
      container.appendChild(gridCell(cell, scale, { mini, showBasic, sum, u, soft }));
    });
  });
}

function headCell(text, cls = 'gcell-head') {
  const el = document.createElement('div');
  el.className = `gcell ${cls}`;
  el.textContent = text;
  return el;
}

function gridCell(cell, scale, ctx) {
  const el = document.createElement('div');
  el.className = 'gcell';

  if (!cell) {
    el.classList.add('gcell-unlearned');
    return el;
  }

  const color = cellColor(cell, scale);
  if (color) el.style.background = color;
  else el.classList.add('gcell-unlearned');

  if (!ctx.mini) {
    // Text label, so identity never rests on colour alone.
    const mark = document.createElement('span');
    mark.className = 'gcell-mark';
    mark.textContent = cell.action === 'hit' ? 'H' : 'S';
    el.appendChild(mark);
  }

  const diverges = cell.learned && cell.basic !== cell.action;
  if (ctx.showBasic && diverges) el.classList.add('gcell-diverge');

  if (!ctx.mini) {
    el.addEventListener('mousemove', (e) => showTip(cellTip(cell), e));
    el.addEventListener('mouseleave', hideTip);
  }
  return el;
}

function cellTip(cell) {
  const rows = [
    ['Q(hit)', cell.qHit.toFixed(4)],
    ['Q(stand)', cell.qStand.toFixed(4)],
    ['margin', cell.margin.toFixed(4)],
  ];
  if (cell.visits >= 0) rows.push(['visits', cell.visits.toLocaleString()]);
  rows.push(['basic strategy', cell.basic]);

  return (
    `<b>(${cell.playerSum}, ${cell.dealerUpcard}, ${cell.usableAce ? 'T' : 'F'})</b><br>` +
    `<b>${cell.action.toUpperCase()}</b>${cell.learned ? '' : ' — heuristic fallback'}<br>` +
    rows.map(([k, v]) => `<span class="tt-row"><span>${k}</span><span>${v}</span></span>`).join('')
  );
}

function buildPolicyTable(cells, soft) {
  const table = $('policy-table');
  const sums = soft ? SUMS_SOFT : SUMS_HARD;
  const map = indexCells(cells);

  const head =
    `<caption>Rows: player sum. Columns: dealer upcard. H = hit, S = stand, ` +
    `followed by Q(hit) − Q(stand).</caption>` +
    `<thead><tr><th>player sum</th>` +
    `<th colspan="${UPCARDS.length}" class="th-group">dealer upcard</th></tr>` +
    `<tr><th></th>${UPCARDS.map((u) => `<th>${upcardLabel(u)}</th>`).join('')}</tr></thead>`;
  const body = sums.map((sum) => {
    const tds = UPCARDS.map((u) => {
      const c = map.get(cellKey(sum, u, soft));
      if (!c) return '<td>—</td>';
      return `<td>${c.action === 'hit' ? 'H' : 'S'} <span style="color:var(--ink-3)">${c.margin.toFixed(2)}</span></td>`;
    }).join('');
    return `<tr><td>${sum}</td>${tds}</tr>`;
  }).join('');

  table.innerHTML = `${head}<tbody>${body}</tbody>`;
}

async function loadPolicy() {
  try {
    const res = await api(`/api/policy?agent=${state.agent}`);
    state.policyCells = res.cells;
    renderPolicy();
  } catch (err) {
    setEngine(false, err.message);
  }
}

function renderPolicy() {
  if (!state.policyCells) return;
  const soft = state.handType === 'soft';
  buildGrid($('policy-grid'), state.policyCells, { soft, showBasic: state.showBasic });
  buildPolicyTable(state.policyCells, soft);
}

/* ========================================================== TRAINING ===== */

// Sizes the backing bitmap for the display density and returns a context whose
// coordinates are CSS pixels.
//
// The logical height comes from data-h, never from canvas.height: assigning to
// canvas.height rewrites that same attribute, so reading it back would multiply
// by devicePixelRatio on every redraw and stretch the chart a little further
// each time. The CSS height is pinned for the same reason — with width:100% and
// an auto height, the element's aspect ratio would follow the bitmap.
// Measures the canvas's laid-out width. A canvas inside a panel that was just
// unhidden, or one measured before layout has settled, can report 0 — so fall
// back to the container and let the caller retry rather than drawing nothing.
function chartWidth(canvas) {
  const own = Math.round(canvas.getBoundingClientRect().width) || canvas.clientWidth;
  if (own >= 2) return own;

  const box = canvas.parentElement;
  if (!box) return 0;
  // Subtract the container's horizontal padding, which the canvas does not span.
  const cs = getComputedStyle(box);
  const pad = parseFloat(cs.paddingLeft || 0) + parseFloat(cs.paddingRight || 0);
  return Math.max(0, Math.round(box.getBoundingClientRect().width - pad));
}

function chartCtx(canvas) {
  const dpr = window.devicePixelRatio || 1;
  const h = Number(canvas.dataset.h) || 180;
  canvas.style.height = `${h}px`;

  const w = chartWidth(canvas);
  if (w < 2) return null;             // not laid out yet; drawLine will retry

  const bw = Math.round(w * dpr);
  const bh = Math.round(h * dpr);
  if (canvas.width !== bw || canvas.height !== bh) {
    canvas.width = bw;
    canvas.height = bh;
  }

  const ctx = canvas.getContext('2d');
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  ctx.clearRect(0, 0, w, h);
  return { ctx, w, h };
}

function css(name) {
  return getComputedStyle(document.body).getPropertyValue(name).trim();
}

// A single-series line chart. Two measures on different scales get two of
// these stacked, never one chart with two y-axes.
function drawLine(canvas, points, accessor, opts) {
  const sized = chartCtx(canvas);
  if (!sized) {
    // The canvas has no width yet. Come back after the browser has laid out,
    // instead of leaving a blank box.
    retryChartsOnce();
    return;
  }
  chartRetriesLeft = 5;   // sized successfully; allow retries again next time
  const { ctx, w, h } = sized;
  const padL = 46, padR = 12, padT = 10, padB = 22;
  const plotW = w - padL - padR;
  const plotH = h - padT - padB;

  const inkGrid = css('--line');
  const inkText = css('--ink-3');
  const stroke = opts.color;

  const yMin = opts.yMin !== undefined ? opts.yMin : 0;
  const yMax = opts.yMax !== undefined ? opts.yMax : 1;
  const xMax = points.length ? points[points.length - 1].episode : 1;

  ctx.font = '10px ui-monospace, monospace';
  ctx.textBaseline = 'middle';

  // Recessive gridlines and axis labels. Tick count is per-chart so each axis
  // lands on round numbers.
  const ticks = opts.ticks || 4;
  for (let i = 0; i <= ticks; i++) {
    const v = yMin + ((yMax - yMin) * i) / ticks;
    const y = padT + plotH - (plotH * i) / ticks;
    ctx.strokeStyle = inkGrid;
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.moveTo(padL, y + 0.5);
    ctx.lineTo(w - padR, y + 0.5);
    ctx.stroke();
    ctx.fillStyle = inkText;
    ctx.textAlign = 'right';
    ctx.fillText(opts.fmt(v), padL - 8, y);
  }

  // Reference line: the benchmark this curve is trying to reach.
  if (opts.reference && opts.reference.value >= yMin && opts.reference.value <= yMax) {
    const ry = padT + plotH - ((opts.reference.value - yMin) / (yMax - yMin)) * plotH;
    ctx.save();
    ctx.strokeStyle = inkText;
    ctx.setLineDash([4, 4]);
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.moveTo(padL, ry + 0.5);
    ctx.lineTo(w - padR, ry + 0.5);
    ctx.stroke();
    ctx.restore();
    ctx.fillStyle = inkText;
    ctx.textAlign = 'right';
    ctx.fillText(opts.reference.label, w - padR - 4, ry - 8);
  }

  if (points.length < 2) return;

  ctx.textAlign = 'center';
  ctx.fillStyle = inkText;
  ctx.fillText('0', padL, h - 9);
  ctx.fillText(xMax.toLocaleString(), w - padR, h - 9);

  const px = (p) => padL + (p.episode / xMax) * plotW;
  const py = (p) => padT + plotH - ((accessor(p) - yMin) / (yMax - yMin)) * plotH;

  // Clip to the plot rectangle. A noisy window can briefly leave the axis
  // range, and without this the stroke would run across the tick labels.
  ctx.save();
  ctx.beginPath();
  ctx.rect(padL, padT, plotW, plotH);
  ctx.clip();

  ctx.strokeStyle = stroke;
  ctx.lineWidth = 2;
  ctx.lineJoin = 'round';
  ctx.beginPath();
  points.forEach((p, i) => (i ? ctx.lineTo(px(p), py(p)) : ctx.moveTo(px(p), py(p))));
  ctx.stroke();

  // Direct label on the last point rather than a number on every point.
  const last = points[points.length - 1];
  ctx.fillStyle = stroke;
  ctx.beginPath();
  ctx.arc(px(last), py(last), 3, 0, Math.PI * 2);
  ctx.fill();

  ctx.restore();
}

// One deferred retry, for the case where a chart is asked to draw before the
// browser has given it a width. Guarded so a genuinely zero-width container
// can't spin.
let chartRetry = 0;
let chartRetriesLeft = 5;
function retryChartsOnce() {
  if (chartRetry || chartRetriesLeft <= 0) return;
  chartRetriesLeft--;
  chartRetry = requestAnimationFrame(() => {
    chartRetry = 0;
    redrawCharts();
  });
}

// Draw whenever the charts' container gains or changes width. This covers the
// first reveal of the training panel, window resizes, and layout settling after
// fonts load, without any of them needing their own hook.
function watchChartSize() {
  if (typeof ResizeObserver !== 'function') return;
  const box = $('chart-ev').parentElement;
  if (!box) return;
  let last = 0;
  new ResizeObserver((entries) => {
    const w = Math.round(entries[0].contentRect.width);
    if (w >= 2 && w !== last) {
      last = w;
      redrawCharts();
    }
  }).observe(box);
}

function redrawCharts() {
  const pts = state.training.points;
  // Basic strategy's EV under these rules, drawn as the target line so the
  // curve is read against the achievable ceiling rather than against zero.
  // Range covers what a window actually produces: an untrained agent bottoms
  // out near -0.34, and a lucky window can run slightly positive.
  drawLine($('chart-ev'), pts, (p) => p.avgReward, {
    color: css('--stand'), yMin: -0.4, yMax: 0.2, ticks: 6, fmt: (v) => v.toFixed(1),
    reference: state.baselineEV === null
      ? null
      : { value: state.baselineEV, label: 'basic strategy' },
  });
  drawLine($('chart-winrate'), pts, (p) => p.winRate, {
    color: css('--ink-2'), yMin: 0, yMax: 0.6, ticks: 6, fmt: (v) => `${(v * 100).toFixed(0)}%`,
  });
  drawLine($('chart-epsilon'), pts, (p) => p.epsilon, {
    color: css('--hit'), yMin: 0, yMax: 1, ticks: 4, fmt: (v) => v.toFixed(2),
  });
}

async function pollTraining() {
  try {
    // In WASM mode there is no worker thread, so this loop *is* the trainer:
    // run a slice of episodes, then hand control back to the browser so it can
    // paint the chart and stay responsive.
    if (transport.mode === 'wasm') {
      await api(`/api/train/step?episodes=${WASM_STEP_EPISODES}`);
    }

    const res = await api(`/api/train/progress?since=${state.training.cursor}`);
    state.training.cursor = res.cursor;
    state.training.points.push(...res.points);

    $('s-progress').textContent = res.done.toLocaleString();
    if (state.training.points.length) {
      const last = state.training.points[state.training.points.length - 1];
      $('s-ev').textContent = last.avgReward.toFixed(3);
      $('s-winrate').textContent = `${(last.winRate * 100).toFixed(1)}%`;
      $('s-epsilon').textContent = last.epsilon.toFixed(4);
      $('s-states').textContent = last.statesLearned;
    }
    redrawCharts();

    // Refresh the live grid a few times a second at most.
    if (res.points.length) {
      const p = await api(`/api/policy?agent=${state.agent}`);
      buildGrid($('mini-grid'), p.cells, { mini: true });
      if (state.policyCells) { state.policyCells = p.cells; renderPolicy(); }
    }

    if (!res.running) {
      finishTraining(res);
      return;
    }
    // WASM drives episodes from here, so come straight back; the HTTP build is
    // only polling a thread that runs on its own.
    state.training.poll = setTimeout(pollTraining, transport.mode === 'wasm' ? 0 : 400);
  } catch (err) {
    setEngine(false, err.message);
    finishTraining({ done: 0, total: 0 });
  }
}

function finishTraining(res) {
  clearTimeout(state.training.poll);
  $('btn-train').disabled = false;
  $('btn-train').textContent = 'Start';
  $('btn-train-stop').disabled = true;
  $('mini-note').textContent = res.done
    ? `Finished ${Number(res.done).toLocaleString()} episodes. Save the table to keep it.`
    : 'Training stopped.';
  refreshStatus().catch(() => {});
  loadPolicy();
}

async function startTraining() {
  const episodes = Number($('train-episodes').value) || 100000;
  const reset = $('chk-reset').checked;

  state.training = { cursor: 0, points: [], poll: null };
  $('btn-train').disabled = true;
  $('btn-train').textContent = 'Running…';
  $('btn-train-stop').disabled = false;
  $('mini-note').textContent = 'Training…';
  redrawCharts();

  try {
    await post(`/api/train?agent=${state.agent}&episodes=${episodes}&reset=${reset}`);
    pollTraining();
  } catch (err) {
    setEngine(false, err.message);
    $('mini-note').textContent = err.message;
    finishTraining({ done: 0 });
  }
}

/* =========================================================== COMPARE ===== */

// Average reward is the hero, not win rate: win rate is capped near 44% by the
// rules of the game, so it cannot separate a good policy from a mediocre one.
function agentBlock(r, baselineEV) {
  const total = r.wins + r.losses + r.pushes;
  const pct = (n) => (total ? (n / total) * 100 : 0);
  const isBaseline = r.agent === 'basic';

  let gap = '';
  if (!isBaseline && baselineEV !== null) {
    const d = r.avgReward - baselineEV;
    // Only call it a real difference if it clears the combined error bars.
    const sig = Math.abs(d) > r.avgRewardCI95;
    gap = `<div class="cmp-gap ${sig ? 'is-sig' : ''}">${d >= 0 ? '+' : ''}${d.toFixed(4)} vs basic strategy${
      sig ? '' : ' (within noise)'}</div>`;
  }

  return `
    <div class="card-surface cmp-card${isBaseline ? ' cmp-card-baseline' : ''}">
      <h3>${r.label}${isBaseline ? ' <span class="cmp-tag">benchmark</span>' : ''}</h3>
      <p class="cmp-sub">${r.games.toLocaleString()} hands${isBaseline ? ', fixed policy' : ', greedy policy'}</p>
      <div class="cmp-hero">${r.avgReward >= 0 ? '+' : ''}${r.avgReward.toFixed(4)}</div>
      <span class="cmp-hero-l">average reward per hand ±${r.avgRewardCI95.toFixed(4)}</span>
      ${gap}
      <div class="cmp-bar" role="img" aria-label="${r.wins} won, ${r.losses} lost, ${r.pushes} pushed">
        <div class="cmp-seg cmp-seg-win"  style="width:${pct(r.wins)}%"></div>
        <div class="cmp-seg cmp-seg-loss" style="width:${pct(r.losses)}%"></div>
        <div class="cmp-seg cmp-seg-push" style="width:${pct(r.pushes)}%"></div>
      </div>
      <div class="cmp-key">
        <span><i class="cmp-seg-win"></i>won ${r.wins.toLocaleString()}</span>
        <span><i class="cmp-seg-loss"></i>lost ${r.losses.toLocaleString()}</span>
        <span><i class="cmp-seg-push"></i>pushed ${r.pushes.toLocaleString()}</span>
      </div>
      <div class="cmp-key" style="margin-top:14px">
        <span>win rate ${(r.winRate * 100).toFixed(2)}%</span>
        <span>blackjacks ${r.blackjacks.toLocaleString()}</span>
      </div>
    </div>`;
}

function disagreementBlock(res) {
  if (!res.comparableStates) {
    return `<div class="card-surface"><p class="empty">Neither agent has learned enough states to compare. Train them first.</p></div>`;
  }
  const rows = res.disagreementCells.map((d) =>
    `<tr><td>(${d.playerSum}, ${d.dealerUpcard}, ${d.usableAce ? 'T' : 'F'})</td><td>${d.q}</td><td>${d.mc}</td></tr>`).join('');

  return `
    <div class="card-surface">
      <h3>Where they disagree</h3>
      <p class="cmp-sub">${res.disagreements} of ${res.comparableStates} states both agents actually learned</p>
      ${res.disagreements
        ? `<div class="table-scroll"><table>
             <thead><tr><th>state</th><th>Q-learning</th><th>Monte Carlo</th></tr></thead>
             <tbody>${rows}</tbody></table></div>`
        : `<p class="empty">Identical policies on every shared state.</p>`}
    </div>`;
}

async function runCompare() {
  const games = Number($('cmp-games').value) || 50000;
  const btn = $('btn-compare');
  btn.disabled = true;
  btn.textContent = 'Running…';
  $('compare-body').innerHTML = `<p class="empty">Playing ${games.toLocaleString()} hands per agent in C++…</p>`;

  try {
    const res = await api(`/api/compare?games=${games}`);
    state.baselineEV = res.basic.avgReward;
    $('compare-body').innerHTML =
      `<div class="cmp-row">${agentBlock(res.basic, null)}${agentBlock(res.q, res.basic.avgReward)}${
        agentBlock(res.mc, res.basic.avgReward)}</div>` + disagreementBlock(res);
  } catch (err) {
    $('compare-body').innerHTML = `<p class="empty">${err.message}</p>`;
    setEngine(false, err.message);
  } finally {
    btn.disabled = false;
    btn.textContent = 'Run';
  }
}

/* ============================================================== WIRING === */

function bindSegmented(selector, attr, onPick) {
  document.querySelectorAll(selector).forEach((btn) => {
    btn.addEventListener('click', () => {
      const group = btn.closest('.segmented');
      group.querySelectorAll('.seg').forEach((b) => {
        b.classList.remove('is-on');
        b.setAttribute('aria-checked', 'false');
      });
      btn.classList.add('is-on');
      btn.setAttribute('aria-checked', 'true');
      onPick(btn.dataset[attr]);
    });
  });
}

async function init() {
  setEngine(false, 'starting…');
  try {
    await initTransport();
  } catch (err) {
    setEngine(false, `could not start engine: ${err.message}`);
    return;
  }
  if (transport.mode === 'wasm') {
    $('btn-save').textContent = 'Download q-table';
  }

  // Tabs
  document.querySelectorAll('.tab').forEach((tab) => {
    tab.addEventListener('click', () => {
      document.querySelectorAll('.tab').forEach((t) => {
        t.classList.remove('is-on');
        t.setAttribute('aria-selected', 'false');
      });
      document.querySelectorAll('.panel').forEach((p) => p.classList.remove('is-on'));
      tab.classList.add('is-on');
      tab.setAttribute('aria-selected', 'true');
      $(`panel-${tab.dataset.tab}`).classList.add('is-on');

      if (tab.dataset.tab === 'policy' && !state.policyCells) loadPolicy();
      if (tab.dataset.tab === 'training') redrawCharts();
    });
  });

  bindSegmented('.seg[data-agent]', 'agent', (agent) => {
    state.agent = agent;
    state.policyCells = null;
    stopAutoplay();
    state.hand = null;
    refreshStatus().catch(() => {});
    if ($('panel-policy').classList.contains('is-on')) loadPolicy();
  });

  bindSegmented('.seg[data-decider]', 'decider', setDecider);

  bindSegmented('.seg[data-hand]', 'hand', (type) => {
    state.handType = type;
    renderPolicy();
  });

  $('chk-basic').addEventListener('change', (e) => {
    state.showBasic = e.target.checked;
    renderPolicy();
  });

  $('btn-deal').addEventListener('click', () => { stopAutoplay(); deal(); });
  $('btn-step').addEventListener('click', () => step('auto'));
  $('btn-hit').addEventListener('click', () => step('hit'));
  $('btn-stand').addEventListener('click', () => step('stand'));
  $('btn-auto').addEventListener('click', () => (state.autoplay ? stopAutoplay() : startAutoplay()));
  $('speed').addEventListener('input', (e) => { $('speed-val').textContent = `${e.target.value}ms`; });

  $('btn-train').addEventListener('click', startTraining);
  $('btn-train-stop').addEventListener('click', () => post('/api/train/stop').catch(() => {}));
  $('btn-save').addEventListener('click', async () => {
    const btn = $('btn-save');
    const label = btn.textContent;
    btn.disabled = true;
    try {
      if (transport.mode === 'wasm') {
        // Nothing persistent to write to in a browser tab, so hand the trained
        // table to the user as a file they can drop into data/.
        const csv = await apiText(`/api/qtable.csv?agent=${state.agent}`);
        downloadText(`${state.agent}_q_table.csv`, csv, 'text/csv');
        btn.textContent = 'Downloaded';
      } else {
        const res = await post(`/api/save?agent=${state.agent}`);
        btn.textContent = `Saved ${res.saved}`;
      }
    } catch (err) {
      btn.textContent = err.message;
    } finally {
      setTimeout(() => { btn.textContent = label; btn.disabled = false; }, 2200);
    }
  });

  $('btn-compare').addEventListener('click', runCompare);

  window.addEventListener('resize', () => {
    if ($('panel-training').classList.contains('is-on')) redrawCharts();
  });

  setDecider('agent');
  watchChartSize();
  refreshStatus().then(deal).catch(() => {});

  // Measure the benchmark once in the background; the training chart draws it
  // as a target line, so the curve is read against what is actually achievable.
  api('/api/simulate?agent=basic&games=200000')
    .then((r) => {
      state.baselineEV = r.avgReward;
      if ($('panel-training').classList.contains('is-on')) redrawCharts();
    })
    .catch(() => {});
}

document.addEventListener('DOMContentLoaded', () => { init(); });

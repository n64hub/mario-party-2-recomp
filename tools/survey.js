// Evaluated in the page by N64Recomp-browser's tools/record_shaders.mjs (tools/shader_plan.json) and play_page.mjs:
// finds patches/survey.c's struct in RDRAM by its magic words and offers overlay(), the overlay playing (the one
// omOvlGotoEx last went to, a word at 0x800FA63C: 0x5B Mario Land, 0x4D Rules Land, 0x4C its board, 0x3E 0x41 0x43
// 0x45 0x47 0x49 the six boards, 0x01-0x3C the minigames, 0x3F 0x42 0x44 0x46 0x48 0x4A the boards' duels, 0x5F and
// 0x60 a minigame's explanation; the debug menu's table, ovl_00_Debug's debug_ovl_table, names them all);
// untilOverlay(overlay, ms), which resolves true once that overlay plays, false after ms; warps(), how many requests
// the game has taken; ask(from, overlay, event, stat, every, groups, computers, minigame), which asks the game to go to
// overlay (with omOvlGotoEx's event and stat, -1 for the game's own) the next time it leaves for `from` (-1: for
// anywhere), or every time with `every` (a board, which the game goes to again after each minigame), with the players'
// groups (four of GwPlayer's group, player 1's first; left out, they stay), those the computer plays (four booleans;
// left out, they stay) and GwSystem's minigame ID (left out, it stays) set as it goes (patches/survey.c);
// minigame(from, id, groups), which opens a minigame as a board does, by its explanation screen, with its groups and
// four computer players: the game's debug menu's groups for each ID are [0, 1, 2, 3] for four players, [0, 1, 1, 1] for
// 1 vs 3 and [0, 0, 1, 1] for 2 vs 2 and a duel; frames(), the scheduler's frame count (one a frame the game makes: its
// own rate); mark() and rate(), how many frames a second the game made from the one to the other.
(() => {
  const magic = (a) => n64r(a) === 0x4d505254 && n64r(a + 4) === 0x53525659;
  if (!window.__survey) {
    // The patches' memory first (patches/patches.ld), then the rest of RDRAM.
    for (let a = 0x80801000; a < 0x81000000 && !window.__survey; a += 4) if (magic(a)) window.__survey = a;
    for (let a = 0x80000000; a < 0x80801000 && !window.__survey; a += 4) if (magic(a)) window.__survey = a;
  }
  const s = window.__survey;
  const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));
  const hex = (n) => (n < 0 ? '-' : '') + Math.abs(n).toString(16);
  window.overlay = () => n64r(s + 32) | 0;
  window.frames = () => n64r(s + 36) >>> 0;
  window.warps = () => n64r(s + 28) >>> 0;
  window.untilOverlay = async (overlay, ms) => {
    const end = Date.now() + ms;
    while (window.overlay() !== overlay && Date.now() < end) await sleep(100);
    return window.overlay() === overlay;
  };
  window.ask = (from, overlay, event, stat, every, groups, computers, minigame) => {
    n64w(s + 12, from >>> 0);
    n64w(s + 16, overlay);
    n64w(s + 20, event >>> 0);
    n64w(s + 24, stat >>> 0);
    n64w(s + 40, groups ? ((groups[0] << 24) | (groups[1] << 16) | (groups[2] << 8) | groups[3]) >>> 0 : 0xffffffff);
    const bits = computers ? computers.reduce((all, on, player) => all | (on ? 1 << player : 0), 0x80000000) : 0;
    n64w(s + 44, bits >>> 0);
    n64w(s + 48, minigame === undefined ? 0xffffffff : minigame);
    n64w(s + 8, every ? 2 : 1);
    const when = every ? ' every time' : '';
    return `asked for ${hex(overlay)} in place of ${hex(from)}${when}, ${window.warps()} taken before`;
  };
  window.minigame = (from, id, groups) =>
    window.ask(from, id < 30 ? 0x5f : 0x60, 0, 0x94, false, groups, [true, true, true, true], id) + `, minigame ${hex(id)}`;
  window.mark = () => { window.__mark = [window.frames(), performance.now()]; return 'frames ' + window.__mark[0]; };
  window.rate = () => {
    const [frames, ms] = [window.frames() - window.__mark[0], performance.now() - window.__mark[1]];
    return `${frames} game frames in ${(ms / 1000).toFixed(1)} s: ${(frames * 1000 / ms).toFixed(1)} a second`;
  };
  return s ? 'survey at ' + s.toString(16) : 'no survey struct: a page built without patches/survey.c';
})()

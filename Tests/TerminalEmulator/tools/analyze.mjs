import fs from 'node:fs';
const rec = JSON.parse(fs.readFileSync(process.argv[2], 'utf8'));
const text = rec.chunks.map((c) => c.data).join('');
const csi = new Map();
const priv = new Map();
const sgr = new Map();
const other = new Map();
const bump = (m, k) => m.set(k, (m.get(k) || 0) + 1);
const re = /\x1b\[([?>=!]?)([0-9;:]*)([ -\/]*)([@-~])|\x1b\]([^\x07\x1b]*)(?:\x07|\x1b\\)|\x1b([()*+].|[^\[\]])/g;
let m;
while ((m = re.exec(text)) !== null) {
	if (m[4] !== undefined) {
		const key = (m[1] || '') + m[4];
		if (m[4] === 'm') {
			for (const p of (m[2] === '' ? '0' : m[2]).split(';')) bump(sgr, p);
		} else if (m[1] === '?' && (m[4] === 'h' || m[4] === 'l')) {
			bump(priv, `?${m[2]}${m[4]}`);
		} else {
			bump(csi, key + (m[2] === '' ? '' : ' (e.g. ' + m[2] + ')'));
		}
	} else if (m[5] !== undefined) {
		bump(other, 'OSC ' + m[5].slice(0, 30));
	} else {
		bump(other, 'ESC ' + JSON.stringify(m[6]));
	}
}
const dump = (title, map) => console.log(title + ':', [...map.entries()].sort((a, b) => b[1] - a[1]).slice(0, 40).map(([k, v]) => `${k} x${v}`).join(' | '));
dump('CSI finals', csi);
dump('private modes', priv);
dump('SGR parameters', sgr);
dump('ESC / OSC', other);
const nonAscii = new Set([...text].filter((c) => c.charCodeAt(0) > 126));
console.log('non-ASCII chars:', [...nonAscii].join(' ').slice(0, 120));
const ctl = new Map();
for (const c of text) if (c < ' ' && c !== '\x1b') bump(ctl, JSON.stringify(c));
dump('control chars', ctl);
console.log('first 300 chars:', JSON.stringify(text.slice(0, 300)));

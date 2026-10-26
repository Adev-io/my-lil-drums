#!/usr/bin/env python3
"""panel.py: writes res/Drums.svg and src/layout.hpp from one layout table.

Labels become paths (DejaVu Sans from the Rack install, or the font named by RACK_FONT), so the SVG has no <text>.
The widgets read the same table through layout.hpp, so art and components cannot drift.
House style as in My lil Looper / Acid Bass. Run from anywhere: py -3.11 tools/panel.py   (needs fontTools).
Author: Ari Klopfer / A_Development. MIT
"""
import math
import os
from fontTools.ttLib import TTFont
from fontTools.pens.svgPathPen import SVGPathPen
from fontTools.pens.transformPen import TransformPen

H = 128.5
W = 5.08 * 60
BAND = 8.6
INK, INK2 = '#16161a', '#202027'
LIME, MAGENTA, BLUE, WHITE, DIM = '#c8ff3a', '#ff3d9a', '#5ac8e6', '#f2f2f4', '#8d8d96'
FONT = os.environ.get('RACK_FONT', r'C:\Program Files\VCV\Rack2Free\res\fonts\DejaVuSans.ttf')
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

RADIUS = {'knob': 6.7, 'button': 3.1, 'seq': 4.0, 'play': 4.0, 'cv': 4.0, 'out': 4.0}
LABEL_SIZE = {'knob': 2.4, 'button': 2.0, 'seq': 2.4, 'out': 2.2}
RING_COLOR = {'seq': BLUE, 'play': LIME, 'cv': MAGENTA}
PAD_R = 4.8
RING_R = RADIUS['cv'] + 0.7

font = TTFont(FONT)
glyphs, cmap, upm = font.getGlyphSet(), font.getBestCmap(), font['head'].unitsPerEm


def width(s, size):
    return sum(glyphs[cmap[ord(c)]].width for c in s) * size / upm


def text(s, x, y, size, fill, anchor='middle', stroke=0.0):
    """One <path> for the string: baseline at y, em size in mm, optional stroke for a bolder face."""
    k = size / upm
    w = width(s, size)
    pen_x = x - w / 2 if anchor == 'middle' else x - w if anchor == 'end' else x
    d = []
    for c in s:
        g = glyphs[cmap[ord(c)]]
        pen = SVGPathPen(glyphs)
        g.draw(TransformPen(pen, (k, 0, 0, -k, pen_x, y)))
        d.append(pen.getCommands())
        pen_x += g.width * k
    bold = ' stroke="%s" stroke-width="%g" stroke-linejoin="round"' % (fill, stroke) if stroke else ''
    return '<path fill="%s"%s d="%s"/>' % (fill, bold, ' '.join(d))


TITLE, TITLE_X, TITLE_Y, TITLE_SIZE = 'DRUMS', 11.2, 6.3, 4.3
MASCOT_X = W - 14.46
MASCOT_R = BAND * 0.36


def mascot(cx, cy, r):
    e = r * 0.37
    return ''.join([
        '<circle cx="%g" cy="%g" r="%g" fill="%s"/>' % (cx, cy, r, MAGENTA),
        '<circle cx="%g" cy="%g" r="%g" fill="%s"/>' % (cx - e, cy - r * 0.32, r * 0.145, INK),
        '<circle cx="%g" cy="%g" r="%g" fill="%s"/>' % (cx + e, cy - r * 0.32, r * 0.145, INK),
        '<line x1="%g" y1="%g" x2="%g" y2="%g" stroke="%s" stroke-width="%g" stroke-linecap="round"/>'
        % (cx - r * 0.58, cy + r * 0.45, cx + r * 0.58, cy + r * 0.45, INK, r * 0.16),
    ])


# ---- regions drawn by widgets (x, y, w, h in mm)
REGIONS = {
    'ROLL': (4.0, 13.0, 95.6, 57.6),      # 6 mm row-number gutter + 16 columns x 16 rows
    'TABS': (10.0, 71.6, 89.6, 4.4),      # 4 page tabs
    'VEL': (10.0, 77.0, 89.6, 12.0),      # velocity bars of the selected pad
    'PADGRID': (106.0, 13.0, 56.5, 52.0), # 4x4 pads + caption line
}
EDITOR = (168.0, 13.0, 130.0, 76.0)       # backing frame for the pad editor knobs

# ---- components: (name, kind, x, y, label)
EDIT_X = [178.0 + 16.0 * i for i in range(8)]
ROW1, ROW2 = 34.0, 66.0
TRANSPORT_KNOB_Y, JACK_Y = 99.5, 115.0
EDIT = [
    ('VOL_PARAM', 'knob', EDIT_X[0], ROW1, 'VOL'), ('PAN_PARAM', 'knob', EDIT_X[1], ROW1, 'PAN'),
    ('PITCH_PARAM', 'knob', EDIT_X[2], ROW1, 'PITCH'), ('ATK_PARAM', 'knob', EDIT_X[3], ROW1, 'ATK'),
    ('DEC_PARAM', 'knob', EDIT_X[4], ROW1, 'DEC'), ('SUS_PARAM', 'knob', EDIT_X[5], ROW1, 'SUS'),
    ('REL_PARAM', 'knob', EDIT_X[6], ROW1, 'REL'), ('MODE_PARAM', 'button', EDIT_X[7], ROW1, 'GATE'),
    ('LP_PARAM', 'knob', EDIT_X[0], ROW2, 'LP'), ('HP_PARAM', 'knob', EDIT_X[1], ROW2, 'HP'),
    ('THR_PARAM', 'knob', EDIT_X[2], ROW2, 'THRESH'), ('RATIO_PARAM', 'knob', EDIT_X[3], ROW2, 'RATIO'),
    ('CATK_PARAM', 'knob', EDIT_X[4], ROW2, 'C ATK'), ('CREL_PARAM', 'knob', EDIT_X[5], ROW2, 'C REL'),
    ('MAKEUP_PARAM', 'knob', EDIT_X[6], ROW2, 'MAKEUP'), ('CHOKE_PARAM', 'knob', EDIT_X[7], ROW2, 'CHOKE'),
]
TRANSPORT = [
    ('BPM_PARAM', 'knob', 14.0, TRANSPORT_KNOB_Y, 'BPM'), ('SWING_PARAM', 'knob', 30.0, TRANSPORT_KNOB_Y, 'SWING'),
    ('LENGTH_PARAM', 'knob', 46.0, TRANSPORT_KNOB_Y, 'STEPS'), ('PATTERN_PARAM', 'knob', 62.0, TRANSPORT_KNOB_Y, 'PATTERN'),
    ('LEVEL_PARAM', 'knob', 78.0, TRANSPORT_KNOB_Y, 'LEVEL'),
    ('CLOCK_INPUT', 'seq', 14.0, JACK_Y, 'CLOCK'), ('RESET_INPUT', 'seq', 30.0, JACK_Y, 'RESET'),
    ('RUN_INPUT', 'seq', 46.0, JACK_Y, 'RUN IN'), ('RUN_PARAM', 'button', 62.0, JACK_Y, 'RUN'),
    ('OUT_L_OUTPUT', 'out', 78.0, JACK_Y, 'OUT L'), ('OUT_R_OUTPUT', 'out', 92.0, JACK_Y, 'OUT R'),
]
PADOUT = [(112.0 + 12.0 * (i % 8), TRANSPORT_KNOB_Y if i < 8 else JACK_Y, str(i + 1)) for i in range(16)]
LAYOUT = EDIT + TRANSPORT + [('PADOUT%d' % i, 'out', x, y, lab) for i, (x, y, lab) in enumerate(PADOUT)]
SEP_X = 107.0


def reach(kind):
    return RING_R if kind in RING_COLOR else PAD_R if kind == 'out' else RADIUS[kind]


def label_base(kind, y):
    return y + reach(kind) + (2.6 if kind != 'knob' else 3.2)


def problems():
    """Parts must not overlap each other, their labels, the band, the screws or the panel edge. Must be empty."""
    bad = []
    parts = [(n, x, y, reach(k)) for n, k, x, y, _ in LAYOUT]
    for j, (a, ax, ay, ar) in enumerate(parts):
        if ax - ar < 0 or ax + ar > W or ay - ar < BAND or ay + ar > H:
            bad.append('%s leaves the panel or enters the band' % a)
        for b, bx, by, br in parts[j + 1:]:
            if math.hypot(ax - bx, ay - by) < ar + br:
                bad.append('%s touches %s' % (a, b))
        for sx in (5.08, W - 10.16):
            for sy in (0.0, H - 5.08):
                if sx - ar < sx + 5.08 and sx < ax + ar and sy < ay + ar and ay - ar < sy + 5.08 and ax - ar < sx + 5.08:
                    bad.append('%s under a screw' % a)
    for n, k, x, y, lab in LAYOUT:   # a label must clear every other part
        w, base, size = width(lab, LABEL_SIZE[k]), label_base(k, y), LABEL_SIZE[k]
        box = (x - w / 2, base - 0.73 * size, x + w / 2, base)
        for m, mx, my, mr in parts:
            if m != n:
                nx, ny = min(max(mx, box[0]), box[2]), min(max(my, box[1]), box[3])
                if math.hypot(mx - nx, my - ny) < mr:
                    bad.append('%s label covers %s' % (n, m))
        if box[2] > W or box[3] > H:
            bad.append('%s label leaves the panel' % n)
    return bad


def panel():
    out = [
        '<?xml version="1.0" encoding="UTF-8"?>',
        '<svg xmlns="http://www.w3.org/2000/svg" width="%gmm" height="%gmm" viewBox="0 0 %g %g">' % (W, H, W, H),
        '<rect width="%g" height="%g" fill="%s"/>' % (W, H, INK),
        '<rect width="%g" height="%g" fill="%s"/>' % (W, BAND, LIME),
        text(TITLE, TITLE_X, TITLE_Y, TITLE_SIZE, INK, 'start', 0.39), mascot(MASCOT_X, BAND / 2, MASCOT_R),
        text('my lil drums', W / 2, 12.6, 2.4, WHITE, stroke=0.12),
        '<rect x="2.5" y="92" width="%g" height="33.5" rx="2" fill="%s"/>' % (W - 5, INK2),
        '<rect x="%g" y="94" width="0.4" height="29.5" fill="%s"/>' % (SEP_X, DIM),
        '<rect x="%g" y="%g" width="%g" height="%g" rx="2" fill="%s"/>' % (EDITOR + (INK2,)),
    ]
    for x, y, w, h in REGIONS.values():
        out.append('<rect x="%g" y="%g" width="%g" height="%g" rx="1" fill="#0f0f12"/>' % (x, y, w, h))
    for _, kind, x, y, label in LAYOUT:
        if kind in RING_COLOR:
            out.append('<circle cx="%g" cy="%g" r="%g" fill="none" stroke="%s" stroke-width="0.5"/>' % (x, y, RING_R, RING_COLOR[kind]))
        elif kind == 'out':
            out.append('<circle cx="%g" cy="%g" r="%g" fill="%s" stroke="%s" stroke-width="0.5"/>' % (x, y, PAD_R, INK, LIME))
        colour, stroke = (LIME, 0.2) if kind == 'out' else (WHITE, 0.15)
        out.append(text(label, x, label_base(kind, y), LABEL_SIZE[kind], colour, stroke=stroke))
    out.append(text('A_DEVELOPMENT', W - 6, 127.2, 2.2, DIM, 'end'))
    out.append('</svg>')
    return '\n'.join(out) + '\n'


def header():
    lines = ['// layout.hpp: generated by tools/panel.py, do not edit. Component centres and widget regions in mm.', '#pragma once',
             'namespace layout {', 'namespace drums {']
    lines += ['constexpr float %s[2] = {%.2ff, %.2ff};' % (n, x, y) for n, k, x, y, _ in LAYOUT if not n.startswith('PADOUT')]
    lines += ['constexpr float PADOUT[16][2] = {%s};' % ', '.join('{%.2ff, %.2ff}' % (x, y) for x, y, _ in PADOUT)]
    lines += ['constexpr float %s[4] = {%.2ff, %.2ff, %.2ff, %.2ff};   // x, y, w, h' % ((n,) + r) for n, r in REGIONS.items()]
    lines += ['constexpr float FACE[3] = {%.2ff, %.2ff, %.3ff};   // mascot centre x, y, radius (TalkingFace)' % (MASCOT_X, BAND / 2, MASCOT_R)]
    lines += ['}', '}', '']
    return '\n'.join(lines)


if __name__ == '__main__':
    bad = problems()
    assert not bad, '; '.join(bad)
    files = {os.path.join('res', 'Drums.svg'): panel(), os.path.join('src', 'layout.hpp'): header()}
    for rel, body in files.items():
        with open(os.path.join(ROOT, rel), 'w', encoding='utf-8') as f:
            f.write(body)
    print('wrote', ', '.join(files), '(%d components)' % len(LAYOUT))

# My lil Drums — design

Clean-room FPC-style drum sequencer for VCV Rack. Same toolchain, panel style and mascot as the other My lil modules (see `my-lil-looper`, `my-lil-acid-bass-vcv`).

## Scope
One module, built in three phases.

1. Sequencer + 16 sample pads + stereo mix out.
2. Per-pad sound shaping: ADSR, pitch, filter, compressor.
3. Presets (pattern, kit, factory beats), MIDI out, 16 individual outs.

## Panel
- Transport: clock / reset / run inputs, internal BPM, swing, step-length knob (1–64).
- 4x4 pad grid; click selects the pad, shows sample name.
- Piano roll: 16 rows (one per pad) x up to 64 steps, shown 16 at a time. Click toggles, drag paints. Velocity lane below.
- Pad editor for the selected pad: sample load (file picker), mode (one-shot / gate), choke group, ADSR, pitch (+-12 st), LP/HP filter, compressor (threshold, ratio, attack, release, makeup), pan, volume.

## Outputs
Stereo mix, 16 individual outs, MIDI out (note per pad, velocity per step; routed to FL through loopMIDI).

## DSP per pad
Sample player -> amp ADSR -> pitch -> one-pole LP/HP -> compressor -> pan/volume -> mix + individual out.
Choke groups: a pad firing silences others in its group (open/closed hat).

## Presets
- 16 pattern slots, switchable live, each up to 64 steps.
- Kit save/load: JSON holding 16 sample paths + all per-pad settings.
- Factory beats: boom-bap, house, trap.

## Non-goals
Sample start/trim, time-stretch, per-pad layers, velocity layers. Add when asked.

## Testing
Core (sequencer clock/step logic, ADSR, compressor, choke) lives in `core/` free of Rack types, with plain `assert` tests in `tests/`, as in `my-lil-looper`.

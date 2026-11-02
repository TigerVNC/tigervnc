#!/usr/bin/env python3
#
# This is free and unencumbered software released into the public domain.
#
# Anyone is free to copy, modify, publish, use, compile, sell, or
# distribute this software, either in source code form or as a compiled
# binary, for any purpose, commercial or non-commercial, and by any
# means.
#
# In jurisdictions that recognize copyright laws, the author or authors
# of this software dedicate any and all copyright interest in the
# software to the public domain. We make this dedication for the benefit
# of the public at large and to the detriment of our heirs and
# successors. We intend this dedication to be an overt act of
# relinquishment in perpetuity of all present and future rights to this
# software under copyright law.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
# OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.
#
# For more information, please refer to <https://unlicense.org/>

"""
Plays a constant tone, then a logarithmic sine sweep and then a linear
one through the AudioAdapter, by way of audioperf, into a simulated
output whose clock runs at a slightly different rate than the
server's. It then analyses what the output played and plots that. The
first plot is the warm-up, where the tone shows how far the pitch
deviates while the drift compensation settles, along with the delay.
The others are the spectra of the sweeps that follow, once things have
settled. The plots go straight to the terminal as sixel graphics
unless asked to go to files, e.g.:

    audioperf.py --drift 500 --duration 60
"""

import argparse
import fcntl
import io
import json
import os
import shutil
import struct
import subprocess
import sys
import termios

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.colors import LinearSegmentedColormap
import numpy as np
from PIL import Image
import scipy.signal

FFT_SIZE = 4096
FFT_HOP = 1024
FFT_CHUNK = 256

SECTION_MARGIN = 256

SPECTROGRAM_BANDS = 300
SPECTROGRAM_LOW = 10

SWEEPS = ("logarithmic", "linear")

COMBINED_WIDTH = 1080
COMBINED_GAP = 40

PANEL_GAP = 0.05

THEMES = {
    "dark": {
        "surface": "#1a1a19",
        "text": "#ffffff",
        "muted": "#c3c2b7",
        "grid": "#3a3a38",
        "sweep": "#3987e5",
        "opposite": "#d95926",
        "reference": "#c3c2b7",
        "ramp": ["#000000", "#ffffff"],
    },
    "light": {
        "surface": "#fcfcfb",
        "text": "#0b0b0b",
        "muted": "#52514e",
        "grid": "#e4e3df",
        "sweep": "#2a78d6",
        "opposite": "#eb6834",
        "reference": "#52514e",
        "ramp": ["#ffffff", "#000000"],
    },
}


def find_test(path):
    if path is not None:
        return path

    here = os.path.dirname(os.path.abspath(__file__))
    candidates = [
        os.path.join(here, "audioperf"),
        os.path.join(here, "..", "..", "build", "tests", "perf", "audioperf"),
    ]
    for candidate in candidates:
        if os.access(candidate, os.X_OK):
            return candidate

    found = shutil.which("audioperf")
    if found is not None:
        return found

    sys.exit("Could not find audioperf, use --test to say where it is")


def sweep_start(params, kind):
    return params["warmup"] + SWEEPS.index(kind) * params["duration"]


def sweep_phase(params, kind, t):
    start, end, duration = params["start"], params["end"], params["duration"]

    # Integral of the sweep's frequency, so the phase never accumulates
    # errors
    if kind == "logarithmic":
        k = np.log(end / start)
        return 2 * np.pi * start * duration / k * (np.exp(k * t / duration) - 1)
    return 2 * np.pi * (start * t + (end - start) * t ** 2 / (2 * duration))


def excitation(params):
    rate = params["frequency"]
    length = params["warmup"] + len(SWEEPS) * params["duration"]
    t = np.arange(int(np.rint(length * rate))) / rate

    phase = 2 * np.pi * params["tone"] * t
    offset = 2 * np.pi * params["tone"] * params["warmup"]
    for kind in SWEEPS:
        start = sweep_start(params, kind)
        during = t >= start
        phase[during] = offset + sweep_phase(params, kind, t[during] - start)
        offset += sweep_phase(params, kind, params["duration"])

    amplitude = 10 ** (params["level"] / 20) * 32767
    return np.rint(amplitude * np.sin(phase)).astype(np.int16)


class Adapter:
    def __init__(self, test):
        self.proc = subprocess.Popen([test], stdin=subprocess.PIPE,
                                     stdout=subprocess.PIPE)

        self.params = {}
        while True:
            line = self.proc.stdout.readline().decode()
            if not line:
                sys.exit("audioperf exited unexpectedly")
            if line == "# input\n":
                break
            _, _, name, value = line.split()
            self.params[name] = int(value)

    def read(self, size):
        data = self.proc.stdout.read(size)
        if len(data) != size:
            sys.exit("audioperf exited unexpectedly")
        return data

    def process(self, delay, samples):
        self.proc.stdin.write(struct.pack("=qI", delay, len(samples)))
        self.proc.stdin.write(samples.tobytes())
        self.proc.stdin.flush()
        error, frames = struct.unpack("=dI", self.read(12))
        return np.frombuffer(self.read(frames * 2), np.int16), error

    def flush(self):
        return self.process(0, np.empty(0, np.int16))

    def close(self):
        self.proc.stdin.close()
        if self.proc.wait() != 0:
            sys.exit("audioperf failed")


class LerpResampler:
    def __init__(self, params):
        self.step = 1 / (1 + params["drift"] / 1e6)
        self.target = params["target"]
        self.history = np.zeros(1)
        self.pos = 0.0

    def process(self, delay, samples):
        x = np.concatenate((self.history, samples))
        count = max(0, int(np.ceil((len(x) - 1 - self.pos) / self.step)))
        t = self.pos + np.arange(count) * self.step
        i = t.astype(int)
        f = t - i
        out = x[i] * (1 - f) + x[np.minimum(i + 1, len(x) - 1)] * f

        self.pos += count * self.step - (len(x) - 1)
        self.history = x[-1:]
        return np.rint(out).astype(np.int16), self.target - delay

    def flush(self):
        return np.empty(0, np.int16), np.nan


def resampler(params, adapter):
    if params["lerp_demo"]:
        return LerpResampler(params)
    return adapter


def simulate(params, adapter, until=None):
    rate = params["frequency"]
    sink_rate = rate * (1 + params["drift"] / 1e6)
    signal = excitation(params)
    if until is not None:
        signal = signal[:int(np.rint(until * rate))]
    random = np.random.default_rng(0)

    def frames(ms):
        return int(np.rint(ms * rate / 1000))

    # What the output has played, or has queued up to play: one channel
    # of the sample, and where in the input it came from, or NaN if it
    # is silence. It starts a new stream with silence.
    samples = [np.zeros(params["target"], np.int16)]
    positions = [np.full(params["target"], np.nan)]
    queued = params["target"]

    packet_times = []
    delay_errors = []
    owed = 0.0
    underruns = 0

    pos = 0
    while pos < len(signal):
        size = frames(params["packet"])
        if params["spread"] > 0:
            size = max(1, frames(params["packet"] +
                                 random.uniform(-params["spread"],
                                                params["spread"])))
        packet = signal[pos:pos + size]

        delay = queued
        if params["jitter"] > 0:
            delay = max(0, delay + frames(random.uniform(-params["jitter"],
                                                         params["jitter"])))

        if params["bypass"]:
            out = packet
            error = params["target"] - delay
        else:
            out, error = adapter.process(delay, packet)
        packet_times.append(pos / rate)
        delay_errors.append(error)

        # Close enough, as the analysis leaves room either side of the
        # tone for the resampler's own delay
        samples.append(out)
        positions.append(pos + (np.arange(len(out)) + 0.5) * len(packet) /
                         len(out))
        queued += len(out)

        # The output plays while the next packet is on its way
        owed += len(packet) * sink_rate / rate
        played = int(owed)
        owed -= played
        if played > queued:
            samples.append(np.zeros(played - queued, np.int16))
            positions.append(np.full(played - queued, np.nan))
            underruns += played - queued
            queued = 0
        else:
            queued -= played

        pos += len(packet)

    if not params["bypass"]:
        tail, _ = adapter.flush()
        samples.append(tail)
        positions.append(np.full(len(tail), np.nan))

    return {"samples": np.concatenate(samples),
            "positions": np.concatenate(positions),
            "packet_times": np.array(packet_times),
            "delay_errors": np.array(delay_errors),
            "underruns": underruns}


OPPOSITE_PREFIX = "opposite_"


def save(path, params, run, opposite):
    arrays = dict(run)
    if opposite is not None:
        arrays.update((OPPOSITE_PREFIX + name, value)
                      for name, value in opposite.items())
    with open(path, "wb") as f:
        np.savez(f, params=json.dumps(params), **arrays)


def load(file):
    with np.load(file) as f:
        run = {}
        opposite = {}
        for name in f.files:
            if name == "params":
                continue
            if name.startswith(OPPOSITE_PREFIX):
                opposite[name[len(OPPOSITE_PREFIX):]] = f[name]
            else:
                run[name] = f[name]
        return json.loads(str(f["params"])), run, opposite or None


def to_dbfs(mean_square):
    # Relative to a full scale sine
    with np.errstate(divide="ignore"):
        return 10 * np.log10(np.asarray(mean_square) / 0.5)


def quantization_floor():
    return to_dbfs((1 / 32768) ** 2 / 12 * 2 / FFT_SIZE)


def short_time_fft(rate):
    window = scipy.signal.windows.blackmanharris(FFT_SIZE, sym=False)
    return scipy.signal.ShortTimeFFT(window, FFT_HOP, rate,
                                     fft_mode="onesided2X", scale_to="psd")


def spectra(sft, run, slices):
    samples = run["samples"] / 32768
    for first in range(0, len(slices), FFT_CHUNK):
        rows = slice(first, first + FFT_CHUNK)
        chosen = slices[rows]
        spectrum = sft.stft(samples, chosen[0], chosen[-1] + 1)
        yield rows, spectrum[:, chosen - chosen[0]].T * np.sqrt(sft.delta_f)


def band_edges(rate, kind):
    if kind == "logarithmic":
        return SPECTROGRAM_LOW * (rate / 2 / SPECTROGRAM_LOW) ** \
            (np.arange(SPECTROGRAM_BANDS + 1) / SPECTROGRAM_BANDS)
    return np.linspace(0, rate / 2, SPECTROGRAM_BANDS + 1)


def band_levels(sft, spectrum, edges):
    edges = edges / sft.delta_f
    low, high = edges[:-1], edges[1:]
    first = np.ceil(low).astype(int)
    last = np.floor(high).astype(int)
    narrow = first > last
    first[narrow] = last[narrow] = np.rint(np.sqrt(low * high))[narrow]

    power = np.abs(spectrum) ** 2
    levels = np.stack([power[:, f:l + 1].max(axis=1)
                       for f, l in zip(first, last)], axis=1)
    return np.maximum(to_dbfs(levels), -200)


def analysis_windows(sft, run):
    positions = run["positions"]
    slices = np.arange(sft.lower_border_end[1],
                       sft.upper_border_begin(len(positions))[1])
    starts = slices * FFT_HOP - sft.m_num_mid

    # Silence before the stream starts, or after it ends, would be
    # counted as distortion
    silent = np.concatenate(([0], np.cumsum(np.isnan(positions))))
    valid = silent[starts + FFT_SIZE] == silent[starts]

    return slices, valid, positions[starts], \
        positions[starts + FFT_SIZE - 1]


def analyse_warmup(params, run, notes):
    rate = params["frequency"]
    sink_rate = rate * (1 + params["drift"] / 1e6)
    tone = params["tone"]
    sft = short_time_fft(rate)
    tone_bin = int(np.rint(tone / sft.delta_f))

    slices, valid, _, last = analysis_windows(sft, run)
    past = np.flatnonzero(valid & (last >= params["warmup"] * rate -
                                   SECTION_MARGIN))
    if len(past) > 0:
        valid = valid[:past[0]]
    slices = slices[np.flatnonzero(valid)]

    phasor = np.empty(len(slices), complex)
    for rows, spectrum in spectra(sft, run, slices):
        phasor[rows] = spectrum[:, tone_bin]

    # How far the phase moved between two windows, beyond what the
    # bin's own frequency accounts for, is how far the tone is from it
    delta = np.angle(phasor[1:] * np.conj(phasor[:-1]) *
                     np.exp(-2j * np.pi * tone_bin * FFT_HOP / FFT_SIZE))
    freq = (tone_bin + delta * FFT_SIZE / (2 * np.pi * FFT_HOP)) / \
        FFT_SIZE * sink_rate
    deviation = (freq / tone - 1) * 1e6

    following = np.diff(slices) == 1
    time = slices[1:][following] * FFT_HOP / sink_rate
    packet = np.searchsorted(run["packet_times"], time, side="right") - 1
    delay = -run["delay_errors"][np.maximum(packet, 0)]
    deviation = deviation[following]
    if len(time) == 0:
        return None

    notes.append("warmup at %+g ppm: deviation at end %+.2f ppm, delay at "
                 "end %+.0f frames from target"
                 % (params["drift"], deviation[-1], delay[-1]))

    return {"drift": params["drift"], "time": time, "deviation": deviation,
            "delay": delay}


def analyse_sweep(params, run, kind, at):
    rate = params["frequency"]
    sink_rate = rate * (1 + params["drift"] / 1e6)
    sft = short_time_fft(rate)
    begin = sweep_start(params, kind)
    end = begin + params["duration"]

    slices, valid, first, last = analysis_windows(sft, run)
    chosen = valid & (first >= begin * rate + SECTION_MARGIN) & \
        (last < end * rate - SECTION_MARGIN)
    slices = slices[chosen]
    if len(slices) == 0:
        return None

    bands = band_edges(rate, kind)
    levels = np.empty((len(slices), SPECTROGRAM_BANDS))
    for rows, spectrum in spectra(sft, run, slices):
        levels[rows] = band_levels(sft, spectrum, bands)

    time = slices * FFT_HOP / sink_rate - begin
    data = {"kind": kind, "time": time, "levels": levels, "bands": bands}

    if at is not None:
        index = np.argmin(np.abs(time - at))
        _, spectrum = next(spectra(sft, run, slices[index:index + 1]))
        data["slice"] = {
            "time": time[index],
            "freq": sft.f,
            "level": np.maximum(to_dbfs(np.abs(spectrum[0]) ** 2), -200),
        }

    return data


def style_axis(ax, theme):
    ax.set_facecolor(theme["surface"])
    ax.grid(True, color=theme["grid"], linewidth=0.8)
    ax.tick_params(which="both", colors=theme["muted"], labelsize=9)
    for spine in ax.spines.values():
        spine.set_visible(False)
    ax.xaxis.label.set_color(theme["text"])
    ax.yaxis.label.set_color(theme["text"])


def title(fig, params, theme, what, drift=None):
    if params.get("bypass"):
        name = "Bypassed"
    elif params.get("lerp_demo"):
        name = "Lerp resampler"
    else:
        name = "AudioAdapter"
    if drift is None:
        drift = "%g" % params["drift"]
    fig.suptitle("%s %s with %s ppm clock drift (%g ms packets, %g dBFS)"
                 % (name, what, drift, params["packet"], params["level"]),
                 color=theme["text"], fontsize=12, x=0.01, ha="left")


def label_panel(ax, theme, what, unit):
    ax.set_title(what, loc="left", color=theme["text"], fontsize=10)
    ax.set_ylabel(unit)


def plot_warmup(params, data, theme, width, height):
    dpi = 100
    fig, (tone, delay) = plt.subplots(
        2, 1, sharex=True, figsize=(width / dpi, height / dpi), dpi=dpi,
        layout="constrained", gridspec_kw={"height_ratios": [3, 1.2]})
    fig.get_layout_engine().set(hspace=PANEL_GAP)
    fig.patch.set_facecolor(theme["surface"])

    for ax in (tone, delay):
        style_axis(ax, theme)

    drift = None
    if len(data) > 1:
        drift = "\u00b1%g" % abs(params["drift"])
    title(fig, params, theme, "%g Hz tone warming up" % params["tone"], drift)

    for run, colour in zip(data, (theme["sweep"], theme["opposite"])):
        label = "%+g ppm drift" % run["drift"]
        tone.plot(run["time"], run["deviation"], color=colour, linewidth=2,
                  label=label)
        delay.plot(run["time"], run["delay"] / params["frequency"] * 1000,
                   color=colour, linewidth=2, label=label)

    tone.axhline(0, color=theme["reference"], linewidth=1, linestyle="--")
    label_panel(tone, theme, "Tone frequency deviation", "ppm")
    if len(data) > 1:
        legend = tone.legend(loc="upper right", fontsize=9,
                             facecolor=theme["surface"],
                             edgecolor=theme["grid"],
                             labelcolor=theme["text"])
        legend.get_frame().set_alpha(1)

    delay.axhline(0, color=theme["reference"], linewidth=1, linestyle="--")
    label_panel(delay, theme, "Delay error", "ms")
    delay.set_xlabel("Time since stream start (s)")
    delay.set_xlim(0, params["warmup"])


    fig.align_ylabels()

    return fig


def edges(centres):
    middle = (centres[1:] + centres[:-1]) / 2
    return np.concatenate(([2 * centres[0] - middle[0]], middle,
                           [2 * centres[-1] - middle[-1]]))


def plot_sweep(params, data, theme, width, height):
    dpi = 100
    sliced = data.get("slice")
    if sliced is None:
        fig, (sweep, scale) = plt.subplots(
            1, 2, figsize=(width / dpi, height / dpi), dpi=dpi,
            layout="constrained", gridspec_kw={"width_ratios": [60, 1]})
    else:
        fig, ((sweep, scale), (response, unused)) = plt.subplots(
            2, 2, figsize=(width / dpi, height / dpi), dpi=dpi,
            layout="constrained",
            gridspec_kw={"height_ratios": [3, 1.5], "width_ratios": [60, 1]})
        unused.set_visible(False)
    fig.get_layout_engine().set(hspace=PANEL_GAP)
    fig.patch.set_facecolor(theme["surface"])
    style_axis(sweep, theme)
    sweep.grid(False)

    title(fig, params, theme, "%s sine sweep %g s into the stream"
          % (data["kind"], sweep_start(params, data["kind"])))

    logarithmic = data["kind"] == "logarithmic"
    low = params["start"] if logarithmic else 0
    high = data["bands"][-1]

    ramp = LinearSegmentedColormap.from_list("ramp", theme["ramp"])
    levels = data["levels"]
    vmin = np.floor(quantization_floor() / 5) * 5
    vmax = np.ceil(levels.max() / 5) * 5
    mesh = sweep.pcolormesh(edges(data["time"]), data["bands"],
                            levels.T, cmap=ramp, vmin=vmin, vmax=vmax,
                            shading="flat", rasterized=True)
    if logarithmic:
        sweep.set_yscale("log")
    sweep.set_ylim(low, high)
    label_panel(sweep, theme, "Output spectrum", "Hz")
    sweep.set_xlabel("Time since sweep start (s)")

    if sliced is not None:
        sweep.axvline(sliced["time"], color=theme["reference"], linewidth=1,
                      linestyle="--")

        style_axis(response, theme)
        response.plot(sliced["freq"], sliced["level"], color=theme["sweep"],
                      linewidth=1)
        if logarithmic:
            response.set_xscale("log")
        response.set_xlim(low, high)
        response.set_ylim(vmin, vmax)
        label_panel(response, theme, "Output spectrum at %.2f s"
                    % sliced["time"], "dBFS")
        response.set_xlabel("Hz")

    bar = fig.colorbar(mesh, cax=scale)
    bar.outline.set_visible(False)
    bar.ax.tick_params(colors=theme["muted"], labelsize=9)
    bar.set_label("dBFS", color=theme["text"])

    fig.align_ylabels()

    return fig


def terminal_size():
    # Pixel size of the terminal, if it is willing to say
    for stream in (sys.stdout, sys.stderr, sys.stdin):
        try:
            packed = fcntl.ioctl(stream.fileno(), termios.TIOCGWINSZ,
                                 b"\0" * 8)
        except (OSError, ValueError, io.UnsupportedOperation):
            continue
        rows, cols, xpixels, ypixels = struct.unpack("HHHH", packed)
        if xpixels > 0 and ypixels > 0:
            return xpixels, ypixels, rows
    return None


def to_sixel(image):
    # Sixel only has registers for a limited palette
    image = image.convert("RGB").quantize(colors=255,
                                          method=Image.Quantize.MEDIANCUT)
    palette = image.getpalette()
    pixels = np.asarray(image)
    height, width = pixels.shape

    out = ["\x1bP0;1;0q", '"1;1;%d;%d' % (width, height)]
    for index in np.unique(pixels).tolist():
        r, g, b = palette[index * 3:index * 3 + 3]
        out.append("#%d;2;%d;%d;%d" % (index, r * 100 // 255,
                                       g * 100 // 255, b * 100 // 255))

    weights = (1 << np.arange(6)).reshape(6, 1)
    for top in range(0, height, 6):
        band = pixels[top:top + 6]
        rows = band.shape[0]
        first = True

        for index in np.unique(band).tolist():
            bits = ((band == index) * weights[:rows]).sum(axis=0)
            used = np.flatnonzero(bits)
            # Trailing blank columns need not be sent
            bits = bits[:used[-1] + 1]

            if not first:
                out.append("$")
            first = False
            out.append("#%d" % index)

            # Run length encode, one run per change in value
            starts = np.concatenate(
                ([0], np.flatnonzero(np.diff(bits)) + 1))
            ends = np.append(starts[1:], len(bits))
            for start, end in zip(starts, ends):
                char = chr(63 + int(bits[start]))
                count = end - start
                if count > 3:
                    out.append("!%d%s" % (count, char))
                else:
                    out.append(char * count)

        out.append("-")

    out.append("\x1b\\")
    return "".join(out)


def with_suffix(path, suffix):
    root, ext = os.path.splitext(path)
    return root + suffix + ext


def main():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--test", metavar="PATH",
                        help="the audioperf binary to run")
    parser.add_argument("--input", metavar="FILE",
                        help="plot what was kept with --save instead of "
                             "running audioperf ('-' for stdin)")
    parser.add_argument("--save", metavar="FILE",
                        help="also keep the parameters and what the "
                             "output played in this file")
    destination = parser.add_mutually_exclusive_group()
    destination.add_argument("--output", metavar="FILE",
                             help="write the plots to FILE-warmup, "
                                  "FILE-logarithmic-sweep and "
                                  "FILE-linear-sweep instead of the terminal")
    destination.add_argument("--combined", metavar="FILE",
                             help="write all the plots, one above the other, "
                                  "to a single PNG that is %d pixels wide"
                                  % COMBINED_WIDTH)
    parser.add_argument("--theme", choices=THEMES.keys(), default="dark",
                        help="colours to plot with (default: dark)")
    parser.add_argument("--size", metavar="WxH",
                        help="plot size in pixels (default: fit the "
                             "terminal); with --combined, only the height "
                             "is used")
    parser.add_argument("--warmup", metavar="S", type=float, default=120,
                        help="how long to play a constant tone before the "
                             "sweeps (default: 120)")
    parser.add_argument("--tone", metavar="HZ", type=float, default=1000,
                        help="frequency of that tone (default: 1000)")
    parser.add_argument("--duration", metavar="S", type=float, default=30,
                        help="length of each sweep (default: 30)")
    parser.add_argument("--start", metavar="HZ", type=float, default=20,
                        help="sweep start frequency (default: 20)")
    parser.add_argument("--end", metavar="HZ", type=float, default=20000,
                        help="sweep end frequency (default: 20000)")
    parser.add_argument("--level", metavar="DBFS", type=float, default=-3,
                        help="level of the sine (default: -3)")
    parser.add_argument("--drift", metavar="PPM", type=float, default=200,
                        help="how much faster the output's clock is than "
                             "the server's (default: 200)")
    parser.add_argument("--packet", metavar="MS", type=float, default=10,
                        help="audio per server message (default: 10)")
    parser.add_argument("--packet-spread", metavar="MS", type=float,
                        default=0, dest="spread",
                        help="vary each message's length randomly by up "
                             "to this much (default: 0)")
    parser.add_argument("--delay-jitter", metavar="MS", type=float,
                        default=0, dest="jitter",
                        help="vary the delay the output reports randomly "
                             "by up to this much (default: 0)")
    parser.add_argument("--bypass", action="store_true",
                        help="skip the adapter, to see what the "
                             "measurement itself adds")
    parser.add_argument("--lerp-demo", action="store_true",
                        help="use a simple linear interpolation resampler, "
                             "running at exactly the drift, instead of the "
                             "adapter")
    parser.add_argument("--slice", metavar="S", type=float,
                        help="also plot the spectrum this far into each "
                             "sweep")
    args = parser.parse_args()

    if args.warmup < 0 or args.duration <= 0 or args.tone <= 0 or \
            args.start <= 0 or args.end <= 0 or args.packet <= 0 or \
            args.spread < 0 or args.jitter < 0:
        parser.error("durations and frequencies must be positive")

    if args.bypass and args.lerp_demo:
        parser.error("--bypass and --lerp-demo cannot be combined")

    if args.input == "-":
        params, run, opposite = load(io.BytesIO(sys.stdin.buffer.read()))
    elif args.input is not None:
        params, run, opposite = load(args.input)
    else:
        test = find_test(args.test)
        adapter = Adapter(test)
        params = dict(adapter.params)
        params.update((name, getattr(args, name)) for name in
                      ("warmup", "tone", "duration", "start", "end",
                       "level", "drift", "packet", "spread", "jitter",
                       "bypass", "lerp_demo"))
        if max(params["tone"], params["end"]) >= params["frequency"] / 2:
            parser.error("frequencies must be below %g Hz"
                         % (params["frequency"] / 2))
        run = simulate(params, resampler(params, adapter))
        adapter.close()

        opposite = None
        if params["warmup"] > 0 and params["drift"] != 0:
            opposite_params = dict(params, drift=-params["drift"])
            adapter = Adapter(test)
            opposite = simulate(opposite_params,
                                resampler(opposite_params, adapter),
                                params["warmup"])
            adapter.close()

    if args.save is not None:
        save(args.save, params, run, opposite)

    if args.slice is not None and \
            not 0 <= args.slice <= params["duration"]:
        parser.error("slice must be within the %g s sweeps"
                     % params["duration"])

    notes = []
    warmup = None
    if params["warmup"] > 0:
        warmup = [analyse_warmup(params, run, notes)]
        if opposite is not None:
            warmup.append(analyse_warmup(dict(params,
                                              drift=-params["drift"]),
                                         opposite, notes))
        warmup = [data for data in warmup if data is not None] or None
    sweeps = [analyse_sweep(params, run, kind, args.slice) for kind in SWEEPS]
    notes.append("underruns: %d frames" % run["underruns"])

    if warmup is None and not any(sweeps):
        sys.exit("No measurements in audioperf output")

    size = None
    if args.size is not None:
        size = tuple(int(v) for v in args.size.lower().split("x"))

    if args.combined is not None:
        width = COMBINED_WIDTH
        height = COMBINED_WIDTH * 3 // 4 if size is None else size[1]
    elif size is not None:
        width, height = size
    else:
        width, height = 1200, 900
        if args.output is None:
            size = terminal_size()
            if size is not None:
                xpixels, ypixels, rows = size
                width = min(xpixels, 1600)
                # Leave room for the summary and the prompt
                height = max(300, min(int(width * 0.75),
                                      ypixels - 6 * ypixels // max(rows, 1)))

    plots = [("warmup", plot_warmup, warmup)]
    for kind, sweep in zip(SWEEPS, sweeps):
        plots.append((kind + "-sweep", plot_sweep, sweep))
    images = []
    for name, plot, plot_data in plots:
        if plot_data is None:
            continue

        fig = plot(params, plot_data, THEMES[args.theme], width, height)

        if args.output is not None:
            fig.savefig(with_suffix(args.output, "-" + name),
                        facecolor=fig.get_facecolor())
        else:
            buf = io.BytesIO()
            fig.savefig(buf, format="png", facecolor=fig.get_facecolor())
            buf.seek(0)
            image = Image.open(buf).convert("RGB")
            if args.combined is not None:
                images.append(image)
            else:
                sys.stdout.write(to_sixel(image))
                sys.stdout.write("\n")
        plt.close(fig)

    if args.combined is not None:
        combined = Image.new("RGB", (width, sum(i.height for i in images) +
                                     COMBINED_GAP * (len(images) - 1)),
                             THEMES[args.theme]["surface"])
        top = 0
        for image in images:
            combined.paste(image, (0, top))
            top += image.height + COMBINED_GAP
        combined.save(args.combined, format="PNG")

    for note in notes:
        print(note)


if __name__ == "__main__":
    main()

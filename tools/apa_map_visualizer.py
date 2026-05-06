#!/usr/bin/env python3
"""
APA Map Log Parser and Visualizer
基于参考实现map_replay.py重写
"""

import re
import json
import argparse
import sys
import matplotlib.pyplot as plt

plt.rcParams['font.sans-serif'] = ['SimHei', 'Microsoft YaHei', 'PingFang SC', 'STHeiti', 'sans-serif']
plt.rcParams['axes.unicode_minus'] = False


class APALogParser:
    def __init__(self, filepath):
        self.filepath = filepath
        self.frames = []

    def _create_empty_frame(self):
        return {
            'frame_id': -1,
            'Obj1': {'BSeg': [], 'NSeg': [], 'FusSeg': []},
            'Obj2': {'BSeg': [], 'NSeg': [], 'FusSeg': []},
            'Sub':  {'BSeg': [], 'NSeg': [], 'FusSeg': []}
        }

    def parse(self):
        try:
            with open(self.filepath, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
        except Exception as e:
            print(f"Error reading file: {e}")
            sys.exit(1)

        chunks = content.split("Current frame end.")

        for chunk in chunks:
            frame_match = re.search(r'## Start Runonce \[(\d+)\]', chunk)
            if not frame_match:
                continue
            frame_id = int(frame_match.group(1))

            current_frame = self._create_empty_frame()
            current_frame['frame_id'] = frame_id

            i_obj1 = chunk.find("==FSDFusionObj1Success==")
            i_obj2 = chunk.find("==FSDFusionObj2Success==")
            i_sub = chunk.find("==FSDFusionSubLaneSuccess==")
            if i_sub == -1:
                i_sub = chunk.find("==FSDFusionSubSuccess==")

            text_obj1 = chunk[0:i_obj1 if i_obj1 != -1 else len(chunk)]
            text_obj2 = chunk[i_obj1:i_obj2 if i_obj2 != -1 else len(chunk)] if i_obj1 != -1 else ""
            text_sub = chunk[i_obj2:i_sub if i_sub != -1 else len(chunk)] if i_obj2 != -1 else ""

            current_frame['Obj1']['BSeg'] = self._get_seg_pts(text_obj1, "=BSegData==")
            current_frame['Obj1']['NSeg'] = self._get_seg_pts(text_obj1, "=NSegData==")
            current_frame['Obj1']['FusSeg'] = self._get_seg_pts(text_obj1, "=FusSegData==")

            current_frame['Obj2']['BSeg'] = self._get_seg_pts(text_obj2, "=BSegData==")
            current_frame['Obj2']['NSeg'] = self._get_seg_pts(text_obj2, "=NSegData==")
            current_frame['Obj2']['FusSeg'] = self._get_seg_pts(text_obj2, "=FusSegData==")

            current_frame['Sub']['BSeg'] = self._get_seg_pts(text_sub, "=BSegData==")
            current_frame['Sub']['NSeg'] = self._get_seg_pts(text_sub, "=NSegData==")
            current_frame['Sub']['FusSeg'] = self._get_seg_pts(text_sub, "=FusSegData==")

            if self._has_data(current_frame):
                self.frames.append(current_frame)

        print(f"Parse Complete! Extracted {len(self.frames)} valid frames.")
        return self.frames

    def _get_seg_pts(self, block, seg_name):
        pt_pattern = re.compile(r'\((-?\d+\.?\d*|-?nan),\s*(-?\d+\.?\d*|-?nan)\)')
        starts = [m.start() for m in re.finditer(seg_name, block)]
        pts = []

        for idx in starts:
            next_segs = ["=BSegData==", "=NSegData==", "=FusLineIndex==", "=Fusiondebug==", "=FusSegData==", "==FSD"]
            end_idx = len(block)
            for n_seg in next_segs:
                if n_seg == seg_name:
                    continue
                ni = block.find(n_seg, idx + len(seg_name))
                if ni != -1 and ni < end_idx:
                    end_idx = ni

            buffer = block[idx:end_idx]

            source = "FSD"
            if "PDC" in buffer.upper():
                source = "PDC"
            elif "SDG" in buffer.upper():
                source = "SDG"

            matches = pt_pattern.findall(buffer)
            for mx, my in matches:
                if 'nan' not in mx.lower() and 'nan' not in my.lower():
                    fx, fy = float(mx), float(my)
                    if abs(fx) < 100000 and abs(fy) < 100000:
                        pts.append({"x": fx / 1000.0, "y": fy / 1000.0, "source": source})
        return pts

    def _has_data(self, frame):
        for obj in ['Obj1', 'Obj2', 'Sub']:
            if len(frame[obj]['BSeg']) > 0 or len(frame[obj]['NSeg']) > 0 or len(frame[obj]['FusSeg']) > 0:
                return True
        return False


class APAVisualizer:
    def __init__(self, frames, start_idx=0):
        self.frames = frames
        if not self.frames:
            print("Error: No point data parsed from log!")
            sys.exit(1)

        self.current_idx = max(0, min(start_idx, len(self.frames) - 1))

        self.view_xlim = None
        self.view_ylim = None

        self.color_palette = {
            'Obj1': {'BSeg': '#FF9999', 'NSeg': '#FF0000', 'FusSeg': '#8B0000'},  # 红
            'Obj2': {'BSeg': '#99CCFF', 'NSeg': '#0088FF', 'FusSeg': '#00008B'},  # 蓝
            'Sub':  {'BSeg': '#99FF99', 'NSeg': '#00AA00', 'FusSeg': '#004400'}   # 绿
        }

        self.fig, self.ax = plt.subplots(figsize=(12, 8))
        self.fig.canvas.mpl_connect('key_press_event', self.on_press)
        self.fig.canvas.mpl_connect('scroll_event', self.on_scroll)

        self.update_plot()

    def on_press(self, event):
        if event.key in ['right', ' ', 'down']:
            self.current_idx = min(self.current_idx + 1, len(self.frames) - 1)
            self.update_plot()
        elif event.key in ['left', 'up']:
            self.current_idx = max(self.current_idx - 1, 0)
            self.update_plot()
        elif event.key == 'home':
            self.current_idx = 0
            self.update_plot()
        elif event.key == 'end':
            self.current_idx = len(self.frames) - 1
            self.update_plot()
        elif event.key == 'r':
            self.view_xlim = None
            self.view_ylim = None
            self.update_plot()
        elif event.key == 'q':
            plt.close()

    def on_scroll(self, event):
        if event.inaxes != self.ax:
            return

        base_scale = 1.3

        if event.button == 'up' or (hasattr(event, 'step') and event.step > 0):
            scale_factor = 1 / base_scale
        elif event.button == 'down' or (hasattr(event, 'step') and event.step < 0):
            scale_factor = base_scale
        else:
            return

        cur_xlim = self.ax.get_xlim()
        cur_ylim = self.ax.get_ylim()

        xdata = event.xdata
        ydata = event.ydata

        new_width = (cur_xlim[1] - cur_xlim[0]) * scale_factor
        new_height = (cur_ylim[1] - cur_ylim[0]) * scale_factor

        relx = (cur_xlim[1] - xdata) / (cur_xlim[1] - cur_xlim[0])
        rely = (cur_ylim[1] - ydata) / (cur_ylim[1] - cur_ylim[0])

        self.view_xlim = [xdata - new_width * (1 - relx), xdata + new_width * relx]
        self.view_ylim = [ydata - new_height * (1 - rely), ydata + new_height * rely]

        self.ax.set_xlim(self.view_xlim)
        self.ax.set_ylim(self.view_ylim)
        self.fig.canvas.draw_idle()

    def update_plot(self):
        self.ax.clear()
        frame = self.frames[self.current_idx]

        title = (f"Frame ID: {frame['frame_id']} | Progress: {self.current_idx + 1}/{len(self.frames)}\n"
                 f"Keys: [<-/->] Flip | [Scroll] Zoom | [R] Reset View | [Q] Quit")
        self.ax.set_title(title, fontsize=11, pad=10, fontweight='bold')

        layers = [
            ('BSeg',   'Old',    15),
            ('FusSeg', 'Fusion', 30),
            ('NSeg',   'New',    40)
        ]

        for target in ['Obj1', 'Obj2', 'Sub']:
            for seg_type, label_prefix, size in layers:
                pts = frame[target][seg_type]
                if not pts:
                    continue

                c_hex = self.color_palette[target][seg_type]

                xs_line = [p['x'] for p in pts]
                ys_line = [p['y'] for p in pts]

                line_style = '--' if seg_type == 'BSeg' else '-'
                line_alpha = 0.4 if seg_type == 'BSeg' else 0.7
                self.ax.plot(xs_line, ys_line, color=c_hex, linestyle=line_style,
                            linewidth=1.2, alpha=line_alpha, zorder=1)

                source_groups = {}
                for p in pts:
                    source_groups.setdefault(p['source'], {'x': [], 'y': []})
                    source_groups[p['source']]['x'].append(p['x'])
                    source_groups[p['source']]['y'].append(p['y'])

                for src, coords in source_groups.items():
                    if seg_type == 'NSeg':
                        self.ax.scatter(coords['x'], coords['y'],
                                        color='gold', alpha=0.5,
                                        marker='.', s=size * 4,
                                        edgecolors='none', zorder=2)
                        ec = '#FFD700'
                        lw = 1.0
                        zo = 4
                        actual_size = size * 1.5
                    elif seg_type == 'FusSeg':
                        ec = 'black'
                        lw = 0.5
                        zo = 3
                        actual_size = size
                    else:
                        ec = 'none'
                        lw = 0
                        zo = 2
                        actual_size = size

                    self.ax.scatter(coords['x'], coords['y'],
                                    c=c_hex, alpha=1.0,
                                    marker='.', s=actual_size,
                                    edgecolors=ec, linewidths=lw, zorder=zo,
                                    label=f"{target} {label_prefix}({src})")

        self.ax.set_xlabel("X (m)", fontweight='bold')
        self.ax.set_ylabel("Y (m)", fontweight='bold')

        if self.view_xlim is not None and self.view_ylim is not None:
            self.ax.set_xlim(self.view_xlim)
            self.ax.set_ylim(self.view_ylim)
        else:
            self.ax.axis('equal')

        self.ax.grid(True, linestyle='--', alpha=0.4)

        handles, labels = self.ax.get_legend_handles_labels()
        unique = dict(zip(labels, handles))
        if unique:
            self.ax.legend(unique.values(), unique.keys(),
                          loc='upper left', bbox_to_anchor=(1.02, 1), fontsize=9, markerscale=2)

        plt.tight_layout()
        plt.draw()

    def show(self):
        plt.show()


def main():
    parser = argparse.ArgumentParser(description="APA Log Map Visualizer")
    parser.add_argument('log_file', help="Path to the log file")
    parser.add_argument('-o', '--output', default=None, help="Export parsed data to a JSON file")
    parser.add_argument('-f', '--frame', type=int, default=0, help="Start from specified frame index (0-based)")
    args = parser.parse_args()

    parser_obj = APALogParser(args.log_file)
    frames = parser_obj.parse()

    if args.output:
        try:
            with open(args.output, 'w', encoding='utf-8') as f:
                json.dump(frames, f, indent=2, ensure_ascii=False)
            print(f"Exported to JSON: {args.output}")
        except Exception as e:
            print(f"Failed to export JSON: {e}")

    if frames:
        print("\nEngine Started!")
        print("Controls: ")
        print("  - [<-] / [->] or [Space]: Prev / Next")
        print("  - Mouse Scroll: Zoom In / Zoom Out")
        print("  - [R]: Reset Zoom View")
        print("  - [Home] / [End]: First / Last")
        print("  - [Q]: Quit")
        visualizer = APAVisualizer(frames, start_idx=args.frame)
        visualizer.show()


if __name__ == "__main__":
    main()
import argparse
import json
import logging
import math
import re
import sys
from dataclasses import dataclass, field
from typing import Optional

import matplotlib
import matplotlib.pyplot as plt

# ==========================================
# 屏蔽 Matplotlib 烦人的字体找不到警告
# ==========================================
logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)
plt.rcParams["axes.unicode_minus"] = False

# ==================== 类型定义 ====================

@dataclass
class Point:
    """二维坐标点，支持来源标注"""
    x: float
    y: float
    source: str = ""


@dataclass
class Segment:
    """线段段：包含 BSeg/NSeg/FusSeg 三类点集合"""
    BSeg: list[Point] = field(default_factory=list)
    NSeg: list[Point] = field(default_factory=list)
    FusSeg: list[Point] = field(default_factory=list)


@dataclass
class Frame:
    """单帧数据：包含 frame_id 和 Obj1/Obj2/Sub 三个对象的线段"""
    frame_id: int
    Obj1: Segment = field(default_factory=Segment)
    Obj2: Segment = field(default_factory=Segment)
    Sub: Segment = field(default_factory=Segment)


class APALogParser:
    """APA 日志解析器，将日志文件转换为帧数据列表"""

    # 分段标记的优先级顺序（决定解析范围）
    _SEG_MARKERS = [
        "=BSegData==",
        "=NSegData==",
        "=FusLineIndex==",
        "=Fusiondebug==",
        "=FusSegData==",
        "==FSD",
    ]

    def __init__(self, filepath: str):
        self.filepath = filepath
        self.frames: list[Frame] = []

    def _parse_pts(self, block: str, marker: str) -> list[Point]:
        """从指定标记的文本块中提取坐标点"""
        pt_pattern = re.compile(r"\((-?\d+\.?\d*|-?nan),\s*(-?\d+\.?\d*|-?nan)\)")
        starts = [m.start() for m in re.finditer(marker, block)]
        pts = []

        for idx in starts:
            # 确定当前标记的解析范围（到下一个标记为止）
            end_idx = len(block)
            for seg in self._SEG_MARKERS:
                if seg == marker:
                    continue
                ni = block.find(seg, idx + len(marker))
                if ni != -1 and ni < end_idx:
                    end_idx = ni

            buffer = block[idx:end_idx]
            # 判断数据来源
            source = "FSD"
            if "PDC" in buffer.upper():
                source = "PDC"
            elif "SDG" in buffer.upper():
                source = "SDG"

            for mx, my in pt_pattern.findall(buffer):
                if "nan" in mx.lower() or "nan" in my.lower():
                    continue
                fx, fy = float(mx), float(my)
                # 过滤异常值和原点占位符
                if abs(fx) > 100000 or abs(fy) > 100000:
                    continue
                if abs(fx) < 1e-5 and abs(fy) < 1e-5:
                    continue
                pts.append(Point(x=fx / 1000.0, y=fy / 1000.0, source=source))
        return pts

    def _has_valid_data(self, frame: Frame) -> bool:
        """检查帧是否有有效数据"""
        for seg in [frame.Obj1, frame.Obj2, frame.Sub]:
            if seg.BSeg or seg.NSeg or seg.FusSeg:
                return True
        return False

    def parse(self) -> list[Frame]:
        """解析日志文件，返回帧列表"""
        try:
            with open(self.filepath, "r", encoding="utf-8", errors="ignore") as f:
                content = f.read()
        except Exception as e:
            print(f"Error reading file: {e}")
            sys.exit(1)

        frames = []
        for chunk in content.split("Current frame end."):
            frame_match = re.search(r"## Start Runonce \[(\d+)\]", chunk)
            if not frame_match:
                continue

            frame_id = int(frame_match.group(1))
            i_obj1 = chunk.find("==FSDFusionObj1Success==")
            i_obj2 = chunk.find("==FSDFusionObj2Success==")
            i_sub = chunk.find("==FSDFusionSubLaneSuccess==")
            if i_sub == -1:
                i_sub = chunk.find("==FSDFusionSubSuccess==")

            # 各对象文本区间
            end = len(chunk)
            text_obj1 = chunk[:i_obj1] if i_obj1 != -1 else chunk[:end]
            text_obj2 = chunk[i_obj1:i_obj2] if i_obj1 != -1 and i_obj2 != -1 else ""
            text_sub = chunk[i_obj2:i_sub] if i_obj2 != -1 and i_sub != -1 else ""

            frame = Frame(
                frame_id=frame_id,
                Obj1=Segment(
                    BSeg=self._parse_pts(text_obj1, "=BSegData=="),
                    NSeg=self._parse_pts(text_obj1, "=NSegData=="),
                    FusSeg=self._parse_pts(text_obj1, "=FusSegData=="),
                ),
                Obj2=Segment(
                    BSeg=self._parse_pts(text_obj2, "=BSegData=="),
                    NSeg=self._parse_pts(text_obj2, "=NSegData=="),
                    FusSeg=self._parse_pts(text_obj2, "=FusSegData=="),
                ),
                Sub=Segment(
                    BSeg=self._parse_pts(text_sub, "=BSegData=="),
                    NSeg=self._parse_pts(text_sub, "=NSegData=="),
                    FusSeg=self._parse_pts(text_sub, "=FusSegData=="),
                ),
            )

            if self._has_valid_data(frame):
                frames.append(frame)

        self.frames = frames
        print(f"Parse Complete! Extracted {len(self.frames)} valid frames.")
        return self.frames


class HoverPoint:
    """悬停点的数据结构"""
    def __init__(self, x: float, y: float, target: str, seg: str, source: str):
        self.x = x
        self.y = y
        self.target = target
        self.seg = seg
        self.source = source


@dataclass
class PanState:
    """鼠标拖拽平移状态"""
    active: bool = False
    start_pixel: tuple = field(default_factory=lambda: (0, 0))
    start_xlim: tuple = field(default_factory=lambda: (0, 0))
    start_ylim: tuple = field(default_factory=lambda: (0, 0))


class APAVisualizer:
    """APA 可视化器，支持帧浏览、缩放、平移和悬停提示"""

    # 配色方案：Obj1(红)、Obj2(蓝)、Sub(绿)
    _COLORS = {
        "Obj1": {"BSeg": "#FF9999", "NSeg": "#FF0000", "FusSeg": "#8B0000"},
        "Obj2": {"BSeg": "#99CCFF", "NSeg": "#0088FF", "FusSeg": "#00008B"},
        "Sub": {"BSeg": "#99FF99", "NSeg": "#00AA00", "FusSeg": "#004400"},
    }

    # 渲染层配置：(线段类型, 标签前缀, 点大小, 透明度)
    _LAYERS = [
        ("BSeg", "Old", 20, 0.5),
        ("NSeg", "New", 50, 0.9),
        ("FusSeg", "Fusion", 80, 1.0),
    ]

    def __init__(self, frames: list[Frame], start_idx: int = 0):
        self.frames = frames
        if not self.frames:
            print("Error: No point data parsed from log!")
            sys.exit(1)

        self.current_idx = max(0, min(start_idx, len(self.frames) - 1))

        # 视口状态
        self.view_xlim: Optional[list[float]] = None
        self.view_ylim: Optional[list[float]] = None

        # 悬停点缓存
        self.hover_points: list[HoverPoint] = []
        self.annot: Optional[matplotlib.text.Annotation] = None

        # 拖拽状态机
        self.pan_state = PanState()

        self.fig, self.ax = plt.subplots(figsize=(12, 8))
        self._bind_events()
        self.update_plot()

    def _bind_events(self):
        """绑定所有交互事件"""
        self.fig.canvas.mpl_connect("key_press_event", self._on_key)
        self.fig.canvas.mpl_connect("scroll_event", self._on_scroll)
        self.fig.canvas.mpl_connect("button_press_event", self._on_mouse_press)
        self.fig.canvas.mpl_connect("button_release_event", self._on_mouse_release)
        self.fig.canvas.mpl_connect("motion_notify_event", self._on_mouse_move)

    def _on_key(self, event):
        """键盘控制：帧切换、重置、退出"""
        key = event.key
        if key in ["right", " ", "down"]:
            self.current_idx = min(self.current_idx + 1, len(self.frames) - 1)
            self.update_plot()
        elif key in ["left", "up"]:
            self.current_idx = max(self.current_idx - 1, 0)
            self.update_plot()
        elif key == "home":
            self.current_idx = 0
            self.update_plot()
        elif key == "end":
            self.current_idx = len(self.frames) - 1
            self.update_plot()
        elif key == "r":
            self.view_xlim = None
            self.view_ylim = None
            self.update_plot()
        elif key == "q":
            plt.close(self.fig)

    def _on_scroll(self, event):
        """鼠标滚轮缩放"""
        if event.inaxes != self.ax:
            return

        base_scale = 1.3
        if event.button == "up" or (hasattr(event, "step") and event.step > 0):
            scale_factor = 1 / base_scale
        elif event.button == "down" or (hasattr(event, "step") and event.step < 0):
            scale_factor = base_scale
        else:
            return

        cur_xlim, cur_ylim = self.ax.get_xlim(), self.ax.get_ylim()
        xdata, ydata = event.xdata, event.ydata

        new_width = (cur_xlim[1] - cur_xlim[0]) * scale_factor
        new_height = (cur_ylim[1] - cur_ylim[0]) * scale_factor
        relx = (cur_xlim[1] - xdata) / (cur_xlim[1] - cur_xlim[0])
        rely = (cur_ylim[1] - ydata) / (cur_ylim[1] - cur_ylim[0])

        self.view_xlim = [xdata - new_width * (1 - relx), xdata + new_width * relx]
        self.view_ylim = [ydata - new_height * (1 - rely), ydata + new_height * rely]

        self.ax.set_xlim(self.view_xlim)
        self.ax.set_ylim(self.view_ylim)
        self.fig.canvas.draw_idle()

    def _on_mouse_press(self, event):
        """鼠标右键按下开始拖拽"""
        if event.button == 3 and event.inaxes == self.ax:
            self.pan_state.active = True
            self.pan_state.start_pixel = (event.x, event.y)
            self.pan_state.start_xlim = self.ax.get_xlim()
            self.pan_state.start_ylim = self.ax.get_ylim()
            if self.annot and self.annot.get_visible():
                self.annot.set_visible(False)
                self.fig.canvas.draw_idle()

    def _on_mouse_release(self, event):
        """鼠标右键释放停止拖拽"""
        if event.button == 3:
            self.pan_state.active = False

    def _on_mouse_move(self, event):
        """鼠标移动：处理拖拽和悬停提示"""
        if event.inaxes != self.ax:
            if self.annot and self.annot.get_visible():
                self.annot.set_visible(False)
                self.fig.canvas.draw_idle()
            return

        # 拖拽优先
        if self.pan_state.active:
            dx_pixel = event.x - self.pan_state.start_pixel[0]
            dy_pixel = event.y - self.pan_state.start_pixel[1]
            bbox = self.ax.bbox
            dx_data = dx_pixel * (self.pan_state.start_xlim[1] - self.pan_state.start_xlim[0]) / bbox.width
            dy_data = dy_pixel * (self.pan_state.start_ylim[1] - self.pan_state.start_ylim[0]) / bbox.height
            self.view_xlim = [self.pan_state.start_xlim[0] - dx_data, self.pan_state.start_xlim[1] - dx_data]
            self.view_ylim = [self.pan_state.start_ylim[0] - dy_data, self.pan_state.start_ylim[1] - dy_data]
            self.ax.set_xlim(self.view_xlim)
            self.ax.set_ylim(self.view_ylim)
            self.fig.canvas.draw_idle()
            return

        # 悬停提示
        if not self.hover_points:
            return

        xlim = self.ax.get_xlim()
        threshold = (xlim[1] - xlim[0]) * 0.015

        min_dist = float("inf")
        closest = None
        for p in self.hover_points:
            dist = math.hypot(p.x - event.xdata, p.y - event.ydata)
            if dist < min_dist:
                min_dist = dist
                closest = p

        if closest and min_dist < threshold:
            self.annot.xy = (closest.x, closest.y)
            self.annot.set_text(f"X: {closest.x:.2f}\nY: {closest.y:.2f}\n{closest.target} {closest.seg}({closest.source})")
            self.annot.get_bbox_patch().set_edgecolor(self._COLORS[closest.target]["NSeg"])
            self.annot.set_visible(True)
        else:
            if self.annot.get_visible():
                self.annot.set_visible(False)
        self.fig.canvas.draw_idle()

    @staticmethod
    def _break_gap(xs: list[float], ys: list[float], threshold: float = 2.0) -> tuple[list[float], list[float]]:
        """如果两点距离大于阈值，插入 nan 截断连线"""
        if not xs:
            return [], []
        brk_x, brk_y = [xs[0]], [ys[0]]
        for i in range(1, len(xs)):
            if math.hypot(xs[i] - xs[i - 1], ys[i] - ys[i - 1]) > threshold:
                brk_x.append(float("nan"))
                brk_y.append(float("nan"))
            brk_x.append(xs[i])
            brk_y.append(ys[i])
        return brk_x, brk_y

    def update_plot(self):
        """渲染当前帧"""
        self.ax.clear()
        self.hover_points.clear()
        frame = self.frames[self.current_idx]

        self.ax.set_title(
            f"Frame ID: {frame.frame_id} | Progress: {self.current_idx + 1}/{len(self.frames)}\n"
            f"Keys: [<-/->] Flip | [Scroll] Zoom | [Right Click Drag] Pan | [R] Reset View",
            fontsize=11, pad=10, fontweight="bold"
        )

        for target in ["Obj1", "Obj2", "Sub"]:
            seg = getattr(frame, target)
            for seg_type, label_prefix, size, alpha in self._LAYERS:
                pts = getattr(seg, seg_type)
                if not pts:
                    continue

                # 收集悬停点
                for p in pts:
                    self.hover_points.append(HoverPoint(p.x, p.y, target, seg_type, p.source))

                c_hex = self._COLORS[target][seg_type]

                # 1. 绘制连线（跨越间隙处断开）
                xs, ys = self._break_gap([p.x for p in pts], [p.y for p in pts])
                line_style = "--" if seg_type == "BSeg" else "-"
                line_alpha = 0.3 if seg_type == "BSeg" else 0.8
                self.ax.plot(xs, ys, color=c_hex, linestyle=line_style,
                             linewidth=1.5, alpha=line_alpha, zorder=1)

                # 2. 绘制散点（按来源分组）
                from collections import defaultdict
                groups = defaultdict(lambda: {"x": [], "y": []})
                for p in pts:
                    groups[p.source]["x"].append(p.x)
                    groups[p.source]["y"].append(p.y)

                for src, coords in groups.items():
                    if seg_type == "NSeg":
                        self.ax.scatter(coords["x"], coords["y"], color="gold", alpha=0.4,
                                         marker=".", s=size * 5, edgecolors="none", zorder=2)
                    zo = 2 if seg_type == "BSeg" else 3
                    self.ax.scatter(coords["x"], coords["y"], c=c_hex, alpha=alpha,
                                    marker=".", s=size, edgecolors="none", zorder=zo,
                                    label=f"{target} {label_prefix}({src})")

        self.ax.set_xlabel("X (m)", fontweight="bold")
        self.ax.set_ylabel("Y (m)", fontweight="bold")

        if self.view_xlim is not None and self.view_ylim is not None:
            self.ax.set_xlim(self.view_xlim)
            self.ax.set_ylim(self.view_ylim)
        else:
            self.ax.axis("equal")

        self.ax.grid(True, linestyle="--", alpha=0.4)

        # 图例
        handles, labels = self.ax.get_legend_handles_labels()
        if handles:
            unique = dict(zip(labels, handles))
            self.ax.legend(unique.values(), unique.keys(), loc="upper left",
                           bbox_to_anchor=(1.02, 1), fontsize=9, markerscale=2.5)

        # 初始化 tooltip
        self.annot = self.ax.annotate(
            "", xy=(0, 0), xytext=(10, 10), textcoords="offset points",
            bbox=dict(boxstyle="round,pad=0.4", fc="lightyellow", ec="black", alpha=0.9),
            arrowprops=dict(arrowstyle="->", connectionstyle="arc3,rad=0"), zorder=10
        )
        self.annot.set_visible(False)
        plt.tight_layout()
        plt.draw()

    def show(self):
        plt.show()


def main():
    parser = argparse.ArgumentParser(description="APA Log Map Visualizer")
    parser.add_argument("log_file", help="Path to the log file")
    parser.add_argument(
        "-o", "--output", default=None, help="Export parsed data to a JSON file"
    )
    parser.add_argument(
        "-f",
        "--frame",
        type=int,
        default=0,
        help="Start from specified frame index (0-based)",
    )
    args = parser.parse_args()

    parser_obj = APALogParser(args.log_file)
    frames = parser_obj.parse()

    if args.output:
        try:
            with open(args.output, "w", encoding="utf-8") as f:
                json.dump(frames, f, indent=2, ensure_ascii=False)
            print(f"Exported to JSON: {args.output}")
        except Exception as e:
            print(f"Failed to export JSON: {e}")

    if frames:
        print("\nEngine Started!")
        print("Controls: ")
        print("  - [<-] / [->] or [Space]: Prev / Next")
        print("  - Mouse Scroll: Zoom In / Zoom Out")
        print("  - [Right Click + Drag]: Pan View")
        print("  - [Mouse Hover]: View Coordinates")
        print("  - [R]: Reset Zoom View")
        print("  - [Home] / [End]: First / Last")
        print("  - [Q]: Quit")
        visualizer = APAVisualizer(frames, start_idx=args.frame)
        visualizer.show()


if __name__ == "__main__":
    main()

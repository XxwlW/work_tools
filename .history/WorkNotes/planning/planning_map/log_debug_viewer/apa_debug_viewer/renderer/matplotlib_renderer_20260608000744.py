import matplotlib
matplotlib.use("TkAgg")  # 用原生窗口，不用浏览器

import matplotlib.pyplot as plt
from matplotlib.lines import Line2D
from matplotlib.patches import Polygon
from matplotlib.text import Annotation
from matplotlib.widgets import Slider, Button, TextBox
from matplotlib import rcParams
from .scene_builder import SceneObject

# 配置中文字体
rcParams['font.sans-serif'] = ['SimHei', 'WenQuanYi Micro Hei', 'Noto Sans CJK SC', 'AR PL UMing CN', 'DejaVu Sans']
rcParams['axes.unicode_minus'] = False  # 解决负号显示问题


class MatplotlibRenderer:
    """
    基于 Matplotlib 的渲染器
    使用原生 Tk 窗口显示，无需浏览器
    支持：右键拖动平移、滚轮缩放、悬浮提示、播放控制、帧选择面板
    """

    def __init__(self, figure_width: int = 12, figure_height: int = 9):
        self.figure_width = figure_width
        self.figure_height = figure_height
        self.fig: plt.Figure | None = None
        self.ax: plt.Axes | None = None
        self._pan_state = {"dragging": False, "last_x": 0.0, "last_y": 0.0}
        self._event_handlers_setup = False
        self._legend_handles: dict = {}
        self._title: str = "APA Debug Viewer"
        self._gui_mode: bool = False
        self._scene_objects: list[SceneObject] = []
        self._annotation: Annotation | None = None
        self._hover_annotation: Annotation | None = None  # 悬停提示
        self._last_pick_info = None  # 用于点击固定显示

        # 帧选择器 UI 组件
        self._frame_slider: Slider | None = None
        self._prev_btn: Button | None = None
        self._next_btn: Button | None = None
        self._play_btn: Button | None = None
        self._speed_buttons: list = []
        self._frame_slider_ax = None
        self._control_ax = None

        # 外部回调
        self._on_frame_change: callable | None = None
        self._on_play_toggle: callable | None = None
        self._on_speed_change: callable | None = None
        self._play_speed: int = 1
        self._is_playing: bool = False
        self._total_frames: int = 0
        self._current_frame_idx: int = 0
        self._frame_text: TextBox | None = None
        self._frame_info_ax = None
        self._seq_nums: list[int] = []  # 所有帧的 seq_num 列表，index→seq_num 映射
        self._left_click_pending: bool = False  # 标记左键点击后等待 pick 事件
        self._status_text: plt.Text | None = None  # 画布底部状态栏显示选中点信息
        self._last_hover_key: tuple = (None, None)  # (x, y) 缓存避免重复更新

    # ── 单帧渲染 ──────────────────────────────────────────

    def render(
        self, scene_objects: list[SceneObject], title: str = "APA Debug Viewer",
        block: bool = True,
    ) -> None:
        """渲染场景对象到 Matplotlib 窗口"""
        self._title = title
        self._gui_mode = not block
        self._scene_objects = scene_objects

        if self.fig is None:
            self.fig, self.ax = plt.subplots(figsize=(self.figure_width, self.figure_height))
            self._setup_annotation()
            self._setup_event_handlers()
            self._legend_handles = {}
            for obj in scene_objects:
                if not obj.visible:
                    continue
                self._draw_object(self.ax, obj, self._legend_handles)
            self._apply_layout(self.ax, self.fig, title, self._legend_handles)
            plt.tight_layout(rect=[0, 0.08, 1, 1])
            self.fig.canvas.draw()
            if block:
                plt.show()
        else:
            self.ax.clear()
            self._setup_annotation()
            self._last_hover_key = None
            self._legend_handles = {}
            for obj in scene_objects:
                if not obj.visible:
                    continue
                self._draw_object(self.ax, obj, self._legend_handles)
            self._apply_layout(self.ax, self.fig, title, self._legend_handles)
            self.fig.canvas.draw()
            if block:
                plt.show()

    def render_interactive(self) -> None:
        """启动交互模式（阻塞直到窗口关闭）"""
        if self.fig is None:
            raise RuntimeError("Must call render() with block=False before render_interactive()")
        self.fig.canvas.draw_idle()
        plt.pause(0.1)
        plt.show()

    def update_title(self, title: str) -> None:
        """更新窗口标题"""
        self._title = title
        if self.ax is not None and self.fig is not None:
            self.ax.set_title(title, fontsize=12, pad=10)
            self.fig.canvas.draw_idle()

    # ── 帧选择器 UI ────────────────────────────────────────

    def setup_frame_selector(
        self,
        total_frames: int,
        initial_idx: int = 0,
        seq_nums: list[int] | None = None,
        on_frame_change: callable = None,
        on_play_toggle: callable = None,
        on_speed_change: callable = None,
    ) -> None:
        """设置帧选择器UI面板"""
        self._total_frames = total_frames
        self._current_frame_idx = initial_idx
        self._seq_nums = seq_nums or []
        self._on_frame_change = on_frame_change
        self._on_play_toggle = on_play_toggle
        self._on_speed_change = on_speed_change

        if self.fig is None:
            return

        # 清理旧的控件（如果存在）
        if self._frame_slider_ax is not None:
            self._frame_slider_ax.remove()
        for ax in [c for c in [self._prev_btn, self._next_btn, self._play_btn] + self._speed_buttons if c is not None]:
            if hasattr(ax, 'ax'):
                ax.ax.remove()
            elif hasattr(ax, 'remove'):
                try:
                    ax.remove()
                except:
                    pass

        # 帧滑块 - 占据底部大部分区域
        self._frame_slider_ax = self.fig.add_axes([0.08, 0.005, 0.70, 0.05])
        self._frame_slider = Slider(
            self._frame_slider_ax,
            'Frame',
            0, total_frames - 1,
            valinit=initial_idx,
            valstep=1,
        )
        self._frame_slider.on_changed(self._on_slider_changed)

        # 上一帧按钮 <
        prev_ax = self.fig.add_axes([0.80, 0.01, 0.04, 0.04])
        self._prev_btn = Button(prev_ax, '<', color='lightblue', hovercolor='blue')
        self._prev_btn.label.set_fontsize(14)
        self._prev_btn.on_clicked(lambda _: self._on_prev_clicked())

        # 播放按钮 >
        play_ax = self.fig.add_axes([0.85, 0.008, 0.05, 0.045])
        self._play_btn = Button(play_ax, '>', color='lightgreen', hovercolor='green')
        self._play_btn.label.set_fontsize(14)
        self._play_btn.on_clicked(lambda _: self._on_play_clicked())

        # 下一帧按钮 >>
        next_ax = self.fig.add_axes([0.91, 0.01, 0.04, 0.04])
        self._next_btn = Button(next_ax, '>>', color='lightblue', hovercolor='blue')
        self._next_btn.label.set_fontsize(12)
        self._next_btn.on_clicked(lambda _: self._on_next_clicked())

        # 帧号输入框 - 直接输入日志中的 seq_num（如 21148）回车跳转
        text_ax = self.fig.add_axes([0.955, 0.01, 0.04, 0.045])
        init_seq = str(self._seq_nums[initial_idx]) if initial_idx < len(self._seq_nums) else str(initial_idx)
        self._frame_text = TextBox(text_ax, '', initial=init_seq)
        self._frame_text.label.set_fontsize(0)  # 隐藏标签
        self._frame_text.on_submit(self._on_jump_to_frame)

        # 显示当前帧信息 (seq_num / total)
        init_seq_display = self._seq_nums[initial_idx] if initial_idx < len(self._seq_nums) else '?'
        self._frame_info_ax = self.fig.text(
            0.50, 0.062,
            f'Seq #{init_seq_display}  ({initial_idx+1}/{total_frames})',
            ha='center', va='center', fontsize=9,
            bbox=dict(boxstyle='round,pad=0.2', facecolor='lightgray', alpha=0.5),
        )

        self.fig.canvas.draw()

    def _on_slider_changed(self, val: int) -> None:
        """滑块值变化"""
        idx = int(val)
        self._current_frame_idx = idx
        seq = self._seq_nums[idx] if idx < len(self._seq_nums) else '?'
        # 同步更新帧号输入框（显示 seq_num）
        if self._frame_text is not None:
            self._frame_text.set_val(f'{seq}')
        if self._frame_info_ax is not None:
            self._frame_info_ax.set_text(f'Seq #{seq}  ({idx+1}/{self._total_frames})')
        if self._on_frame_change:
            self._on_frame_change(idx)

    def _on_prev_clicked(self) -> None:
        """上一帧"""
        if self._current_frame_idx > 0:
            self._current_frame_idx -= 1
            self._frame_slider.set_val(self._current_frame_idx)
            if self._on_frame_change:
                self._on_frame_change(self._current_frame_idx)

    def _on_next_clicked(self) -> None:
        """下一帧"""
        if self._current_frame_idx < self._total_frames - 1:
            self._current_frame_idx += 1
            self._frame_slider.set_val(self._current_frame_idx)
            if self._on_frame_change:
                self._on_frame_change(self._current_frame_idx)

    def _on_play_clicked(self) -> None:
        """播放/暂停"""
        self._is_playing = not self._is_playing
        self._play_btn.label.set_text('||' if self._is_playing else '>')
        if self._on_play_toggle:
            self._on_play_toggle(self._is_playing)
        self.fig.canvas.draw_idle()

    def _on_speed_clicked(self, speed: int) -> None:
        """速度按钮"""
        self._play_speed = speed
        for i, s in enumerate([1, 2, 4, 8]):
            btn = self._speed_buttons[i]
            btn.color = 'orange' if s == speed else 'lightyellow'
        if self._on_speed_change:
            self._on_speed_change(speed)
        self.fig.canvas.draw_idle()

    def _on_jump_to_frame(self, text: str) -> None:
        """帧号输入框提交 - 输入 log 中的 seq_num，查表跳转"""
        try:
            target_seq = int(text.strip())
            # 在 seq_nums 列表中查找
            if target_seq in self._seq_nums:
                idx = self._seq_nums.index(target_seq)
                self._on_frame_change(idx)
            else:
                print(f"日志中不存在 seq_num #{target_seq}")
                # 恢复显示当前 seq_num
                if self._current_frame_idx < len(self._seq_nums):
                    self._frame_text.set_val(str(self._seq_nums[self._current_frame_idx]))
        except ValueError:
            print(f"无效的帧号: {text}，请输入日志中的 seq_num（数字）")

    def set_frame_index(self, idx: int) -> None:
        """外部设置当前帧索引（防止递归触发滑块回调）"""
        if idx == self._current_frame_idx:
            return
        self._current_frame_idx = idx
        if self._frame_slider is not None:
            self._frame_slider.set_val(idx)
        seq = self._seq_nums[idx] if idx < len(self._seq_nums) else '?'
        if self._frame_text is not None:
            self._frame_text.set_val(f'{seq}')
        if self._frame_info_ax is not None:
            self._frame_info_ax.set_text(f'Seq #{seq}  ({idx+1}/{self._total_frames})')
            self.fig.canvas.draw_idle()

    def set_playing(self, is_playing: bool) -> None:
        """外部设置播放状态"""
        self._is_playing = is_playing
        if self._play_btn is not None:
            self._play_btn.label.set_text('||' if is_playing else '>')
            self.fig.canvas.draw_idle()

    # ── 悬停提示 ──────────────────────────────────────────

    def _setup_annotation(self) -> None:
        """创建信息面板（固定位置，不浮动）：
           - _hover_info: 左下角悬停面板
           - _click_info: 左下角点击固定面板（更粗的边框）
        """
        if self.ax is None:
            return
        # 悬停面板 - 透明背景，左对齐
        self._hover_annotation = self.ax.text(
            0.01, 0.99, "",
            transform=self.ax.transAxes,
            bbox=dict(boxstyle="round,pad=0.3", facecolor="lightyellow",
                      edgecolor="darkorange", alpha=0.85, linewidth=1.5),
            fontsize=9,
            fontfamily="monospace",
            verticalalignment="top",
            horizontalalignment="left",
            visible=False,
            zorder=100,
        )
        # 点击固定面板 - 固定在上方
        self._annotation = self.ax.text(
            0.01, 0.87, "",
            transform=self.ax.transAxes,
            bbox=dict(boxstyle="round,pad=0.5", facecolor="#FFF8DC",
                      edgecolor="#CC5500", alpha=0.95, linewidth=2.5),
            fontsize=10,
            fontfamily="monospace",
            verticalalignment="top",
            horizontalalignment="left",
            visible=False,
            zorder=101,
        )

    def _make_hover_text(self, pt, obj) -> str:
        """从点对象生成悬停文本，坐标同时显示 m 和 mm"""
        meta = obj.meta or {}
        ptype = meta.get('point_type', obj.layer)
        src = meta.get('source', '')
        stage = meta.get('stage_name', '')
        desc = meta.get('description', '')
        mm_x = pt.x * 1000
        mm_y = pt.y * 1000

        lines = [
            f"({pt.x:.3f}, {pt.y:.3f}) m",
            f"({mm_x:.0f}, {mm_y:.0f}) mm",
            f"{ptype}",
        ]
        if src:
            lines.append(f"src={src}")
        if stage:
            lines.append(stage)
        if desc:
            lines.append(desc)
        return "\n".join(lines)

    def _show_hover_at(self, pt, obj) -> None:
        """显示悬停提示（固定于画布左上角）"""
        if self._hover_annotation is None:
            return
        text = self._make_hover_text(pt, obj)
        # 如果内容没变，跳过更新
        if text == self._last_hover_key:
            return
        self._last_hover_key = text
        self._hover_annotation.text = text
        self._hover_annotation.set_visible(True)
        self.fig.canvas.draw()

    def _hide_hover(self) -> None:
        """隐藏悬停提示"""
        if self._hover_annotation and self._hover_annotation.get_visible():
            self._hover_annotation.set_visible(False)
            self._last_hover_key = None
            self.fig.canvas.draw()

    def _show_click_annotation(self, pt, obj) -> None:
        """点击固定显示点的信息（画布左上角信息面板 + 底部状态栏）"""
        if self._annotation is None:
            return
        text = self._make_hover_text(pt, obj)
        self._annotation.text = text
        self._annotation.set_visible(True)
        self._last_pick_info = (pt, obj)
        if self._status_text is not None:
            one_line = text.replace("\n", " | ")
            self._status_text.set_text(f" ● 选中: {one_line}")
        self.fig.canvas.draw()

    def _hide_click_annotation(self) -> None:
        """隐藏点击固定提示"""
        if self._annotation and self._annotation.get_visible():
            self._annotation.set_visible(False)
            self._last_pick_info = None
        if self._status_text is not None:
            self._status_text.set_text("左键点击画布上的点查看坐标 | 右键拖动平移 | 滚轮缩放")
        self.fig.canvas.draw()

    # ── 事件处理 ──────────────────────────────────────────

    def _setup_event_handlers(self) -> None:
        """注册鼠标、键盘和 pick 事件"""
        if self.fig is None:
            return
        print("[DEBUG] 注册事件处理器")
        self.fig.canvas.mpl_connect("scroll_event", self._on_scroll)
        self.fig.canvas.mpl_connect("button_press_event", self._on_button_press)
        self.fig.canvas.mpl_connect("motion_notify_event", self._on_motion)
        self.fig.canvas.mpl_connect("button_release_event", self._on_button_release)
        self.fig.canvas.mpl_connect("key_press_event", self._on_key_press)
        self.fig.canvas.mpl_connect("pick_event", self._on_pick)
        print("[DEBUG] 事件处理器注册完成")

    def _on_scroll(self, event) -> None:
        """滚轮缩放"""
        if event.inaxes != self.ax or self.ax is None:
            return
        scale = 1.15 if event.button == "up" else 1.0 / 1.15
        xlim = self.ax.get_xlim()
        ylim = self.ax.get_ylim()
        xc = event.xdata if event.xdata else (xlim[0] + xlim[1]) / 2
        yc = event.ydata if event.ydata else (ylim[0] + ylim[1]) / 2
        self.ax.set_xlim([xc + (x - xc) * scale for x in xlim])
        self.ax.set_ylim([yc + (y - yc) * scale for y in ylim])
        self.fig.canvas.draw()

    def _on_button_press(self, event) -> None:
        """鼠标按下：
          - 左键 (1) 点击固定选中 / 空白处取消
          - 右键 (3) 平移画布
        """
        print(f"[DEBUG] button_press: button={event.button}, inaxes={event.inaxes is self.ax}, xdata={event.xdata:.2f}, ydata={event.ydata:.2f}")
        if event.button == 1:
            if event.inaxes == self.ax:
                print(f"[DEBUG] 左键点击画布，设置 left_click_pending=True")
                self._hide_click_annotation()
                self._left_click_pending = True
                # 行数统计
                n_total = len(self.ax.lines)
                n_pts = sum(1 for l in self.ax.lines if hasattr(l, '_scene_points'))
                print(f"[DEBUG] ax.lines: {n_total}条, 有点数据: {n_pts}条")
            else:
                self._hide_click_annotation()
        elif event.button == 3 and event.inaxes == self.ax and self.ax:
            print(f"[DEBUG] 右键按下，开始平移")
            self._pan_state["dragging"] = True
            self._pan_state["last_x"] = event.xdata or 0.0
            self._pan_state["last_y"] = event.ydata or 0.0
            self.fig.canvas.draw_idle()

    def _on_pick(self, event) -> None:
        """pick 事件 — 鼠标点击点时触发"""
        try:
            artist = event.artist
            print(f"[DEBUG] pick_event 触发! artist 类型={type(artist).__name__}, has _scene_points={hasattr(artist, '_scene_points')}, visible={artist.get_visible() if hasattr(artist, 'get_visible') else '?'}")
            if not hasattr(artist, '_scene_points'):
                print("[DEBUG] pick 跳过: 无 _scene_points")
                return
            if not artist.get_visible():
                print("[DEBUG] pick 跳过: 不可见")
                return
            pts = artist._scene_points
            meta = artist._scene_meta
            layer = artist._scene_layer
            if not pts or len(event.ind) == 0:
                print(f"[DEBUG] pick 跳过: pts={len(pts) if pts else 0}, ind={event.ind}")
                return
            ind = event.ind[0]
            pt = pts[ind]
            print(f"[DEBUG] pick 命中! 索引={ind}, 点=({pt.x:.3f}, {pt.y:.3f}), 类型={meta.get('point_type', layer)}, layer={layer}")
            print(f"[DEBUG] left_click_pending={self._left_click_pending}")
            obj_wrapper = type('obj', (), {'meta': meta, 'layer': layer})()

            if self._left_click_pending:
                self._left_click_pending = False
                print(f"[DEBUG] → 调用 _show_click_annotation")
                self._show_click_annotation(pt, obj_wrapper)
            else:
                print(f"[DEBUG] → 调用 _show_hover_at")
                self._show_hover_at(pt, obj_wrapper)
        except Exception as e:
            import traceback
            print(f"[pick error] {e}")
            traceback.print_exc()

    def _find_hover_point_pixel(self, event) -> tuple | None:
        """像素空间查找鼠标最近的数据点（不受缩放影响）"""
        if self.ax is None or event.xdata is None or event.ydata is None:
            return None
        transform = self.ax.transData
        mx, my = transform.transform((event.xdata, event.ydata))
        best_dist = 15  # 像素阈值（放大到15px提升手感）
        best_result = None

        for line in self.ax.lines:
            pts = getattr(line, '_scene_points', None)
            meta = getattr(line, '_scene_meta', None)
            layer = getattr(line, '_scene_layer', None)
            if pts is None or meta is None or not line.get_visible():
                continue
            for pt in pts:
                px, py = transform.transform((pt.x, pt.y))
                d = ((px - mx) ** 2 + (py - my) ** 2) ** 0.5
                if d < best_dist:
                    best_dist = d
                    best_result = (pt, type('obj', (), {'meta': meta, 'layer': layer})())
        if best_result:
            print(f"[DEBUG] hover命中: ({best_result[0].x:.2f}, {best_result[0].y:.2f}), 距离={best_dist:.1f}px")
        return best_result

    def _on_motion(self, event) -> None:
        """鼠标移动：
           - 右键拖动时 → 平移画布
           - 未拖动且在画布上 → 像素空间 hover 检测
        """
        self._left_click_pending = False

        if self._pan_state["dragging"]:
            if event.inaxes != self.ax or self.ax is None or event.xdata is None or event.ydata is None:
                return
            dx = event.xdata - self._pan_state["last_x"]
            dy = event.ydata - self._pan_state["last_y"]
            xlim = self.ax.get_xlim()
            ylim = self.ax.get_ylim()
            self.ax.set_xlim(xlim[0] - dx, xlim[1] - dx)
            self.ax.set_ylim(ylim[0] - dy, ylim[1] - dy)
            self._pan_state["last_x"] = event.xdata
            self._pan_state["last_y"] = event.ydata
            self.fig.canvas.draw()
            return

        if event.inaxes != self.ax:
            if self._hover_annotation and self._hover_annotation.get_visible():
                self._hide_hover()
            return

        result = self._find_hover_point_pixel(event)
        if result:
            pt, obj_wrapper = result
            self._show_hover_at(pt, obj_wrapper)
        else:
            if self._hover_annotation and self._hover_annotation.get_visible():
                self._hide_hover()

    def _on_button_release(self, event) -> None:
        """鼠标释放"""
        if event.button == 3:
            self._pan_state["dragging"] = False

    def _on_key_press(self, event) -> None:
        """键盘事件"""
        pass

    def set_key_handler(self, handler) -> None:
        """设置键盘回调"""
        if self.fig is not None:
            self.fig.canvas.mpl_connect("key_press_event", handler)

    # ── 子图渲染 ──────────────────────────────────────────

    def render_on_axes(self, ax: plt.Axes, scene_objects: list[SceneObject],
                       legend_handles: dict | None = None) -> dict:
        if legend_handles is None:
            legend_handles = {}
        for obj in scene_objects:
            if not obj.visible:
                continue
            self._draw_object(ax, obj, legend_handles)
        ax.set_aspect("equal")
        ax.grid(True, linestyle="--", alpha=0.3)
        return legend_handles

    # ── 多帧对比 ──────────────────────────────────────────

    def render_multi_frame(self, frame_objects: list[list[SceneObject]],
                          titles: list[str] | None = None) -> None:
        n = len(frame_objects)
        if n == 1:
            self.render(frame_objects[0], titles[0] if titles else "APA Debug Viewer")
            return
        cols = min(n, 3)
        rows = (n + cols - 1) // cols
        fig, axes = plt.subplots(rows, cols, figsize=(self.figure_width, self.figure_height))
        axes = axes.flatten() if n > 1 else [axes]
        legend_handles = {}
        for i in range(n):
            self.render_on_axes(axes[i], frame_objects[i], legend_handles)
            axes[i].set_title(titles[i] if titles and i < len(titles) else f"Frame {i+1}")
        for j in range(n, len(axes)):
            axes[j].axis("off")
        if legend_handles:
            fig.legend(handles=list(legend_handles.values()),
                      labels=list(legend_handles.keys()),
                      loc="lower center", ncol=min(len(legend_handles), 6), fontsize=9)
        fig.suptitle("APA Debug Viewer - Multi Frame", fontsize=14)
        plt.tight_layout(rect=[0, 0.05, 1, 0.95])
        plt.show()

    # ── 内部绘制 ──────────────────────────────────────────

    def _draw_object(self, ax: plt.Axes, obj: SceneObject, legend_handles: dict) -> None:
        """绘制单个场景对象（每个点可被 picker 选中）"""
        if not obj.points:
            return
        xs = [p.x for p in obj.points]
        ys = [p.y for p in obj.points]
        marker = self._get_marker_for_type(obj.meta.get("point_type", "") if obj.meta else "")
        # 绘制连线（不可选中）
        ax.plot(xs, ys, color=obj.color, linewidth=obj.line_width,
                alpha=obj.opacity, linestyle="-", label=None)
        # 绘制点（可选中，picker=8 表示 8 点像素半径）
        line, = ax.plot(xs, ys, color=obj.color, marker=marker,
                        markersize=6, linestyle="None",
                        picker=8, label=None)
        line._scene_points = obj.points   # 存储原始 Point 对象
        line._scene_meta = obj.meta or {}
        line._scene_layer = obj.layer
        # 处理闭合多边形填充
        is_closed = len(obj.points) >= 3 and obj.layer in ("CurrentVehicle", "GoalVehicle", "Slot")
        if is_closed and len(xs) > 2:
            poly_xs = xs + [xs[0]]
            poly_ys = ys + [ys[0]]
            poly = Polygon(list(zip(poly_xs, poly_ys)), closed=True, color=obj.color, alpha=0.15)
            ax.add_patch(poly)
        if obj.layer not in legend_handles:
            legend_handles[obj.layer] = Line2D([0], [0], color=obj.color,
                                                linewidth=obj.line_width, marker=marker,
                                                markersize=5, label=obj.layer)

    @staticmethod
    def _get_marker_for_type(ptype: str) -> str:
        return {"BSeg": "o", "NSeg": "s", "FusSeg": "^",
                "PDC": "D", "Vehicle": "o", "Slot": "s", "Boundary": "_"}.get(ptype, "o")

    # ── 布局 ──────────────────────────────────────────────

    def _apply_layout(self, ax: plt.Axes, fig: plt.Figure, title: str, legend_handles: dict) -> None:
        ax.set_aspect("equal")
        ax.grid(True, linestyle="--", alpha=0.3)
        ax.set_xlabel("x (m)")
        ax.set_ylabel("y (m)")
        ax.set_title(title, fontsize=12, pad=10)
        if legend_handles:
            ax.legend(handles=list(legend_handles.values()), labels=list(legend_handles.keys()),
                     loc="upper right", fontsize=8, framealpha=0.8)
        fig.set_facecolor("#f8f9fa")
        ax.set_facecolor("#ffffff")
        # 底部状态栏（显示选中点信息）
        bbox = ax.get_position()
        if self._status_text is not None:
            self._status_text.remove()
        self._status_text = fig.text(
            0.5, 0.095,
            "左键点击画布上的点查看坐标 | 右键拖动平移 | 滚轮缩放",
            ha='center', va='bottom', fontsize=9,
            color='#555555',
        )

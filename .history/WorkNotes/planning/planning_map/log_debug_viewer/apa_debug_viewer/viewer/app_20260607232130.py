import os
import sys
from pathlib import Path

# 确保项目根目录在 sys.path 中
_project_root = Path(__file__).resolve().parent.parent
if str(_project_root) not in sys.path:
    sys.path.insert(0, str(_project_root))


class App:
    """
    主应用入口
    整合 Parser → SceneBuilder → Renderer → Viewer
    """

    def __init__(self):
        from parser.frame_splitter import FrameSplitter
        from parser.frame_parser import FrameParser
        from renderer.scene_builder import SceneBuilder
        from renderer.matplotlib_renderer import MatplotlibRenderer
        from viewer.timeline import Timeline
        from viewer.layer_manager import LayerManager
        from viewer.inspector import Inspector

        self.splitter = FrameSplitter()
        self.parser = FrameParser()
        self.scene_builder = SceneBuilder()
        self.renderer = MatplotlibRenderer()
        self.timeline = Timeline()
        self.layer_manager = LayerManager()
        self.inspector = Inspector()

        # 缓存上一个有效的车辆 pose
        self._last_vehicle_pose = None
        self._last_goal_pose = None

        # 播放状态
        self._playing = False
        self._play_speed = 1

        # 播放定时器
        self._play_timer = None

    def load_log(self, log_path: str | Path) -> None:
        """加载日志文件并解析所有帧"""
        log_path = Path(log_path)
        if not log_path.exists():
            raise FileNotFoundError(f"日志文件不存在: {log_path}")

        text = log_path.read_text(encoding="utf-8", errors="replace")

        frames_text = self.splitter.split_with_seq(text)

        frames = []
        total = len(frames_text)
        for i, (seq_num, frame_text) in enumerate(frames_text):
            frame = self.parser.parse(frame_text, seq_num=seq_num)
            frames.append(frame)

        self.timeline.load(frames)

        # 找到第一帧有效数据
        first_valid = 0
        for i, frame in enumerate(frames):
            if frame.current_pose is not None or frame.has_fusion_data or frame.slot_pts:
                first_valid = i
                break

        self.timeline.go_to(first_valid)
        frame = self.timeline.current_frame
        if frame.current_pose is not None:
            self._last_vehicle_pose = frame.current_pose
        if frame.goal_pose is not None:
            self._last_goal_pose = frame.goal_pose

        print(f"加载 {len(frames)} 帧, 首帧 seq=#{frame.seq_num}")

    def show_current(self, block: bool = True) -> None:
        """显示当前帧"""
        frame = self.timeline.current_frame
        if frame is None:
            print("没有可显示的帧")
            return

        scene_objects = self.scene_builder.build(frame, self._last_vehicle_pose, self._last_goal_pose)

        for obj in scene_objects:
            obj.visible = self.layer_manager.is_visible(obj.layer)

        has_data = any(obj.visible for obj in scene_objects)
        if not has_data:
            return

        title = f"APA Debug Viewer - Frame #{frame.seq_num}"
        try:
            self.renderer.render(scene_objects, title=title, block=block)
        except Exception as e:
            print(f"[ERROR] 渲染帧 #{frame.seq_num} 失败: {e}")

    def run_gui(self) -> None:
        """运行 GUI 交互模式"""
        frame = self.timeline.current_frame
        if frame is None:
            print("没有可显示的帧")
            return

        scene_objects = self.scene_builder.build(frame, self._last_vehicle_pose, self._last_goal_pose)
        for obj in scene_objects:
            obj.visible = self.layer_manager.is_visible(obj.layer)

        title = f"APA Debug Viewer - Frame #{frame.seq_num}"

        # 键盘事件处理
        def on_key_press(event):
            from matplotlib.backend_bases import KeyEvent
            handled = True

            if event.key == "right":
                self._playing = False
                self.renderer.set_playing(False)
                if self.timeline.has_next:
                    self.timeline.next()
                    self._on_frame_changed()
            elif event.key == "left":
                self._playing = False
                self.renderer.set_playing(False)
                if self.timeline.has_previous:
                    self.timeline.previous()
                    self._on_frame_changed()
            elif event.key == "up":
                self._playing = False
                self.renderer.set_playing(False)
                self.timeline.first()
                self._on_frame_changed()
            elif event.key == "down":
                self._playing = False
                self.renderer.set_playing(False)
                self.timeline.last()
                self._on_frame_changed()
            elif event.key == " ":
                self._playing = not self._playing
                self.renderer.set_playing(self._playing)
                self._update_play_timer()
            elif event.key in ["1", "2", "3", "4"]:
                speed = int(event.key)
                self._play_speed = speed
                self._update_play_timer()
            elif event.key == "t":
                self._toggle_layer_menu_gui()
            elif event.key == "g":
                print("请在底部输入框中直接输入 seq_num 后回车跳转")
            else:
                handled = False

        try:
            import matplotlib
            matplotlib.use("TkAgg")
            self.renderer.render(scene_objects, title=title, block=False)
            self.renderer.set_key_handler(on_key_press)

            # 设置帧选择器 UI
            self.renderer.setup_frame_selector(
                total_frames=self.timeline.total_frames,
                initial_idx=self.timeline.current_index,
                seq_nums=self.timeline.get_frame_indices(),
                on_frame_change=self._on_frame_slider_changed,
                on_play_toggle=self._on_play_toggled,
                on_speed_change=self._on_speed_changed,
            )

            self.renderer.render_interactive()
        except Exception as e:
            print(f"GUI 渲染失败: {e}")
            import traceback
            traceback.print_exc()

    def _on_frame_slider_changed(self, idx: int) -> None:
        """滑块变化时的回调"""
        self.timeline.go_to(idx)
        self._on_frame_changed()

    def _on_frame_changed(self) -> None:
        """帧变化后的处理"""
        frame = self.timeline.current_frame
        if frame is None:
            return

        # 更新缓存的 pose
        if frame.current_pose is not None:
            self._last_vehicle_pose = frame.current_pose
        if frame.goal_pose is not None:
            self._last_goal_pose = frame.goal_pose

        scene_objects = self.scene_builder.build(frame, self._last_vehicle_pose, self._last_goal_pose)
        for obj in scene_objects:
            obj.visible = self.layer_manager.is_visible(obj.layer)

        title = f"APA Debug Viewer - Frame #{frame.seq_num} [{self.timeline.current_index+1}/{self.timeline.total_frames}]"
        self.renderer.update_title(title)
        self.renderer.render(scene_objects, title=title, block=False)
        self.renderer.set_frame_index(self.timeline.current_index)
        print(self.inspector.format_info(frame))

    def _on_play_toggled(self, is_playing: bool) -> None:
        """播放状态切换"""
        self._playing = is_playing
        self._update_play_timer()

    def _on_speed_changed(self, speed: int) -> None:
        """速度变化"""
        self._play_speed = speed
        self._update_play_timer()

    def _update_play_timer(self) -> None:
        """更新播放定时器"""
        # 移除旧的定时器
        if self._play_timer is not None:
            self._play_timer.stop()
            self._play_timer = None

        if self._playing and self._play_timer is None:
            # 设置定时器
            interval = 0.5 / self._play_speed
            self._play_timer = self.renderer.fig.canvas.new_timer(interval=int(interval * 1000))
            self._play_timer.add_callback(self._play_next_frame)
            self._play_timer.start()

    def _play_next_frame(self) -> None:
        """播放下一帧"""
        if self._playing and self.timeline.has_next:
            self.timeline.next()
            self._on_frame_changed()
        else:
            self._playing = False
            self.renderer.set_playing(False)
            if self._play_timer:
                self._play_timer.stop()
                self._play_timer = None

    def _toggle_layer_menu_gui(self) -> None:
        """切换图层可见性"""
        layers = self.layer_manager.get_all_layers()
        print("\n--- Layer Visibility ---")
        for i, layer in enumerate(layers):
            status = "✓" if layer.visible else " "
            print(f"  [{status}] {i+1}. {layer.name}")
        print("------------------------")

        try:
            choice = input("切换图层编号 (回车取消): ").strip()
            if choice:
                idx = int(choice) - 1
                if 0 <= idx < len(layers):
                    self.layer_manager.toggle(layers[idx].name)
                    self._on_frame_changed()
        except (ValueError, IndexError, EOFError, KeyboardInterrupt):
            pass

    def run_interactive(self) -> None:
        """运行交互式命令行界面"""
        print("\n" + "=" * 60)
        print("  APA Debug Viewer")
        print("=" * 60)
        print("  Commands:")
        print("    n          下一帧")
        print("    p          上一帧")
        print("    f          第一帧")
        print("    l          最后一帧")
        print("    g <num>    跳转到指定帧")
        print("    i          显示当前帧信息")
        print("    t          切换图层可见性")
        print("    q          退出")
        print("=" * 60)

        while True:
            frame = self.timeline.current_frame
            total = self.timeline.total_frames
            idx = self.timeline.current_index

            try:
                cmd = input(f"\n[{idx + 1}/{total}] > ").strip()
            except (EOFError, KeyboardInterrupt):
                print("\n退出")
                break

            if not cmd:
                continue

            parts = cmd.split()
            action = parts[0].lower()

            if action == "q":
                break
            elif action == "n":
                self.timeline.next()
                self.show_current()
            elif action == "p":
                self.timeline.previous()
                self.show_current()
            elif action == "f":
                self.timeline.first()
                self.show_current()
            elif action == "l":
                self.timeline.last()
                self.show_current()
            elif action == "g" and len(parts) > 1:
                try:
                    target = int(parts[1])
                    self.timeline.go_to_seq(target)
                    self.show_current()
                except ValueError:
                    print("无效的帧号")
            elif action == "i":
                if frame:
                    print(self.inspector.format_info(frame))
            elif action == "t":
                self._toggle_layer_menu()
            else:
                print(f"未知命令: {action}")

    def _toggle_layer_menu(self) -> None:
        """交互式图层切换菜单"""
        layers = self.layer_manager.get_all_layers()
        print("\n--- Layer Visibility ---")
        for i, layer in enumerate(layers):
            status = "✓" if layer.visible else " "
            print(f"  [{status}] {i+1}. {layer.name}")
        print("------------------------")

        try:
            choice = input("切换图层编号 (回车取消): ").strip()
            if choice:
                idx = int(choice) - 1
                if 0 <= idx < len(layers):
                    self.layer_manager.toggle(layers[idx].name)
                    self.show_current()
        except (ValueError, IndexError, EOFError, KeyboardInterrupt):
            pass
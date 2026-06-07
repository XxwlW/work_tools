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

    def load_log(self, log_path: str | Path) -> None:
        """加载日志文件并解析所有帧"""
        log_path = Path(log_path)
        if not log_path.exists():
            raise FileNotFoundError(f"日志文件不存在: {log_path}")

        text = log_path.read_text(encoding="utf-8", errors="replace")

        # 1. 切分 frame
        frames_text = self.splitter.split_with_seq(text)

        # 2. 解析每个 frame
        frames = []
        for seq_num, frame_text in frames_text:
            frame = self.parser.parse(frame_text, seq_num=seq_num)
            frames.append(frame)
            print(f"  [OK] Frame #{seq_num}: "
                  f"pose={frame.current_pose is not None}, "
                  f"fusion={len(frame.fusion_stages)} stages, "
                  f"boundary={len(frame.boundaries)}")

        # 3. 加载到 timeline
        self.timeline.load(frames)
        print(f"\n成功加载 {len(frames)} 帧")

        # 4. 自动跳到第一帧有效数据（优先级: pose+fusion > fusion > pose > slot）
        first_pose_fusion = -1
        first_fusion = -1
        first_pose = -1
        first_slot = -1

        for i, frame in enumerate(frames):
            if frame.current_pose is not None and frame.has_fusion_data and first_pose_fusion < 0:
                first_pose_fusion = i
            if frame.has_fusion_data and first_fusion < 0:
                first_fusion = i
            if frame.current_pose is not None and first_pose < 0:
                first_pose = i
            if frame.slot_pts and first_slot < 0:
                first_slot = i

        target = first_pose_fusion
        if target < 0:
            target = first_fusion
        if target < 0:
            target = first_pose
        if target < 0:
            target = first_slot
        if target < 0:
            target = 0

        self.timeline.go_to(target)
        frame = self.timeline.current_frame
        if target > 0 or frame.current_pose is not None or frame.has_fusion_data:
            print(f"自动定位到第 {target+1}/{len(frames)} 帧 (seq=#{frame.seq_num})")
        else:
            print("日志中未检测到有效数据")

    def show_current(self, block: bool = True) -> None:
        """显示当前帧

        Args:
            block: 是否阻塞（True=等待窗口关闭才返回，False=不阻塞）
        """
        frame = self.timeline.current_frame
        if frame is None:
            print("没有可显示的帧")
            return

        # 构建场景
        scene_objects = self.scene_builder.build(frame)

        # 应用图层可见性
        for obj in scene_objects:
            obj.visible = self.layer_manager.is_visible(obj.layer)

        # 如果没有任何场景对象，给出提示
        has_data = any(obj.visible for obj in scene_objects)
        if not has_data:
            print(f"Frame #{frame.seq_num}: 没有任何有效数据可渲染")
            print("尝试跳转到有数据的帧（如 --frame 21092）")
            return

        # 渲染
        title = f"APA Debug Viewer - Frame #{frame.seq_num}"
        try:
            import matplotlib
            matplotlib.use("TkAgg")
            self.renderer.render(scene_objects, title=title)
        except Exception as e:
            print(f"渲染失败: {e}")

        # 显示帧信息
        info = self.inspector.format_info(frame)
        print(info)

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
        except (ValueError, IndexError):
            pass

import matplotlib
matplotlib.use("TkAgg")  # 用原生窗口，不用浏览器

import matplotlib.pyplot as plt
from matplotlib.lines import Line2D
from matplotlib.patches import Polygon
from .scene_builder import SceneObject


class MatplotlibRenderer:
    """
    基于 Matplotlib 的渲染器
    使用原生 Tk 窗口显示，无需浏览器
    """

    def __init__(self, figure_width: int = 12, figure_height: int = 9):
        self.figure_width = figure_width
        self.figure_height = figure_height

    # ── 单帧渲染 ──────────────────────────────────────────

    def render(
        self, scene_objects: list[SceneObject], title: str = "APA Debug Viewer",
    ) -> None:
        """渲染场景对象到 Matplotlib 窗口"""
        fig, ax = plt.subplots(figsize=(self.figure_width, self.figure_height))

        legend_handles = {}

        for obj in scene_objects:
            if not obj.visible:
                continue
            self._draw_object(ax, obj, legend_handles)

        self._apply_layout(ax, fig, title, legend_handles)
        plt.tight_layout()
        plt.show()

    # ── 子图集成（供多帧渲染调用） ──────────────────────────

    def render_on_axes(
        self, ax: plt.Axes, scene_objects: list[SceneObject],
        legend_handles: dict | None = None,
    ) -> dict:
        """在指定 axes 上渲染（供子图使用）"""
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

    def render_multi_frame(
        self, frame_objects: list[list[SceneObject]],
        titles: list[str] | None = None,
    ) -> None:
        """渲染多帧对比视图"""
        n = len(frame_objects)
        if n == 1:
            self.render(frame_objects[0], titles[0] if titles else "APA Debug Viewer")
            return

        cols = min(n, 3)
        rows = (n + cols - 1) // cols
        fig, axes = plt.subplots(
            rows, cols,
            figsize=(self.figure_width, self.figure_height),
        )
        axes = axes.flatten() if n > 1 else [axes]

        legend_handles = {}

        for i in range(n):
            ax = axes[i]
            self.render_on_axes(ax, frame_objects[i], legend_handles)
            ax.set_title(titles[i] if titles and i < len(titles) else f"Frame {i+1}")

        # 隐藏多余的子图
        for j in range(n, len(axes)):
            axes[j].axis("off")

        # 统一图例
        if legend_handles:
            fig.legend(
                handles=list(legend_handles.values()),
                labels=list(legend_handles.keys()),
                loc="lower center",
                ncol=min(len(legend_handles), 6),
                fontsize=9,
            )

        fig.suptitle("APA Debug Viewer - Multi Frame", fontsize=14)
        plt.tight_layout(rect=[0, 0.05, 1, 0.95])
        plt.show()

    # ── 内部绘制 ──────────────────────────────────────────

    def _draw_object(
        self, ax: plt.Axes, obj: SceneObject,
        legend_handles: dict,
    ) -> None:
        """绘制单个场景对象"""
        if not obj.points:
            return

        xs = [p.x for p in obj.points]
        ys = [p.y for p in obj.points]

        # 判断是否需要闭合
        is_closed = len(obj.points) >= 3 and obj.layer in (
            "CurrentVehicle", "GoalVehicle", "Slot",
        )
        if is_closed:
            xs = list(xs) + [xs[0]]
            ys = list(ys) + [ys[0]]

        # 绘制线+标记
        line, = ax.plot(
            xs, ys,
            color=obj.color,
            linewidth=obj.line_width,
            alpha=obj.opacity,
            marker="o",
            markersize=3,
            linestyle="-",
            label=obj.id,
        )

        # 车辆填充
        if obj.layer in ("CurrentVehicle", "GoalVehicle") and len(xs) > 4:
            polygon = Polygon(
                list(zip(xs, ys)),
                closed=True,
                color=obj.color,
                alpha=0.15,
            )
            ax.add_patch(polygon)

        # 记录图例（按 layer 分组去重）
        if obj.layer not in legend_handles:
            legend_handles[obj.layer] = Line2D(
                [0], [0],
                color=obj.color,
                linewidth=obj.line_width,
                label=obj.layer,
            )

    # ── 布局 ──────────────────────────────────────────────

    def _apply_layout(
        self, ax: plt.Axes, fig: plt.Figure,
        title: str, legend_handles: dict,
    ) -> None:
        """应用布局"""
        ax.set_aspect("equal")
        ax.grid(True, linestyle="--", alpha=0.3)
        ax.set_xlabel("x (meter)")
        ax.set_ylabel("y (meter)")
        ax.set_title(title, fontsize=14, pad=10)

        if legend_handles:
            ax.legend(
                handles=list(legend_handles.values()),
                labels=list(legend_handles.keys()),
                loc="upper right",
                fontsize=8,
                framealpha=0.8,
            )

        fig.set_facecolor("#f8f9fa")
        ax.set_facecolor("#ffffff")

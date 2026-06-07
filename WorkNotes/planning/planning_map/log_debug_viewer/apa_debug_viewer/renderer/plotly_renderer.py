import plotly.graph_objects as go
from plotly.subplots import make_subplots
from .scene_builder import SceneObject


class PlotlyRenderer:
    """
    基于 Plotly 的渲染器
    将 SceneObject 列表渲染为交互式图形
    """

    def __init__(self, figure_width: int = 1200, figure_height: int = 800):
        self.figure_width = figure_width
        self.figure_height = figure_height

    def render(
        self, scene_objects: list[SceneObject], title: str = "APA Debug Viewer",
    ) -> go.Figure:
        """渲染场景对象为 Plotly Figure"""
        fig = go.Figure()

        for obj in scene_objects:
            if not obj.visible:
                continue
            trace = self._build_trace(obj)
            if trace is not None:
                fig.add_trace(trace)

        self._apply_layout(fig, title)
        return fig

    def render_multi_frame(
        self, frame_objects: list[list[SceneObject]],
        titles: list[str] | None = None,
    ) -> go.Figure:
        """渲染多帧对比视图"""
        n = len(frame_objects)
        fig = make_subplots(
            rows=1, cols=n,
            subplot_titles=titles or [f"Frame {i+1}" for i in range(n)],
            horizontal_spacing=0.05,
        )

        for col_idx, objects in enumerate(frame_objects, start=1):
            for obj in objects:
                if not obj.visible:
                    continue
                trace = self._build_trace(obj)
                if trace is not None:
                    fig.add_trace(trace, row=1, col=col_idx)

        self._apply_layout(fig, "APA Debug Viewer - Multi Frame")
        return fig

    def _build_trace(self, obj: SceneObject) -> go.Scatter | None:
        """为 SceneObject 构建 Plotly trace"""
        if not obj.points:
            return None

        xs = [p.x for p in obj.points]
        ys = [p.y for p in obj.points]

        # 闭合多边形（如果是闭合图形）
        is_closed = False
        if len(obj.points) >= 3:
            # 对车辆、Slot 等闭合图形，自动闭合
            if obj.layer in ("CurrentVehicle", "GoalVehicle", "Slot"):
                is_closed = True

        # 如果图层是填充类型
        fill = None
        if is_closed:
            xs = xs + [xs[0]]
            ys = ys + [ys[0]]
            fill = "toself" if obj.layer in ("CurrentVehicle", "GoalVehicle") else None

        return go.Scatter(
            x=xs,
            y=ys,
            mode="lines+markers",
            name=obj.id,
            legendgroup=obj.layer,
            line=dict(
                color=obj.color,
                width=obj.line_width,
            ),
            marker=dict(
                color=obj.color,
                size=4,
            ),
            opacity=obj.opacity,
            fill=fill,
            fillcolor=obj.color if fill else None,
            showlegend=True,
            hovertemplate=(
                f"<b>{obj.id}</b><br>"
                f"Layer: {obj.layer}<br>"
                f"x: %{{x:.3f}} m<br>"
                f"y: %{{y:.3f}} m<br>"
                "<extra></extra>"
            ),
        )

    def _apply_layout(self, fig: go.Figure, title: str) -> None:
        """应用布局设置"""
        fig.update_layout(
            title=dict(
                text=title,
                x=0.5,
                font=dict(size=16),
            ),
            width=self.figure_width,
            height=self.figure_height,
            showlegend=True,
            legend=dict(
                x=1.02,
                y=1,
                xanchor="left",
                yanchor="top",
                font=dict(size=10),
            ),
            hovermode="closest",
            plot_bgcolor="#ffffff",
            paper_bgcolor="#f8f9fa",
            margin=dict(l=50, r=200, t=50, b=50),
        )

        fig.update_xaxes(
            scaleanchor="y",
            scaleratio=1,
            gridcolor="#e0e0e0",
            zerolinecolor="#cccccc",
            title=dict(text="x (meter)"),
        )

        fig.update_yaxes(
            gridcolor="#e0e0e0",
            zerolinecolor="#cccccc",
            title=dict(text="y (meter)"),
        )

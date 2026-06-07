from dataclasses import dataclass, field
from model.frame import FrameData
from model.point import Point
from model.pose import Pose
from .color_manager import ColorManager


@dataclass
class SceneObject:
    """场景对象，用于传递给渲染器"""
    id: str
    layer: str
    points: list[Point] = field(default_factory=list)
    visible: bool = True
    color: str = "#7f7f7f"
    line_width: float = 2.0
    opacity: float = 1.0
    meta: dict | None = None


class SceneBuilder:
    """
    将 FrameData 转换为 SceneObject 列表
    中间层，隔离数据模型与渲染器
    """

    @staticmethod
    def build(frame: FrameData) -> list[SceneObject]:
        """构建完整的场景对象列表"""
        objects: list[SceneObject] = []

        # 1. 当前车辆
        if frame.current_pose is not None:
            veh_obj = SceneBuilder._build_vehicle(
                "current_vehicle", "CurrentVehicle",
                frame.current_pose, ColorManager.get("CurrentVehicle"),
            )
            if veh_obj:
                objects.append(veh_obj)

        # 2. 目标车辆
        if frame.goal_pose is not None:
            goal_obj = SceneBuilder._build_vehicle(
                "goal_vehicle", "GoalVehicle",
                frame.goal_pose, ColorManager.get("GoalVehicle"),
            )
            if goal_obj:
                objects.append(goal_obj)

        # 3. 停车位
        if frame.slot_pts:
            objects.append(SceneObject(
                id="slot",
                layer="Slot",
                points=frame.slot_pts,
                color=ColorManager.get("Slot"),
                line_width=2.5,
            ))

        # 4. PDC 数据
        if frame.left_pdc:
            objects.append(SceneObject(
                id="left_pdc",
                layer="LeftPDC",
                points=frame.left_pdc,
                color=ColorManager.get("LeftPDC"),
            ))
        if frame.right_pdc:
            objects.append(SceneObject(
                id="right_pdc",
                layer="RightPDC",
                points=frame.right_pdc,
                color=ColorManager.get("RightPDC"),
            ))

        # 5. Fusion 数据
        for stage in frame.fusion_stages:
            prefix = stage.name
            if stage.bseg and not stage.bseg.is_empty:
                objects.append(SceneObject(
                    id=f"{prefix}_bseg",
                    layer="BSeg",
                    points=stage.bseg.points,
                    color=ColorManager.get("BSeg"),
                ))
            if stage.nseg and not stage.nseg.is_empty:
                objects.append(SceneObject(
                    id=f"{prefix}_nseg",
                    layer="NSeg",
                    points=stage.nseg.points,
                    color=ColorManager.get("NSeg"),
                ))
            if stage.fusseg and not stage.fusseg.is_empty:
                objects.append(SceneObject(
                    id=f"{prefix}_fusseg",
                    layer="FusSeg",
                    points=stage.fusseg.points,
                    color=ColorManager.get("FusSeg"),
                ))

        # 6. Boundary
        for boundary in frame.boundaries:
            if boundary.is_empty:
                continue
            layer_name = "SubBoundary" if "sub" in boundary.name.lower() else "Boundary"
            objects.append(SceneObject(
                id=f"boundary_{boundary.name}",
                layer=layer_name,
                points=boundary.points,
                color=ColorManager.get(layer_name),
                line_width=3.0,
            ))

        return objects

    @staticmethod
    def _build_vehicle(
        obj_id: str, layer: str, pose: Pose, color: str,
    ) -> SceneObject | None:
        """构建车辆多边形（5个点组成闭合矩形）"""
        from utils.geometry import build_vehicle_polygon
        points = build_vehicle_polygon(pose)
        if not points:
            return None
        return SceneObject(
            id=obj_id,
            layer=layer,
            points=points,
            color=color,
            line_width=2.0,
        )

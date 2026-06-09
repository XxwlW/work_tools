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

    # Fusion stage name 到 source 的映射
    SOURCE_MAP = {
        "PDCFusionObj1Success": "PDC",
        "PDCFusionObj2Success": "PDC",
        "PDCFusionSubLaneSuccess": "PDC",
        "FSDFusionObj1Success": "FSD",
        "FSDFusionObj2Success": "FSD",
        "FSDFusionSubLaneSuccess": "FSD",
        "SDGFusionObj1InnerBorderSuccess": "SDG",
        "SDGFusionObj1OuterBorderSuccess": "SDG",
        "SDGFusionObj2OuterBorderSuccess": "SDG",
    }

    @staticmethod
    def _get_source(stage_name: str) -> str:
        """从 stage name 获取 source"""
        return SceneBuilder.SOURCE_MAP.get(stage_name, "")

    @staticmethod
    def build(frame: FrameData, cached_vehicle_pose=None, cached_goal_pose=None) -> list[SceneObject]:
        """构建完整的场景对象列表

        Args:
            frame: 当前帧数据
            cached_vehicle_pose: 缓存的车辆 pose（当帧无 pose 时使用，但必须是同坐标系）
            cached_goal_pose: 缓存的目标 pose
        """
        objects: list[SceneObject] = []

        # 使用当前帧的 pose（不使用 cached，因为坐标系可能不同）
        vehicle_pose = frame.current_pose # slot-local 或 None
        goal_pose = frame.goal_pose if frame.goal_pose is not None else cached_goal_pose

        # 1. 当前车辆
        if vehicle_pose is not None:
            veh_obj = SceneBuilder._build_vehicle(
                "current_vehicle", "CurrentVehicle",
                vehicle_pose, ColorManager.get("CurrentVehicle"),
            )
            if veh_obj:
                veh_obj.meta = {"source": "Vehicle", "point_type": "Vehicle"}
                objects.append(veh_obj)

        # 2. 目标车辆
        if goal_pose is not None:
            goal_obj = SceneBuilder._build_vehicle(
                "goal_vehicle", "GoalVehicle",
                goal_pose, ColorManager.get("GoalVehicle"),
            )
            if goal_obj:
                goal_obj.meta = {"source": "Goal", "point_type": "Goal"}
                objects.append(goal_obj)

        # 3. 停车位
        if frame.slot_pts:
            objects.append(SceneObject(
                id="slot",
                layer="Slot",
                points=frame.slot_pts,
                color=ColorManager.get("Slot"),
                line_width=2.5,
                meta={"source": "Slot", "point_type": "Slot"},
            ))

        # 4. PDC 数据（原始传感器数据）
        if frame.left_pdc:
            objects.append(SceneObject(
                id="left_pdc",
                layer="LeftPDC",
                points=frame.left_pdc,
                color=ColorManager.get_by_source("PDC", "PDC"),
                meta={"source": "PDC", "point_type": "PDC"},
            ))
        if frame.right_pdc:
            objects.append(SceneObject(
                id="right_pdc",
                layer="RightPDC",
                points=frame.right_pdc,
                color=ColorManager.get_by_source("PDC", "PDC"),
                meta={"source": "PDC", "point_type": "PDC"},
            ))

        # 5. Fusion 数据
        for stage in frame.fusion_stages:
            source = SceneBuilder._get_source(stage.name)
            prefix = stage.name

            # BSeg: 原始边界点（蓝色系）
            if stage.bseg and not stage.bseg.is_empty:
                layer_name = f"{source}_BSeg" if source else "BSeg"
                objects.append(SceneObject(
                    id=f"{prefix}_bseg",
                    layer=layer_name,
                    points=stage.bseg.points,
                    color=ColorManager.get_by_source(source, "BSeg"),
                    line_width=2.0,
                    meta={
                        "source": source,
                        "point_type": "BSeg",
                        "stage_name": stage.name,
                        "description": f"{source} 原始边界点" if source else "原始边界点",
                    },
                ))

            # NSeg: 新增点（橙色系）
            if stage.nseg and not stage.nseg.is_empty:
                layer_name = f"{source}_NSeg" if source else "NSeg"
                objects.append(SceneObject(
                    id=f"{prefix}_nseg",
                    layer=layer_name,
                    points=stage.nseg.points,
                    color=ColorManager.get_by_source(source, "NSeg"),
                    line_width=2.0,
                    meta={
                        "source": source,
                        "point_type": "NSeg",
                        "stage_name": stage.name,
                        "description": f"{source} 新增点" if source else "新增点",
                    },
                ))

            # FusSeg: 融合结果（红色系）
            if stage.fusseg and not stage.fusseg.is_empty:
                layer_name = f"{source}_FusSeg" if source else "FusSeg"
                objects.append(SceneObject(
                    id=f"{prefix}_fusseg",
                    layer=layer_name,
                    points=stage.fusseg.points,
                    color=ColorManager.get_by_source(source, "FusSeg"),
                    line_width=2.5,
                    meta={
                        "source": source,
                        "point_type": "FusSeg",
                        "stage_name": stage.name,
                        "description": f"{source} 融合结果" if source else "融合结果",
                    },
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
                meta={"source": "Boundary", "point_type": "Boundary"},
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
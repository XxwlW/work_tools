from dataclasses import dataclass, field
from .point import Point
from .pose import Pose
from .fusion import FusionStage
from .boundary import Boundary


@dataclass
class SlotInfo:
    """停车位信息"""
    label: int = 0
    corner_pts: list[Point] = field(default_factory=list)
    slot_depth: float = 0.0
    slot_length: float = 0.0


@dataclass
class FrameData:
    """核心数据模型：单个Frame的全部数据"""
    seq_num: int
    current_pose: Pose | None = None
    goal_pose: Pose | None = None
    slot_pts: list[Point] = field(default_factory=list)
    slot_info: SlotInfo | None = None
    left_pdc: list[Point] = field(default_factory=list)
    right_pdc: list[Point] = field(default_factory=list)
    fusion_stages: list[FusionStage] = field(default_factory=list)
    boundaries: list[Boundary] = field(default_factory=list)
    raw_text: str = ""

    @property
    def has_fusion_data(self) -> bool:
        return any(stage.has_data for stage in self.fusion_stages)

    @property
    def has_boundary_data(self) -> bool:
        return any(not b.is_empty for b in self.boundaries)

    @property
    def has_pdc_data(self) -> bool:
        return len(self.left_pdc) > 0 or len(self.right_pdc) > 0

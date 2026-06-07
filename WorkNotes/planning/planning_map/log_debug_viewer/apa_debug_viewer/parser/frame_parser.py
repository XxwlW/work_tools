from model.frame import FrameData
from model.point import Point
from .fusion_parser import FusionParser
from .boundary_parser import BoundaryParser
from .vehicle_parser import VehicleParser
from .common import ParserUtils


class FrameParser:
    """
    主 Frame 解析器
    组合多个子解析器将 frame 文本块解析为 FrameData
    """

    def __init__(self):
        self.fusion_parser = FusionParser()
        self.boundary_parser = BoundaryParser()
        self.vehicle_parser = VehicleParser()

    def parse(self, frame_text: str, seq_num: int = 0) -> FrameData:
        """将 frame 文本解析为 FrameData"""
        frame = FrameData(seq_num=seq_num, raw_text=frame_text)

        # 1. 解析车辆位姿
        pose = self.vehicle_parser.parse_current_pose(frame_text)
        if pose is not None:
            frame.current_pose = pose

        # 2. 解析目标位姿
        goal = self.vehicle_parser.parse_goal_pose(frame_text)
        if goal is not None:
            frame.goal_pose = goal

        # 3. 解析停车位角点
        slot_pts = self.vehicle_parser.parse_slot_points(frame_text)
        if slot_pts:
            frame.slot_pts = slot_pts

        # 4. 解析 PDC 数据
        left_pdc, right_pdc = self.vehicle_parser.parse_pdc_data(frame_text)
        frame.left_pdc = left_pdc
        frame.right_pdc = right_pdc

        # 5. 解析 Fusion 阶段
        fusion_stages = self.fusion_parser.parse_all(frame_text)
        if fusion_stages:
            frame.fusion_stages = fusion_stages

        # 6. 解析 Boundary
        boundaries = self.boundary_parser.parse_all(frame_text)
        if boundaries:
            frame.boundaries = boundaries

        return frame

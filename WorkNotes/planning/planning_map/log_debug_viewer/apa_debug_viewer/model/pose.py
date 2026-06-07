from dataclasses import dataclass


@dataclass
class Pose:
    """车辆位姿，单位：meter, radian"""
    x: float
    y: float
    yaw: float

    def __repr__(self) -> str:
        return f"Pose({self.x:.3f}, {self.y:.3f}, {self.yaw:.3f})"

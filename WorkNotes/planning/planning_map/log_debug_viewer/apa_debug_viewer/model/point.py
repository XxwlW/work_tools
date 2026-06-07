from dataclasses import dataclass


@dataclass
class Point:
    """二维点，单位：meter"""
    x: float
    y: float

    def __repr__(self) -> str:
        return f"Point({self.x:.3f}, {self.y:.3f})"

from dataclasses import dataclass, field
from .point import Point


@dataclass
class Boundary:
    """边界数据"""
    name: str
    points: list[Point] = field(default_factory=list)

    @property
    def is_empty(self) -> bool:
        return len(self.points) == 0

    @property
    def size(self) -> int:
        return len(self.points)

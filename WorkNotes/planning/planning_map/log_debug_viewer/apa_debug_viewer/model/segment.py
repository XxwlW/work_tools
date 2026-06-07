from dataclasses import dataclass, field
from .point import Point


@dataclass
class Segment:
    """线段数据，对应 BSegData / NSegData / FusSegData"""
    points: list[Point] = field(default_factory=list)

    @property
    def is_empty(self) -> bool:
        return len(self.points) == 0

    @property
    def size(self) -> int:
        return len(self.points)

from dataclasses import dataclass, field
from .segment import Segment


@dataclass
class FusionStage:
    """融合阶段数据"""
    name: str
    bseg: Segment | None = None
    nseg: Segment | None = None
    fusseg: Segment | None = None
    debug: dict | None = field(default_factory=dict)

    @property
    def has_data(self) -> bool:
        return (self.bseg is not None and not self.bseg.is_empty) or \
               (self.nseg is not None and not self.nseg.is_empty) or \
               (self.fusseg is not None and not self.fusseg.is_empty)

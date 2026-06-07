from dataclasses import dataclass, field


@dataclass
class Layer:
    """图层定义"""
    name: str
    visible: bool = True
    description: str = ""


class LayerManager:
    """图层管理器，控制各图层的可见性"""

    _DEFAULT_LAYERS = [
        Layer("CurrentVehicle", True, "当前车辆位置"),
        Layer("GoalVehicle", True, "目标车辆位置"),
        Layer("Slot", True, "停车位"),
        Layer("LeftPDC", True, "左侧 PDC 原始点"),
        Layer("RightPDC", True, "右侧 PDC 原始点"),
        Layer("BSeg", True, "原始边界点 (Base Segment)"),
        Layer("NSeg", True, "新增边界点 (New Segment)"),
        Layer("FusSeg", True, "融合结果 (Fusion Segment)"),
        Layer("Boundary", True, "主边界 (Main Boundary)"),
        Layer("SubBoundary", True, "对向边界 (Sub Boundary)"),
        Layer("Path", True, "规划路径"),
    ]

    def __init__(self):
        self._layers: dict[str, Layer] = {}
        for layer in self._DEFAULT_LAYERS:
            self._layers[layer.name] = layer

    def get(self, name: str) -> Layer | None:
        """获取图层"""
        return self._layers.get(name)

    def set_visible(self, name: str, visible: bool) -> None:
        """设置图层可见性"""
        if name in self._layers:
            self._layers[name].visible = visible

    def toggle(self, name: str) -> None:
        """切换图层可见性"""
        if name in self._layers:
            self._layers[name].visible = not self._layers[name].visible

    def is_visible(self, name: str) -> bool:
        """检查图层是否可见"""
        layer = self._layers.get(name)
        return layer is not None and layer.visible

    def get_all_layers(self) -> list[Layer]:
        """获取所有图层"""
        return list(self._layers.values())

    def get_visible_layers(self) -> list[Layer]:
        """获取所有可见图层"""
        return [l for l in self._layers.values() if l.visible]

    def register(self, name: str, visible: bool = True, description: str = "") -> None:
        """注册新图层"""
        if name not in self._layers:
            self._layers[name] = Layer(name=name, visible=visible, description=description)

    def reset(self) -> None:
        """重置所有图层到默认可见"""
        for layer in self._layers.values():
            layer.visible = True

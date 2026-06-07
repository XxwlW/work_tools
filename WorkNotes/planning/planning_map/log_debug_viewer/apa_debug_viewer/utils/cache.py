from datetime import datetime, timedelta


class Cache:
    """
    简单内存缓存
    用于缓存解析结果，避免重复解析
    """

    def __init__(self, ttl_seconds: int = 300):
        self._store: dict[str, tuple[object, datetime]] = {}
        self._ttl = timedelta(seconds=ttl_seconds)

    def get(self, key: str) -> object | None:
        """获取缓存"""
        if key not in self._store:
            return None
        value, timestamp = self._store[key]
        if datetime.now() - timestamp > self._ttl:
            del self._store[key]
            return None
        return value

    def set(self, key: str, value: object) -> None:
        """设置缓存"""
        self._store[key] = (value, datetime.now())

    def has(self, key: str) -> bool:
        """检查是否存在"""
        return self.get(key) is not None

    def clear(self) -> None:
        """清空缓存"""
        self._store.clear()

    def remove(self, key: str) -> None:
        """移除指定缓存"""
        self._store.pop(key, None)

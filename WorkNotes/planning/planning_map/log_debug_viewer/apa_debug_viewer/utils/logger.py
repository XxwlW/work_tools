import sys
from datetime import datetime


class Logger:
    """简单日志工具"""

    LEVELS = {
        "DEBUG": 0,
        "INFO": 1,
        "WARN": 2,
        "ERROR": 3,
    }

    def __init__(self, name: str = "APA", level: str = "INFO"):
        self.name = name
        self.level = self.LEVELS.get(level.upper(), 1)

    def debug(self, msg: str) -> None:
        if self.level <= self.LEVELS["DEBUG"]:
            self._log("DEBUG", msg)

    def info(self, msg: str) -> None:
        if self.level <= self.LEVELS["INFO"]:
            self._log("INFO", msg)

    def warn(self, msg: str) -> None:
        if self.level <= self.LEVELS["WARN"]:
            self._log("WARN", msg)

    def error(self, msg: str) -> None:
        if self.level <= self.LEVELS["ERROR"]:
            self._log("ERROR", msg)

    def _log(self, level: str, msg: str) -> None:
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        print(f"[{timestamp}] [{self.name}] [{level}] {msg}", file=sys.stderr)

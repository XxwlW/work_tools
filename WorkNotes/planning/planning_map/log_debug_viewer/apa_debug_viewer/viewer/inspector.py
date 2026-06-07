from model.frame import FrameData


class Inspector:
    """
    数据检查器
    用于查看 Frame 的详细信息、调试数据
    """

    @staticmethod
    def inspect(frame: FrameData) -> dict:
        """提取 Frame 的概要信息"""
        info = {
            "seq_num": frame.seq_num,
            "current_pose": str(frame.current_pose) if frame.current_pose else "N/A",
            "goal_pose": str(frame.goal_pose) if frame.goal_pose else "N/A",
            "slot_points": len(frame.slot_pts),
            "left_pdc": len(frame.left_pdc),
            "right_pdc": len(frame.right_pdc),
            "fusion_stages": [],
            "boundaries": [],
        }

        for stage in frame.fusion_stages:
            stage_info = {
                "name": stage.name,
                "bseg": stage.bseg.size if stage.bseg else 0,
                "nseg": stage.nseg.size if stage.nseg else 0,
                "fusseg": stage.fusseg.size if stage.fusseg else 0,
            }
            info["fusion_stages"].append(stage_info)

        for boundary in frame.boundaries:
            info["boundaries"].append({
                "name": boundary.name,
                "points": boundary.size,
            })

        return info

    @staticmethod
    def format_info(frame: FrameData) -> str:
        """格式化为可读字符串"""
        info = Inspector.inspect(frame)
        lines = [
            f"{'='*50}",
            f"Frame #{info['seq_num']}",
            f"{'='*50}",
            f"Current Pose : {info['current_pose']}",
            f"Goal Pose    : {info['goal_pose']}",
            f"Slot Points  : {info['slot_points']}",
            f"Left PDC     : {info['left_pdc']}",
            f"Right PDC    : {info['right_pdc']}",
        ]

        if info["fusion_stages"]:
            lines.append(f"\nFusion Stages:")
            for s in info["fusion_stages"]:
                lines.append(
                    f"  [{s['name']}] "
                    f"BSeg={s['bseg']}, "
                    f"NSeg={s['nseg']}, "
                    f"FusSeg={s['fusseg']}"
                )

        if info["boundaries"]:
            lines.append(f"\nBoundaries:")
            for b in info["boundaries"]:
                lines.append(f"  {b['name']}: {b['points']} points")

        return "\n".join(lines)

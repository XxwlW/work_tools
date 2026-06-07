"""Debug vehicle rendering across frames."""
import sys
sys.path.insert(0, '.')

import builtins
_original_print = builtins.print
def silent_print(*args, **kwargs): pass
builtins.print = silent_print

import matplotlib
matplotlib.use('Agg')

from viewer.app import App

app = App()
app.load_log('0604_162450.log')
builtins.print = _original_print

# Find frames with significantly different poses
frames_with_pose = []
for i, f in enumerate(app.timeline._frames):
    if f.current_pose is not None:
        frames_with_pose.append((i, f.seq_num, f.current_pose))

print(f"Total frames with pose: {len(frames_with_pose)}")

# Find frames where pose changes significantly
prev_pose = None
changed_frames = []
for idx, seq, pose in frames_with_pose:
    if prev_pose is not None:
        dx = abs(pose.x - prev_pose.x)
        dy = abs(pose.y - prev_pose.y)
        if dx > 0.5 or dy > 0.5:  # moved more than 0.5m
            changed_frames.append((idx, seq, pose, dx, dy))
    prev_pose = pose

print(f"Frames where vehicle moved > 0.5m: {len(changed_frames)}")
if changed_frames:
    for idx, seq, pose, dx, dy in changed_frames[:5]:
        print(f"  idx={idx}, seq={seq}: x={pose.x:.3f}, y={pose.y:.3f}, dx={dx:.3f}, dy={dy:.3f}")

# Check a few frames around first significant movement
if changed_frames:
    idx, seq, pose, _, _ = changed_frames[0]
    # Check this frame and next frame
    for offset in [0, 1, 2]:
        if idx + offset < len(app.timeline._frames):
            app.timeline.go_to(idx + offset)
            f = app.timeline.current_frame
            scene = app.scene_builder.build(f)
            veh = next((obj for obj in scene if obj.id == "current_vehicle"), None)
            print(f"\nFrame idx={idx+offset} (seq={f.seq_num}): current_pose={f.current_pose}")
            if veh:
                print(f"  vehicle center: ({f.current_pose.x:.3f}, {f.current_pose.y:.3f})")
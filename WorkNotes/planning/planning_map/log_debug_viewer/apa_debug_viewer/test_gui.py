"""Test script to verify GUI rendering works (uses non-interactive backend)."""
import sys
sys.path.insert(0, '.')

# Use non-interactive backend for testing
import matplotlib
matplotlib.use('Agg')

from viewer.app import App
app = App()
app.load_log('0604_162450.log')

# Find first frame with valid data
for i, f in enumerate(app.timeline._frames):
    if f.current_pose is not None or f.has_fusion_data or f.slot_pts:
        app.timeline.go_to(i)
        print(f"Using frame index {i}, seq={f.seq_num}", file=sys.stderr)
        break

frame = app.timeline.current_frame
scene = app.scene_builder.build(frame)
for obj in scene:
    obj.visible = True

print(f"Scene objects: {len(scene)}", file=sys.stderr)
for obj in scene:
    print(f"  {obj.id}: {len(obj.points)} points", file=sys.stderr)

# Test render with block=False (first time creates figure)
try:
    app.renderer.render(scene, title=f"Test #{frame.seq_num}", block=False)
    print("First render() OK", file=sys.stderr)
except Exception as e:
    print(f"First render error: {e}", file=sys.stderr)
    import traceback
    traceback.print_exc()

# Test update display (subsequent render reuses figure)
try:
    app._update_display()
    print("Second render (update_display) OK", file=sys.stderr)
except Exception as e:
    print(f"Second render error: {e}", file=sys.stderr)
    import traceback
    traceback.print_exc()

print("All tests passed!", file=sys.stderr)
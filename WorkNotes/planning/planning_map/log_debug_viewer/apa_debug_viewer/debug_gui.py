"""Debug script to test GUI rendering step by step."""
import sys
sys.path.insert(0, '.')

# Force non-interactive backend for headless testing
import matplotlib
matplotlib.use('Agg')

from viewer.app import App

print("=== Step 1: Create App ===", file=sys.stderr)
app = App()

print("=== Step 2: Load log ===", file=sys.stderr)
app.load_log('0604_162450.log')

print("=== Step 3: Find frame with data ===", file=sys.stderr)
for i, f in enumerate(app.timeline._frames):
    if f.has_fusion_data:
        app.timeline.go_to(i)
        print(f"  Selected frame index={i}, seq={f.seq_num}, fusion_stages={len(f.fusion_stages)}", file=sys.stderr)
        break

frame = app.timeline.current_frame
print(f"  Current frame: seq={frame.seq_num}", file=sys.stderr)
print(f"  Has fusion: {frame.has_fusion_data}", file=sys.stderr)
print(f"  Fusion stages: {len(frame.fusion_stages)}", file=sys.stderr)

print("=== Step 4: Build scene ===", file=sys.stderr)
scene = app.scene_builder.build(frame)
for obj in scene:
    obj.visible = True  # Force all visible
print(f"  Scene objects: {len(scene)}", file=sys.stderr)
for obj in scene:
    print(f"    {obj.id}: layer={obj.layer}, visible={obj.visible}, points={len(obj.points)}", file=sys.stderr)

print("=== Step 5: First render(block=False) ===", file=sys.stderr)
try:
    title = f"Test #{frame.seq_num}"
    app.renderer.render(scene, title=title, block=False)
    print("  render() succeeded", file=sys.stderr)
    print(f"  renderer.fig is None: {app.renderer.fig is None}", file=sys.stderr)
    print(f"  renderer.ax is None: {app.renderer.ax is None}", file=sys.stderr)
except Exception as e:
    print(f"  render() FAILED: {e}", file=sys.stderr)
    import traceback
    traceback.print_exc(file=sys.stderr)

print("=== Step 6: Check if canvas has content ===", file=sys.stderr)
if app.renderer.fig:
    print(f"  fig size: {app.renderer.fig.get_size_inches()}", file=sys.stderr)
    print(f"  ax has children: {len(app.renderer.ax.get_children())}", file=sys.stderr)
    # Check what was drawn
    for artist in app.renderer.ax.get_children():
        print(f"    Artist: {type(artist).__name__}", file=sys.stderr)

print("=== DONE ===", file=sys.stderr)
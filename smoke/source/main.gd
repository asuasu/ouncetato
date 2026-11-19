extends Node2D

var hue = 0.0

func _ready():
	print("Smoke test OS: ", OS.get_name())
	print("Smoke test engine: ", Engine.get_version_info()["string"])
	var file = File.new()
	var error = file.open("user://smoke-save.txt", File.WRITE)
	if error == OK:
		file.store_string("Godot Switch 2 save succeeded\n")
		file.close()
		print("Smoke test save succeeded")
	else:
		printerr("Smoke test save failed: ", error)

func _process(delta):
	hue = fmod(hue + delta * 0.15, 1.0)
	$Triangle.rotation += delta
	$Triangle.color = Color.from_hsv(hue, 0.65, 1.0)
	$Status.text = "FPS: %d\nA: change background\nB: exit" % Engine.get_frames_per_second()

func _input(event):
	if event is InputEventJoypadButton and event.pressed:
		if event.button_index == 0:
			var color = Color.from_hsv(fmod(hue + 0.5, 1.0), 0.65, 0.65)
			$Background.color = color
			VisualServer.set_default_clear_color(color)
			print("Smoke test A pressed; background changed to ", color, ", rect size ", $Background.rect_size)
		elif event.button_index == 1:
			get_tree().quit()

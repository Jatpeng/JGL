from init import create_engine, create_scene


engine = create_engine()
scene = create_scene(engine, "three_cubes")

light = scene.create_light("main_light")
light.transform.position = (3.0, 4.0, 5.0)
light.color = (1.0, 1.0, 1.0)
light.strength = 120.0

positions = [
    (-2.5, 0.0, 0.0),
    (0.0, 0.0, 0.0),
    (2.5, 0.0, 0.0),
]

for index, position in enumerate(positions):
    cube = scene.create_mesh(f"cube_{index}")
    cube.set_model("Assets/models/cube.fbx")
    cube.set_material("Assets/materials/PBR.xml")
    cube.transform.position = position
    cube.transform.scale = (1.0, 1.0, 1.0)

engine.run()

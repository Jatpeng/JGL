from init import create_engine, create_scene, jgl


engine = create_engine()
scene = create_scene(engine, "three_cubes")

light = scene.create_light("main_light")
light.transform.position = (3.0, 4.0, 5.0)
light.type = jgl.LightType.DIRECTIONAL
light.color = (1.0, 1.0, 1.0)
light.strength = 3.5
light.direction = (-0.35, -1.0, -0.25)
light.casts_shadows = True

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

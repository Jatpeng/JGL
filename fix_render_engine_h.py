with open("JGL_Engine/source/engine/render_engine.h", "r") as f:
    lines = f.readlines()
with open("JGL_Engine/source/engine/render_engine.h", "w") as f:
    i = 0
    while i < len(lines):
        if lines[i].startswith("<<<<<<< HEAD"):
            i += 1
            while not lines[i].startswith("======="):
                f.write(lines[i])
                i += 1
            i += 1
            while not lines[i].startswith(">>>>>>> origin/master"):
                i += 1
            i += 1
            continue
        f.write(lines[i])
        i += 1

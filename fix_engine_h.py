with open("JGL_Engine/source/engine/engine.h", "r") as f:
    lines = f.readlines()
with open("JGL_Engine/source/engine/engine.h", "w") as f:
    i = 0
    while i < len(lines):
        if lines[i].startswith("<<<<<<< HEAD"):
            i += 1
            while not lines[i].startswith("======="):
                i += 1
            i += 1
            while not lines[i].startswith(">>>>>>> origin/master"):
                f.write(lines[i])
                i += 1
            i += 1
            continue
        f.write(lines[i])
        i += 1

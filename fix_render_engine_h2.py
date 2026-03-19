with open("JGL_Engine/source/engine/render_engine.h", "r") as f:
    text = f.read()

import re

# Remove any remaining merge conflict markers
text = re.sub(r'<<<<<<< HEAD\n.*?\n=======\n', '', text, flags=re.DOTALL)
text = re.sub(r'>>>>>>> origin/master\n', '', text)
text = re.sub(r'<<<<<<< HEAD.*?=======\n', '', text, flags=re.DOTALL)
text = re.sub(r'>>>>>>> [0-9a-fA-F]{40}\n', '', text)
text = re.sub(r'<<<<<<< HEAD.*?\n', '', text)
text = re.sub(r'=======\n', '', text)
text = re.sub(r'>>>>>>> .*?\n', '', text)

with open("JGL_Engine/source/engine/render_engine.h", "w") as f:
    f.write(text)

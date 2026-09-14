from PIL import Image
from pathlib import Path

paths = [
    Path(r'C:\Users\malfonso\AppData\Local\Temp\codex-clipboard-3e4e02cb-6c72-4612-b90e-dcfa92da1a9f.png'),
    Path(r'C:\Users\malfonso\AppData\Local\Temp\codex-clipboard-69a5cb75-aca0-44d4-af8a-ae0317fd79d2.png'),
    Path(r'C:\Users\malfonso\AppData\Local\Temp\codex-clipboard-a664218f-1464-40d2-bb60-30720a8c83fe.png'),
]
for p in paths:
    im = Image.open(p)
    print(p.name, im.size, im.mode)

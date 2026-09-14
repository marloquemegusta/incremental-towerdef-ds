from PIL import Image
from pathlib import Path

paths = [
Path(r'C:\Users\malfonso\AppData\Local\Temp\codex-clipboard-3e4e02cb-6c72-4612-b90e-dcfa92da1a9f.png'),
Path(r'C:\Users\malfonso\AppData\Local\Temp\codex-clipboard-69a5cb75-aca0-44d4-af8a-ae0317fd79d2.png'),
Path(r'C:\Users\malfonso\AppData\Local\Temp\codex-clipboard-a664218f-1464-40d2-bb60-30720a8c83fe.png')]
for p in paths:
    im=Image.open(p).convert('RGBA')
    xs=[]; ys=[]
    for y in range(im.height):
        for x in range(im.width):
            if im.getpixel((x,y))[3] > 20:
                xs.append(x); ys.append(y)
    print(p.name, im.size, (min(xs),min(ys),max(xs)+1,max(ys)+1))
    # occupied runs per row/column, useful for locating regular sprite blocks
    for axis in ('x','y'):
        vals=range(im.width) if axis=='x' else range(im.height)
        runs=[]; start=None
        for i in vals:
            hit=any((im.getpixel((i,j))[3]>20 if axis=='x' else im.getpixel((j,i))[3]>20) for j in (range(im.height) if axis=='x' else range(im.width)))
            if hit and start is None: start=i
            if not hit and start is not None: runs.append((start,i)); start=None
        if start is not None: runs.append((start,(im.width if axis=='x' else im.height)))
        print(axis, runs[:30])

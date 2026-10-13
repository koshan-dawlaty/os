#!/usr/bin/env python3
from pathlib import Path
import subprocess

directory_path = Path("images/")

for item in directory_path.iterdir():
    if item.is_file():
        print(str(item))
        subprocess.run(["./build/convert_image.sh", str(item.absolute()), "fs_root/images/" + item.stem + ".tga"])
        

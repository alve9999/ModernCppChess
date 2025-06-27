
import subprocess
import sys
from pathlib import Path

dir_path = Path(sys.argv[1])
combined_file = Path("combined_output.txt")

with combined_file.open("a") as out_f:
    for file in dir_path.iterdir():
        if file.is_file():
            subprocess.run([
                "./primer", "convert",
                str(file),
                "test.plain",
                "--min-ply 16",
                "--max-score 2000",
                "--filter-win 300",
                "--filter-loss 300",
            ])
            with open("test.plain", "r") as temp_f:
                out_f.write(temp_f.read())


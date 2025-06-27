import sqlite3
from pathlib import Path

db_path = "eval.db"
input_file = Path("pos.plain")

conn = sqlite3.connect(db_path)
cur = conn.cursor()

cur.execute("""
CREATE TABLE IF NOT EXISTS evals (
    id INTEGER PRIMARY KEY,
    fen TEXT NOT NULL,
    evaluation REAL NOT NULL,
    wdl REAL NOT NULL
)
""")

with input_file.open("r") as f:
    for line in f:
        line = line.strip()
        if not line:
            continue
        # Example line:
        # r1r3k1/1p3ppp/2b1pb2/p1Nn4/8/P3PB2/1PRB1PPP/1R3K2 w - - 4 25 | 28 | 1

        parts = line.split('|')
        if len(parts) < 2:
            continue

        fen = parts[0].strip()
        # Evaluation is the last part after the last '|'
        # We'll take the last element and convert to float
        try:
            evaluation = float(parts[1].strip())
            wdl = float(parts[2].strip())
        except ValueError:
            continue

        cur.execute("INSERT INTO evals (fen, evaluation, wdl) VALUES (?, ?, ?)", (fen, evaluation, wdl))

conn.commit()
conn.close()


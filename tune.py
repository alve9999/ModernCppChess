import optuna
import pickle
import subprocess
import os


def make_setoption_args(params):
    args = []
    for i, (name, val) in enumerate(params.items()):
        line = f"setvalue {name} {val}"
        if i < len(params) - 1:
            line += "\\n"
        args.append(line)
    return args


def run_match(params,name):
    setoptions = make_setoption_args({
        "KILLER_MOVE_BONUS": int(params["KILLER_MOVE_BONUS"]),
        "COUNTER_HISTORY_BONUS": int(params["COUNTER_HISTORY_BONUS"]),
        "DELTA_MARGIN": int(params["DELTA_MARGIN"]),
        "RFP_MARGIN": int(params["RFP_MARGIN"]),
        "FP_BASE": int(params["FP_BASE"]),
        "FP_ADD": int(params["FP_ADD"]),
        "WINDOW_INIT": int(params["WINDOW_INIT"]),
        "WINDOW_MULT": int(params["WINDOW_MULT"]),
        "HISTORY_AGE_FACTOR": round(params["HISTORY_AGE_FACTOR"], 2)
    })

    print(setoptions)
    
    cmd_str = f'cutechess-cli -engine cmd=./chess2000 initstr="{"".join(setoptions)}" name=test -engine cmd=./chess2000 name=op -each tc=5+0.05 proto=uci -games 20 -pgnout result{name}.pgn -openings file=../Pohl.epd format=epd -concurrency 7'
    
    print("Running command:", cmd_str)
    
    try:
        result = subprocess.run(cmd_str, shell=True, capture_output=True, text=True)
        
        if result.returncode != 0:
            print(f"Command failed with return code {result.returncode}")
            print(f"stderr: {result.stderr}")
            return 0.5
    except FileNotFoundError:
        print("cutechess-cli not found. Please install it or add it to PATH.")
        return 0.5

    result_file = f"result{name}.pgn" 
    if not os.path.exists(result_file):
        print("PGN result file not found.")
        return 0.5

    with open(result_file) as f:
        pgn = f.read()
    wins, draws, losses = 0, 0, 0
    for game in pgn.split("[Event \"?\"]"):
        if "[White \"test\"]" in game:
            if "1-0" in game:
                wins += 1
            elif "0-1" in game:
                losses += 1
            else:
                draws += 1
        elif "[Black \"test\"]" in game:
            if "1-0" in game:
                losses += 1
            elif "0-1" in game:
                wins += 1
            else:
                draws += 1
    print(f"Results: Wins: {wins}, Draws: {draws}, Losses: {losses}")
    return (wins + 0.5 * draws) / 20.0

def objective(trial):
    return run_match({
        "KILLER_MOVE_BONUS": trial.suggest_int("KILLER_MOVE_BONUS", 8000, 15000, step=500),
        "COUNTER_HISTORY_BONUS": trial.suggest_int("COUNTER_HISTORY_BONUS", 1000, 10000, step=500),
        "DELTA_MARGIN": trial.suggest_int("DELTA_MARGIN", 100, 2000, step=100),
        "RFP_MARGIN": trial.suggest_int("RFP_MARGIN", 50, 300, step=10),
        "FP_BASE": trial.suggest_int("FP_BASE", 100, 500, step=50),
        "FP_ADD": trial.suggest_int("FP_ADD", 50, 500, step=50),
        "WINDOW_INIT": trial.suggest_int("WINDOW_INIT", 10, 50, step=5),
        "WINDOW_MULT": trial.suggest_int("WINDOW_MULT", 1, 4),
        "HISTORY_AGE_FACTOR": trial.suggest_float("HISTORY_AGE_FACTOR", 0.5, 5.0, step=0.1)
    },str(trial.number))


def progress_callback(study, trial):
    print(f"Trial {trial.number} | Value: {trial.value} | Params: {trial.params}")
    if trial.number % 10 == 0:
        with open("backup.pkl", "wb") as f:
            pickle.dump(study, f)

if __name__ == "__main__":
    study = optuna.create_study(direction="maximize",study_name="tuning_run",
    storage="sqlite:///optuna.db",load_if_exists=True)
    study.optimize(objective, n_trials=10000,callbacks=[progress_callback])
    print("Best params:", study.best_params)


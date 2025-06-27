import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import Dataset, DataLoader, random_split
import numpy as np
import sqlite3
import random
from tqdm import tqdm
from torch.amp import GradScaler, autocast
import os
from torch.optim.lr_scheduler import LambdaLR

PIECE_TO_IDX = {'P':0,'N':1,'B':2,'R':3,'Q':4,'K':5,'p':6,'n':7,'b':8,'r':9,'q':10,'k':11}
CENTIPAWN_EVAL_SCALE = 410

INPUT_SIZE = 768
HL_SIZE = 512


def fen_to_features(fen, perspective='white'):
    board_part = fen.split()[0]
    features = np.zeros(768, dtype=np.float32)
    square = 0

    for char in board_part:
        if char == '/':
            continue
        elif char.isdigit():
            square += int(char)
        else:
            sq = square
            side = 0 if char.isupper() else 1 # 1 for black piece (lowercase), 0 for white piece (uppercase)
            piece_char = char.upper()
            piece_type_idx = {
                'P': 0, 'N': 1, 'B': 2, 'R': 3, 'Q': 4, 'K': 5
            }[piece_char]

            if perspective == 'black':
                # Flip rank only: (7-rank)*8 + file
                sq = (7 - (square // 8)) * 8 + (square % 8)
                side = side ^ 1 # Invert side: white pieces become 'black side' features, black pieces become 'white side' features

            feature_idx = side * 6 * 64 + piece_type_idx * 64 + sq
            features[feature_idx] = 1
            square += 1

    return features
class ChessDataset(Dataset):
    def __init__(self, db_path, chunk_size=10000000, filter_outliers=True):
        self.db_path = db_path
        self.chunk_size = chunk_size
        self.filter_outliers = filter_outliers
        self.current_epoch = 0

        print(f"Connecting to database at {db_path} to get total length...")
        self.total_length = 100000000
        print(f"Total number of samples in the database: {self.total_length}")

        self._load_new_chunk()

    def _load_new_chunk(self):
        """Load a new random chunk from the database for each epoch."""
        max_start_rowid = max(1, self.total_length - self.chunk_size)
        chunk_start_rowid = random.randint(1, max_start_rowid) if max_start_rowid > 1 else 1

        conn = sqlite3.connect(self.db_path)
        cursor = conn.execute(
            "SELECT fen, evaluation FROM evals WHERE rowid >= ? LIMIT ?",
            (chunk_start_rowid, self.chunk_size),
        )
        raw_data = cursor.fetchall()
        conn.close()
        self.chunk_data = raw_data

        np.random.shuffle(self.chunk_data)

    def new_epoch(self):
        self.current_epoch += 1
        self._load_new_chunk()

    def __len__(self):
        return len(self.chunk_data)

    def __getitem__(self, idx):
        item = self.chunk_data[idx]
        fen, eval_score = item[0], item[1]

        stm_char = fen.split()[1]
        is_white_stm = (stm_char == 'w')

        feat_white_perspective = fen_to_features(fen, perspective='white')
        feat_black_perspective = fen_to_features(fen, perspective='black')

        eval_target_wdl = torch.sigmoid(torch.tensor(-eval_score, dtype=torch.float32)/410)

        if is_white_stm:
            features_stm_perspective = feat_white_perspective
            features_nstm_perspective = feat_black_perspective
        else:
            features_stm_perspective = feat_black_perspective
            features_nstm_perspective = feat_white_perspective
        
        features = (torch.FloatTensor(features_stm_perspective),
                    torch.FloatTensor(features_nstm_perspective),
                    torch.tensor(is_white_stm, dtype=torch.float32))
        
        return features, eval_target_wdl

def worker_init_fn(worker_id):
    worker_info = torch.utils.data.get_worker_info()
    if worker_info is not None:
        seed = torch.seed() % (2**32 - 1) + worker_id
        dataset = worker_info.dataset
        if isinstance(dataset, torch.utils.data.Subset):
            dataset = dataset.dataset
        if hasattr(dataset, 'current_epoch'):
            seed += dataset.current_epoch
        random.seed(seed)
        np.random.seed(seed)

class NNUE(nn.Module):
    def __init__(self, input_size, ft_size):
        super(NNUE, self).__init__()
        M = ft_size
        N = 32
        K = 1

        self.ft = nn.Linear(768, M)
        self.l1 = nn.Linear(2 * M, N)
        self.l2 = nn.Linear(N, K)

    def forward(self, white_features, black_features, stm):
        w = self.ft(white_features)
        b = self.ft(black_features)


        stm = stm.unsqueeze(1)  # shape [batch_size, 1]
        accumulator = (stm * torch.cat([w, b], dim=1)) + ((1 - stm) * torch.cat([b, w], dim=1))
        l1_x = torch.clamp(accumulator, 0.0, 1.0)
        l2_x = torch.clamp(self.l1(l1_x), 0.0, 1.0)
        return self.l2(l2_x)

def loss_to_centipawn_error(mse_wdl):
    return np.sqrt(mse_wdl)


def save_checkpoint(model, optimizer, scheduler, epoch, filepath):
    checkpoint = {
        'epoch': epoch,
        'model_state_dict': model.state_dict(),
        'optimizer_state_dict': optimizer.state_dict(),
        'scheduler_state_dict': scheduler.state_dict(),
    }
    torch.save(checkpoint, filepath)
    print(f"Checkpoint saved to {filepath} at epoch {epoch}")

def load_checkpoint(filepath, device):
    checkpoint = torch.load(filepath, map_location=device)
    model = NNUE(input_size=INPUT_SIZE, ft_size=HL_SIZE).to(device)
    model.load_state_dict(checkpoint['model_state_dict'])

    optimizer = optim.AdamW(model.parameters(), lr=0.001) # Initial lr doesn't matter, it will be overwritten
    optimizer.load_state_dict(checkpoint['optimizer_state_dict'])

    scheduler = optim.lr_scheduler.ReduceLROnPlateau(
        optimizer, mode='min', factor=0.8, patience=5, threshold=0.0005, min_lr=1e-7
    )
    scheduler.load_state_dict(checkpoint['scheduler_state_dict'])
    epoch = checkpoint['epoch']

    print(f"Checkpoint loaded from {filepath}. Resuming from epoch {epoch+1}.")
    return model, optimizer, scheduler, epoch

def train_nnue(db_path, epochs=1000, batch_size=1024, lr=0.001, val_split=0.1, weight_decay=1e-8, exponent=2.0, resume_from=False):
    print("loading dataset...")
    base_dataset = ChessDataset(db_path, filter_outliers=True, chunk_size=5000000)
    print(f"Dataset loaded with {len(base_dataset)} initial samples.")
    device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
    print(f"Using device: {device}")

    start_epoch = 0
    model = NNUE(input_size=INPUT_SIZE, ft_size=HL_SIZE).to(device)
    optimizer = optim.AdamW(model.parameters(), lr=lr, weight_decay=weight_decay, 
                           betas=(0.9, 0.999), eps=1e-8)
    scheduler = optim.lr_scheduler.ReduceLROnPlateau(
        optimizer, mode='min', factor=0.5, patience=3, threshold=0.0001, min_lr=1e-8
    )
    if resume_from and os.path.exists("last_chess_nnue_checkpoint.pth"):
        print(f"Resuming from checkpoint 'best_chess_nnue_checkpoint.pth'")
        model, optimizer, scheduler, start_epoch = \
            load_checkpoint("last_chess_nnue_checkpoint.pth", device=device)
    elif resume_from:
        print("Resume requested but no checkpoint found. Starting new training.")
    else:
        print("Starting new training (not resuming).")

    scaler = GradScaler() if device.type == 'cuda' else None
    
    for epoch in range(start_epoch, epochs):
        print(f"\nStarting epoch {epoch+1}/{epochs}...")
        base_dataset.new_epoch()
        print(f"Epoch {epoch+1} - Dataset length: {len(base_dataset)}")


        train_loader = DataLoader(base_dataset,batch_size=batch_size, shuffle=True, num_workers=4, pin_memory=True, drop_last=True, worker_init_fn=worker_init_fn)

        model.train()
        train_loss = 0
        samples_processed = 0
        train_progress = tqdm(train_loader, desc=f'Epoch {epoch+1}/{epochs} [Train]')

        for (stm_features_batch, nstm_features_batch, is_white_stm_batch), targets in train_progress:
            stm_features_batch = stm_features_batch.to(device)
            nstm_features_batch = nstm_features_batch.to(device)
            is_white_stm_batch = is_white_stm_batch.to(device)
            eval_targets_wdl = targets.to(device)

            optimizer.zero_grad(set_to_none=True) # More efficient


            model_eval_raw = model(stm_features_batch, nstm_features_batch, is_white_stm_batch)
            model_eval_wdl = torch.sigmoid(model_eval_raw / 410)
            loss = torch.pow(model_eval_wdl.squeeze() - eval_targets_wdl, exponent).mean()

            optimizer.zero_grad(set_to_none=True)
            loss.backward()
            torch.nn.utils.clip_grad_norm_(model.parameters(), max_norm=1.0)
            optimizer.step()


            train_loss += loss.item() * stm_features_batch.size(0)
            samples_processed += stm_features_batch.size(0)
            current_train_loss = train_loss / samples_processed
            train_progress.set_postfix({
                'Loss': f'{current_train_loss:.6f}', 'LR': f'{optimizer.param_groups[0]["lr"]:.6f}'
            })
            
            if np.isnan(current_train_loss):
                print("\nLoss is NaN. Halting training.")
                return model # Exit early

        train_loss /= samples_processed
        scheduler.step(train_loss)
        print(f"\nEpoch {epoch+1}/{epochs}:")
        print(f"  Train Loss: {train_loss:.6f} (RMSE WDL Approx: {np.sqrt(train_loss):.6f})") # Use sqrt for RMSE


        # Save checkpoint after scheduler step to capture latest LR if it changed
        save_checkpoint(model, optimizer, scheduler, epoch, "last_chess_nnue_checkpoint.pth")



    return model

def save_model(model, filepath):
    torch.save(model.state_dict(), filepath)
    print(f"Model saved to {filepath}")

def load_model(filepath, device='cpu'):
    model = NNUE(input_size=INPUT_SIZE, ft_size=HL_SIZE).to(device)
    model.load_state_dict(torch.load(filepath, map_location=device))
    model.eval()
    return model


def evaluate_position(model, fen):
    model.eval()
    device = next(model.parameters()).device # Get device from model parameters

    stm_char = fen.split()[1]
    is_white_stm = (stm_char == 'w') # True if white to move, False if black to move

    feat_white_perspective = fen_to_features(fen, perspective='white')
    feat_black_perspective = fen_to_features(fen, perspective='black')

    with torch.no_grad():

        white_features_tensor = torch.FloatTensor(feat_white_perspective).unsqueeze(0).to(device)
        black_features_tensor = torch.FloatTensor(feat_black_perspective).unsqueeze(0).to(device)
        
        stm_tensor = torch.tensor([1.0 if is_white_stm else 0.0], dtype=torch.float32).to(device)

        eval_out_raw = model(white_features_tensor, black_features_tensor, stm_tensor)

        eval_score_cp = eval_out_raw.item()

        if not is_white_stm: # If it was black to move, invert the score
            eval_score_cp = -eval_score_cp
            
        return int(eval_score_cp)


if __name__ == "__main__":
    DB_PATH = "eval.db"

    trained_model = train_nnue(DB_PATH, 
                                epochs=1000, 
                                batch_size=2048, 
                                lr=0.003, # Lowered learning rate
                                val_split=0.1, 
                                weight_decay=1e-8, 
                                exponent=2.0,
                                resume_from=False) # Using standard MSE

    save_model(trained_model, "final_chess_nnue.pth")

    print("\n--- Loading best model from training for evaluation ---")
    device = 'cuda' if torch.cuda.is_available() else 'cpu'
    
    # Check if the best model was saved before trying to load it
    if os.path.exists("best_chess_nnue.pth"):
        best_model = load_model("best_chess_nnue.pth", device=device)

        starting_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
        eval_cp = evaluate_position(best_model, starting_fen)
        print(f"Starting position evaluation: {eval_cp} centipawns")

        example_fen_black_to_move = "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1"
        eval_cp_black = evaluate_position(best_model, example_fen_black_to_move)
        print(f"e4 (black to move) evaluation: {eval_cp_black} centipawns")

        example_fen_white_advantage = "rnbqkbnr/pppppppp/8/8/8/5N2/PPPPPPPP/RNBQKB1R w KQkq - 1 2"
        eval_cp_white_adv = evaluate_position(best_model, example_fen_white_advantage)
        print(f"Nf3 (white to move) evaluation: {eval_cp_white_adv} centipawns")
    else:
        print("Training did not complete successfully, 'best_chess_nnue.pth' not found.")

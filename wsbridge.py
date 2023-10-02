import ctypes
import random
import sys
import json
import websocket

cmd = "gcc -shared -o pujolib.so -fopenmp -fPIC -Ofast -march=native main.c"

try:
    pujolib = ctypes.CDLL("./pujolib.so")
except OSError:
    sys.stderr.write("Library not found. Did you remember to run:\n")
    sys.stderr.write(cmd + "\n\n")
    raise()

WIDTH = 6
NUM_SLICES = WIDTH

Puyos = ctypes.c_short * NUM_SLICES

NUM_PUYO_TYPES = 6

color_t = ctypes.c_int

class SimpleScreen(ctypes.Structure):
    _fields_ = [
        ("grid", Puyos * NUM_PUYO_TYPES),
        ("buffered_garbage", ctypes.c_int)
    ]

COLOR_SELECTION_SIZE = 4

ColorSelection = color_t * COLOR_SELECTION_SIZE

class SimpleGame(ctypes.Structure):
    _fields_ = [
        ("screen", SimpleScreen),
        ("point_residue", ctypes.c_int),
        ("all_clear_bonus", ctypes.c_bool),
        ("pending_garbage", ctypes.c_int),
        ("late_garbage", ctypes.c_int),
        ("late_time_remaining", ctypes.c_float),
        ("move_time", ctypes.c_float),
        ("color_selection", ColorSelection)
    ]

BAG_SIZE = 6

class Bag(color_t * BAG_SIZE):
    def __init__(self):
        super().__init__(
            random.randint(0, 3),
            random.randint(0, 3),
            random.randint(0, 3),
            random.randint(0, 3),
            random.randint(0, 3),
            random.randint(0, 3)
        )

    def advance(self):
        self[0] = self[2]
        self[1] = self[3]

        self[2] = self[4]
        self[3] = self[5]

        self[4] = random.randint(0, 3)
        self[5] = random.randint(0, 3)

NUM_MOVES = WIDTH * 2 + (WIDTH - 1) * 2

game = SimpleGame()
g = ctypes.byref(game)
s = ctypes.byref(game.screen)
pujolib.clear_simple_game(g)

bag = Bag()

heuristic_score = ctypes.c_double()
h = ctypes.byref(heuristic_score)

# All possible locations and orientations right below the garbage buffer line.
MOVES = [
  # Orientation = 0
  {"x1": 0, "y1": 2, "x2": 0, "y2": 1, "orientation": 0},
  {"x1": 1, "y1": 2, "x2": 1, "y2": 1, "orientation": 0},
  {"x1": 2, "y1": 2, "x2": 2, "y2": 1, "orientation": 0},
  {"x1": 3, "y1": 2, "x2": 3, "y2": 1, "orientation": 0},
  {"x1": 4, "y1": 2, "x2": 4, "y2": 1, "orientation": 0},
  {"x1": 5, "y1": 2, "x2": 5, "y2": 1, "orientation": 0},
  # Orientation = 1
  {"x1": 1, "y1": 1, "x2": 0, "y2": 1, "orientation": 1},
  {"x1": 2, "y1": 1, "x2": 1, "y2": 1, "orientation": 1},
  {"x1": 3, "y1": 1, "x2": 2, "y2": 1, "orientation": 1},
  {"x1": 4, "y1": 1, "x2": 3, "y2": 1, "orientation": 1},
  {"x1": 5, "y1": 1, "x2": 4, "y2": 1, "orientation": 1},
  # Orientation = 2
  {"x1": 0, "y1": 1, "x2": 0, "y2": 2, "orientation": 2},
  {"x1": 1, "y1": 1, "x2": 1, "y2": 2, "orientation": 2},
  {"x1": 2, "y1": 1, "x2": 2, "y2": 2, "orientation": 2},
  {"x1": 3, "y1": 1, "x2": 3, "y2": 2, "orientation": 2},
  {"x1": 4, "y1": 1, "x2": 4, "y2": 2, "orientation": 2},
  {"x1": 5, "y1": 1, "x2": 5, "y2": 2, "orientation": 2},
  # Orientation = 3
  {"x1": 0, "y1": 1, "x2": 1, "y2": 1, "orientation": 3},
  {"x1": 1, "y1": 1, "x2": 2, "y2": 1, "orientation": 3},
  {"x1": 2, "y1": 1, "x2": 3, "y2": 1, "orientation": 3},
  {"x1": 3, "y1": 1, "x2": 4, "y2": 1, "orientation": 3},
  {"x1": 4, "y1": 1, "x2": 5, "y2": 1, "orientation": 3},
];

LOG = False

identity = None

wins = 0
draws = 0
losses = 0

def on_message(ws, message):
    global identity, wins, draws, losses
    if LOG:
        print("Message received", message)
    data = json.loads(message)
    if data["type"] == "identity":
        identity = data["player"]
    elif data["type"] == "bag" and data["player"] == identity:
        ws.send(json.dumps({"type": "simple state request"}))
    elif data["type"] == "simple state":
        state = data["state"]
        for j in range(NUM_PUYO_TYPES):
            for i in range(NUM_SLICES):
                game.screen.grid[j][i] = state["screen"]["grid"][j][i]
        game.screen.buffered_garbage = state["screen"]["bufferedGarbage"]
        game.point_residue = state["pointResidue"]
        game.all_clear_bonus = state["allClearBonus"]
        game.pending_garbage = state["pendingGarbage"]
        game.late_garbage = state["lateGarbage"]
        game.late_time_remaining = state["lateTimeRemaining"]
        game.move_time = state["moveTime"]
        for i in range(COLOR_SELECTION_SIZE):
            game.color_selection[i] = state["colorSelection"][i]
        for i in range(len(state["bag"])):
            bag[i] = state["bag"][i]

        move = pujolib.flexDropletStrategy3(g, bag, len(state["bag"]), h)

        pujolib.print_simple_game(g)
        print("Heuristic score:", heuristic_score.value)
        print("W/D/L:", "{}/{}/{}".format(wins, draws, losses))

        response = dict(MOVES[move])
        response["type"] = "move"
        response["hardDrop"] = True
        ws.send(json.dumps(response))
    elif data["type"] == "game result":
        if data["result"] == "win":
            wins += 1
        elif data["result"] == "draw":
            draws += 1
        else:
            losses += 1
        print("Game over:", data["result"], data["reason"])
        ws.send(json.dumps({"type": "game request"}))

def on_error(ws, error):
    print("Error", error)

def on_close(ws, close_status_code, close_msg):
    print("### closed ###")

def on_open(ws):
    print("Connection established.")
    ws.send(json.dumps({"type": "game request"}))

if __name__ == "__main__":
    # websocket.enableTrace(True)
    ws = websocket.WebSocketApp("ws://localhost:3003",
                              on_open=on_open,
                              on_message=on_message,
                              on_error=on_error,
                              on_close=on_close)

    ws.run_forever()

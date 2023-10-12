import argparse
import ctypes
import random
import sys
import json
import websocket
import subprocess

def get_git_revision_hash() -> str:
    return subprocess.check_output(['git', 'rev-parse', 'HEAD']).decode('ascii').strip()

def get_git_revision_short_hash() -> str:
    return subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD']).decode('ascii').strip()

try:
    libpujo = ctypes.CDLL("./build/lib/libpujo.so")
except OSError:
    sys.stderr.write("Library not found. Did you remember compile it?\n")
    raise()

BOTS = {
    "flex1": libpujo.flex_droplet_strategy_1,
    "flex2": libpujo.flex_droplet_strategy_2,
    "flex3": libpujo.flex_droplet_strategy_3,
    "flex4": libpujo.flex_droplet_strategy_4,
}

app_name = "pujobot"

Version = ctypes.c_char * 16
version_str = Version()
libpujo.version(version_str)
version = version_str.value.decode()

client_info = {
    "name": app_name,
    "version": version,
    "resolved": get_git_revision_short_hash(),
}

WIDTH = 6
NUM_SLICES = WIDTH

Puyos = ctypes.c_short * NUM_SLICES

NUM_PUYO_TYPES = 6

color_t = ctypes.c_int

move_t = ctypes.c_char

class JKISS32(ctypes.Structure):
    _fields_ = [
        ("x", ctypes.c_uint),
        ("y", ctypes.c_uint),
        ("z", ctypes.c_uint),
        ("c", ctypes.c_uint),
        ("w", ctypes.c_uint),
    ]

class SimpleScreen(ctypes.Structure):
    _fields_ = [
        ("grid", Puyos * NUM_PUYO_TYPES),
        ("buffered_garbage", ctypes.c_int),
        ("jkiss", JKISS32),
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
        ("color_selection", ColorSelection),
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

PASS = -1
INT_PASS = 255
NUM_MOVES = WIDTH * 2 + (WIDTH - 1) * 2 + 1

# Doesn't help. Still returns "unsigned" bytes.
# libpujo.flex_droplet_strategy_1.restype = ctypes.c_char
# libpujo.flex_droplet_strategy_2.restype = ctypes.c_char
# libpujo.flex_droplet_strategy_3.restype = ctypes.c_char
# libpujo.flex_droplet_strategy_4.restype = ctypes.c_char

game = SimpleGame()
g = ctypes.byref(game)
s = ctypes.byref(game.screen)
libpujo.clear_simple_game(g)

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

bot = None

identity = None

wins = 0
draws = 0
losses = 0

def request_game(ws):
    ws.send(json.dumps({
        "type": "game request",
        "name": "Pujobot/{}".format(bot.title()),
        "clientInfo": client_info
    }))

def on_message(ws, message):
    global identity, wins, draws, losses
    if LOG:
        print("Message received", message)
    data = json.loads(message)
    if data["type"] == "game params":
        identity = data["identity"]
    elif data["type"] == "bag" and data["player"] == identity:
        ws.send(json.dumps({"type": "simple state request"}))
    elif data["type"] == "simple state":
        state = data["state"]
        for j in range(NUM_PUYO_TYPES):
            for i in range(NUM_SLICES):
                game.screen.grid[j][i] = state["screen"]["grid"][j][i]
        game.screen.buffered_garbage = state["screen"]["bufferedGarbage"]

        game.screen.jkiss.x = state["screen"]["jkiss"][0]
        game.screen.jkiss.y = state["screen"]["jkiss"][1]
        game.screen.jkiss.z = state["screen"]["jkiss"][2]
        game.screen.jkiss.c = state["screen"]["jkiss"][3]
        game.screen.jkiss.w = state["screen"]["jkiss"][4]

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

        move = BOTS[bot](g, bag, len(state["bag"]), h)

        libpujo.print_simple_game(g)
        print("Move:", move)
        print("Heuristic score:", heuristic_score.value)
        print("W/D/L:", "{}/{}/{}".format(wins, draws, losses))


        if move == PASS or move == INT_PASS:
            response = {"pass": True}
        else:
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
        request_game(ws)

def on_error(ws, error):
    print("Error", error)

def on_close(ws, close_status_code, close_msg):
    print("### closed ###")

def on_open(ws):
    print("Connection established.")
    request_game(ws)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog='Pujobot',
        description='AI routines for playing Pujo Puyo',
        epilog='This is a websocket bridge linking C with the bun/TypeScript server')
    parser.add_argument("bot", nargs="?", default="flex3", choices=BOTS.keys(), help='Strategy to use')
    args = parser.parse_args()

    bot = args.bot

    ws = websocket.WebSocketApp("ws://localhost:3003",
                              on_open=on_open,
                              on_message=on_message,
                              on_error=on_error,
                              on_close=on_close)

    ws.run_forever()

import ctypes
import random
import sys

try:
    pujolib = ctypes.CDLL("pujolib.so")
except OSError:
    sys.stderr.write("Library not found. Did you remember to run:\ngcc -shared -o pujolib.so -fPIC -Ofast -march=native main.c\n\n")
    raise()

WIDTH = 6
NUM_SLICES = WIDTH

Puyos = ctypes.c_short * NUM_SLICES

NUM_PUYO_TYPES = 6

color_t = ctypes.c_int

class SimpleScreen(ctypes.Structure):
    _fields_ = [
        ("grid", Puyos * NUM_PUYO_TYPES),
        ("garbage_slots", ctypes.c_size_t * WIDTH),
        ("garbage_index", ctypes.c_size_t),
        ("buffered_garbage", ctypes.c_int)
    ]

COLOR_SELECTION_SIZE = 4

ColorSelection = color_t * COLOR_SELECTION_SIZE

class SimpleGame(ctypes.Structure):
    _fields_ = [
        ("screen", SimpleScreen),
        ("pending_garbage", ctypes.c_int),
        ("late_time_remaining", ctypes.c_float),
        ("move_time", ctypes.c_float),
        ("color_selection", ColorSelection)
    ]

class Bag(color_t * 6):
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
score = 0

for _ in range(100):
    move = pujolib.maxDropletStrategy2(g, bag, h)
    pujolib.play_simple(g, bag, move)
    bag.advance()
    score += pujolib.resolve_simple(g)
    pujolib.print_screen(s)
    print("Score:", score)

# TODO: Bridge to a pujo server with websocket

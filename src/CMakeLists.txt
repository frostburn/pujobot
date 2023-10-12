ADD_LIBRARY(
  pujobot
  util.c
  bitboard.c
  screen.c
  game.c
  ai.c
)
INSTALL(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../include/pujobot
  DESTINATION include
  FILES_MATCHING PATTERN "*.h"
)
INSTALL(
  TARGETS pujobot
  ARCHIVE
  DESTINATION lib
)

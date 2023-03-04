# Captain chess engine

Captain is a UCI compliant chess engine written in C++. It can work with any chess GUI that supports UCI, although it has only been tested with the [Banksia chess GUI](https://banksiagui.com/).

I have learned a lot from studying other chess engines and projects, including:

* [Stockfish](https://github.com/official-stockfish/Stockfish)
* [Arasan](https://github.com/jdart1/arasan-chess)
* [Nemorino](https://bitbucket.org/christian_g_nther/nemorino/src/master/)
* [Gigantua](https://github.com/Gigantua/Gigantua)
* [The chess programming wiki](https://www.chessprogramming.org/Main_Page)

## Features

### Board
* [Quad bitboards](https://www.chessprogramming.org/Quad-Bitboards)
* [Hyperbola Quintessence](https://www.chessprogramming.org/Hyperbola_Quintessence) and [Kogge-Stone](https://www.chessprogramming.org/Kogge-Stone_Algorithm) for move generation
* [Copy-make](https://www.chessprogramming.org/Copy-Make)
### Search
* [Principal variation search](https://www.chessprogramming.org/Principal_Variation_Search)
* [Quiescence search](https://www.chessprogramming.org/Quiescence_Search)
  * [Delta pruning](https://www.chessprogramming.org/Delta_Pruning)
  * Prune losing captures
* [Iterative deepening](https://www.chessprogramming.org/Iterative_Deepening)
* [Transposition table](https://www.chessprogramming.org/Transposition_Table)
* [Check extensions](https://www.chessprogramming.org/Check_Extensions)
* [Null move pruning](https://www.chessprogramming.org/Null_Move_Pruning)
* [Late move reductions](https://www.chessprogramming.org/Late_Move_Reductions)
* Move ordering
  * Transposition table move
  * Winning/even captures
  * [Killer moves](https://www.chessprogramming.org/Killer_Heuristic)
  * Quiet moves sorted by [history heuristic](https://www.chessprogramming.org/History_Heuristic)
  * Losing captures
### Evaluation
* [Piece square tables](https://www.chessprogramming.org/Piece-Square_Tables)
* [Static exchange evaluation](https://www.chessprogramming.org/Static_Exchange_Evaluation) for captures
* Bonuses
  * Knight outpost
  * Bishop pair
  * Rook on open file
  * Bishop on open diagonal
  * Proximity to enemy king

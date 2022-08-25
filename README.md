# Chess_engine

A console chess game written in C++. I tried out modules (a C++20 feature) for this project, so compiler support could be lacking. You can play a game against the chess engine in the console, by entering moves in 3 different ways:

1. By entering an origin and destination square (such as e2e4). The first two characters denote the "from" square and the last two denote the "to" square.
2. Typing K/k/Q/q for castling. K or Q denotes kingside or queenside castling, and uppercase denotes white pieces while lowercase denotes black pieces.
3. When promoting a pawn, type the move just as in case (1), but add a letter to denote the promotion piece. For example, h7h8Q to promote to a queen. Upper and lowercase don't matter for the promotion piece.

Example:
![chess_engine_demo](https://user-images.githubusercontent.com/4705982/186613452-73754c3f-fd94-4031-8eff-910904ee46de.png)

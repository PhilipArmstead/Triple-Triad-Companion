Triple Triad Solver
===

This is a companion app for the
[Steam release of Final Fantasy VIII](https://store.steampowered.com/app/39150/FINAL_FANTASY_VIII/).
It will analyse active Triple Triad games and instruct you on what moves to play
to guarantee the best possible outcome.

> Note: Windows is the only supported platform currently.

# What does it do?

The app waits until it's your turn to play before suggesting the move which will
lead to the optimal outcome for you. The optimal outcome is determined by which
game-state ends with the most cards for you.

# What doesn't it do?

- Guarantee a win. Not every hand is winnable (although the opponent can
	blunder, so your outcome could improve over the course of the game.)
- Protect valuable cards. This algorithm goes for the biggest win, which may not
	necessarily be what you want if you're playing with the **Direct** trading
	rule.
- Consider elements, the **Same** or the **Wall** rules, yet.

# How to use?

Download and run the app while Final Fantasy VIII is running. Begin playing
cards and the app will kick in.

![Screenshot of CLI running](assets/solver-cli.png?v=1)

# TODO

- Special rules
	- Same
	- Same Wall
	- Plus
	- Elements


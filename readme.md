
SUBSYSTEM:WINDOWS means that winMain is used. different main. subsystem:console means that it is still attached to the parent console (at least, for us, using SDL.)
SUBSYSTEM:CONSOLE therefore should be good enough to use.


# TODO
	- handle wall collisions. can that happen in slide_move?
	- crouching?
	- better bunny hopping?
	- rocket knockback?


# DONE
	- grid of 10 x 10 units on the y = 0 plane. 
	- we still induce jump too many times.
	- wrap the current move into one that splits into air move and regular move. this is getting convoluted.
	- remove the grounded state from the player_move function.
	- how to deal with planes that are not ground planes? how do we even test that? check against a normal? (like mario!).

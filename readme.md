
SUBSYSTEM:WINDOWS means that winMain is used. different main. subsystem:console means that it is still attached to the parent console (at least, for us, using SDL.)
SUBSYSTEM:CONSOLE therefore should be good enough to use.


# TODO
	- closest face coloring (white for floor, other for z. also assert that the different axes are not talking about the same triangles.)
	- check the wish_direction in ground_move and air move. for some reason, we now stick to the wall when we hit a wall.
	- not that slide_move also does step_slide_move, where they evaluate all collision planes and "stack" all movements across all planes. this is a sort of incremental move.





	- crouching?
	- better bunny hopping?
	- rocket knockback?


# DONE
	- grid of 10 x 10 units on the y = 0 plane. 
	- we still induce jump too many times.
	- wrap the current move into one that splits into air move and regular move. this is getting convoluted.
	- remove the grounded state from the player_move function.
	- how to deal with planes that are not ground planes? how do we even test that? check against a normal? (like mario!).

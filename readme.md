
SUBSYSTEM:WINDOWS means that winMain is used. different main. subsystem:console means that it is still attached to the parent console (at least, for us, using SDL.)
SUBSYSTEM:CONSOLE therefore should be good enough to use.

# TODO
	- both server and client side, we should split the movement to quarter intervals to prevent tunneling (both quake and sm64 do this, it should be good enough for us :~)). when connecting to the server, because the server runs on a lower framerate,
	there is a higher chance of tunneling.
	- parallax shader.
	- think about the reconstruct thing. is it better to first have the packets in an array until we have all of them, and only then reconstruct them? -> yes.
	- async printing and dispatching in the server.




	- generate vs create vs ..... pick a better name.
	- input scale when not moving is bigger if we only jump. that seems like an issue.
	- clean up sdl_test.cc w.r.t. noclip stuff. the visualization stuff is useful, and we maybe also want to do bsp traversal when noclipping, but not the collision part.
	- what to do if we have multiple ground traces?
	- not that slide_move also does step_slide_move, where they evaluate all collision planes and "stack" all movements across all planes. this is a sort of incremental move.
	- crouching?
	- better bunny hopping?
	- rocket knockback?


# DONE
	- get the font file to compile? how to specify a c compiler in meson? this is getting annoying. -> rename to .cc and deal with the fallout. easier than convincing meson
	to do what I wanted.
	- toggling wireframe would be nice. -> you can pass a bool.

	- there is still slight collision issues: seam walking for example. special case that later. (you can barely walk along the edge if you hold the button in that direction at the bottom of an aabb which is not connected to any other.)
	- we should not do this filtering in the bsp.hpp, but somewhere else? or is it too late then?
	- remove the special casing in the player_move.cc and formalize it as a parameter. (bring back the traces)?
	- update all the special casing in sdl_test.cc for ground planes etc
	- separating the collision planes in a ground_plane and "everything else". this is so we can do rejection if we are colliding with something that is "below the floor". we can introduce a height tolerance.
	- check the wish_direction in ground_move and air move. for some reason, we now stick to the wall when we hit a wall. (after the new collision planes). (do not collide with walls that you are moving away from.)
	- closest face coloring (white for floor, other for z. also assert that the different axes are not talking about the same triangles.)
	- finish (bsp_collide_with_AABB)
	- grid of 10 x 10 units on the y = 0 plane. 
	- we still induce jump too many times.
	- wrap the current move into one that splits into air move and regular move. this is getting convoluted.
	- remove the grounded state from the player_move function.
	- how to deal with planes that are not ground planes? how do we even test that? check against a normal? (like mario!).


SUBSYSTEM:WINDOWS means that winMain is used. different main. subsystem:console means that it is still attached to the parent console (at least, for us, using SDL.)
SUBSYSTEM:CONSOLE therefore should be good enough to use.




# TODO
 - fix the naming conventions for ecs.hpp
 - do logging on a different thread
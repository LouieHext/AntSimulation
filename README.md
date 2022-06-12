# Ant Simulation Compute Shader

This is a pheromone based ant simulation. Ants will leave their nest in search of food, once they find the food they will try to return to the nest. This cycle will repeat. Ants leave trails of pheromones to guide each other to and from the food, these pheromones decay and so naturally optimise for time taken between the targets - the ants have emergent path finding abilities.

It is made using compute shaders and openframeworks, to run compute shaders you need openGl 4.3 which sadly is not supported by MacOs.


# To play
pressing "r" restarts the current map
pressing "g" generates a new map (note there is currently no check for if ants spawn inside a wall)

# To Setup
Clone the repo
set up a new openframeworks project with ofxGui
copy the code from the project into the clone, or visa versa
open up the project file 
run

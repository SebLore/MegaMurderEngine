#include "Game.h"

#include "Systems/Systems.h"

void Game::Configure(ECSManager& ecs)
{
    // Add all initialized systems
    ecs.EmplaceSystem<LoadSystem>();
    ecs.Update(); // load system emplaces initilization systems
}

// todo: implement loading from file
void Game::Load(ECSManager& ecs) {}

// Justin Mealey - Project 1 - 005961729

#include "Rabbit.h"
#include "Player.h"
#include "Arena.h"
#include "globals.h"
#include "Game.h"
#include "History.h"

#include <iostream>
#include <string>
#include <random>
#include <utility>
#include <cstdlib>
#include <cctype>

using namespace std;

int main()
{
    Game g(10, 10, 5);
    g.play();
    
}
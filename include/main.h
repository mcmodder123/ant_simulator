#ifndef MAIN_H
#define MAIN_H

#include <SDL2/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

// constants
const int SCREEN_WIDTH = 1240;
const int SCREEN_HEIGHT = 720;
const int ANT_SIZE = 5;
const int ANT_VELOCITY = 3;
const int NUM_ANTS = 100;
const int GRID_SCALE = 8;
const int GRID_WIDTH = SCREEN_WIDTH / GRID_SCALE;
const int GRID_HEIGHT = SCREEN_HEIGHT / GRID_SCALE;

// home
const long double HOME_LEFT = 20.0L;
const long double HOME_TOP = 20.0L;
long double HOME_WIDTH = 100.0L;                  // these used to be
long double HOME_HEIGHT = 100.0L;                 // constants but were
long double HOME_RIGHT = HOME_LEFT + HOME_WIDTH;  // changed because of
long double HOME_BOTTOM = HOME_TOP + HOME_HEIGHT; // the growing colony
                                                  // size functionality

// food
const long double FOOD_LEFT = SCREEN_WIDTH - 250.0L;
const long double FOOD_TOP = SCREEN_HEIGHT / 2.0L - 200.0L;
const long double FOOD_WIDTH = 200.0L;
const long double FOOD_HEIGHT = 400.0L;
const long double FOOD_RIGHT = FOOD_LEFT + FOOD_WIDTH;
const long double FOOD_BOTTOM = FOOD_TOP + FOOD_HEIGHT;

const double PHEROMONE_DROP_AMOUNT = 15.0;
const double PHEROMONE_MAX = 100.0;
const double PHEROMONE_EVAPORATION_RATE = 0.004;
const double SENSOR_DISTANCE = 10.0;
const double SENSOR_ANGLE = M_PI / 6.0;
const double STEERING_STRENGTH = M_PI / 12.0;
const double WANDER_STRENGTH = M_PI / 32.0;

const int INITIAL_FOOD_HEALTH = 1;

extern std::vector<std::vector<double>> foodPheromoneGrid;
extern std::vector<std::vector<double>> homePheromoneGrid;
extern std::vector<std::vector<int>> foodLevelGrid;

namespace Ant {

// ant type
class ant {
public:
  // set up ant
  ant(int id) : antID(id), hasFood(false) {
    x = HOME_LEFT + (rand() % (int)HOME_WIDTH);
    y = HOME_TOP + (rand() % (int)HOME_HEIGHT);
    setRandomDirection();
  }

  int antID;
  long double x;
  long double y;
  double directionRadians;
  bool hasFood;

  void setRandomDirection() {
    directionRadians = (rand() / (double)RAND_MAX) * 2.0 * M_PI;
  }

  // read pheromone level
  double sensePheromoneAt(double sensorX, double sensorY,
                          const std::vector<std::vector<double>> &grid) {
    int gridX = static_cast<int>(sensorX / GRID_SCALE);
    int gridY = static_cast<int>(sensorY / GRID_SCALE);

    if (gridX < 0 || gridX >= GRID_WIDTH || gridY < 0 || gridY >= GRID_HEIGHT) {
      return 0.0;
    }

    if (gridY < grid.size() && gridX < grid[gridY].size()) {
      return grid[gridY][gridX];
    }
    return 0.0;
  }

  // make ant go towards pheromones
  void
  steerTowardsPheromones(const std::vector<std::vector<double>> &targetGrid) {
    double forwardX = x + SENSOR_DISTANCE * cos(directionRadians);
    double forwardY = y + SENSOR_DISTANCE * sin(directionRadians);
    double leftAngle = directionRadians - SENSOR_ANGLE;
    double leftX = x + SENSOR_DISTANCE * cos(leftAngle);
    double leftY = y + SENSOR_DISTANCE * sin(leftAngle);
    double rightAngle = directionRadians + SENSOR_ANGLE;
    double rightX = x + SENSOR_DISTANCE * cos(rightAngle);
    double rightY = y + SENSOR_DISTANCE * sin(rightAngle);

    double pheromoneForward = sensePheromoneAt(forwardX, forwardY, targetGrid);
    double pheromoneLeft = sensePheromoneAt(leftX, leftY, targetGrid);
    double pheromoneRight = sensePheromoneAt(rightX, rightY, targetGrid);

    bool steered = false;
    if (pheromoneForward > 0.1 || pheromoneLeft > 0.1 || pheromoneRight > 0.1) {
      if (pheromoneForward >= pheromoneLeft &&
          pheromoneForward >= pheromoneRight) {
        steered = true;
      } else if (pheromoneRight > pheromoneLeft) {
        directionRadians += STEERING_STRENGTH;
        steered = true;
      } else {
        directionRadians -= STEERING_STRENGTH;
        steered = true;
      }
    }

    // add a little randomness
    double currentWander = steered ? WANDER_STRENGTH * 0.5 : WANDER_STRENGTH;
    directionRadians +=
        ((rand() / (double)RAND_MAX) - 0.5) * 2.0 * currentWander;

    while (directionRadians >= 2 * M_PI)
      directionRadians -= 2 * M_PI;
    while (directionRadians < 0)
      directionRadians += 2 * M_PI;
  }

  // update ant
  void updateAnt() {
    // steer
    if (hasFood) {
      steerTowardsPheromones(homePheromoneGrid);
    } else {
      steerTowardsPheromones(foodPheromoneGrid);
    }

    // calculate position and update
    long double antVelocityX =
        ANT_VELOCITY * cos(directionRadians); // Use const
    long double antVelocityY =
        ANT_VELOCITY * sin(directionRadians); // Use const

    x += antVelocityX;
    y += antVelocityY;

    // bounds checks
    bool bounced = false;
    if (x < 0) {
      x = 0;
      directionRadians = M_PI - directionRadians;
      bounced = true;
    } else if (x >= SCREEN_WIDTH) {
      x = SCREEN_WIDTH - 1;
      directionRadians = M_PI - directionRadians;
      bounced = true;
    }
    if (y < 0) {
      y = 0;
      directionRadians = (2 * M_PI) - directionRadians;
      bounced = true;
    } else if (y >= SCREEN_HEIGHT) {
      y = SCREEN_HEIGHT - 1;
      directionRadians = (2 * M_PI) - directionRadians;
      bounced = true;
    }

    if (bounced) {
      while (directionRadians >= 2 * M_PI)
        directionRadians -= 2 * M_PI;
      while (directionRadians < 0)
        directionRadians += 2 * M_PI;
    }

    // drop pheromones
    int gridX = static_cast<int>(x / GRID_SCALE);
    int gridY = static_cast<int>(y / GRID_SCALE);

    if (gridX >= 0 && gridX < GRID_WIDTH && gridY >= 0 && gridY < GRID_HEIGHT) {
      if (hasFood && gridY < foodPheromoneGrid.size() &&
          gridX < foodPheromoneGrid[gridY].size()) {
        foodPheromoneGrid[gridY][gridX] += PHEROMONE_DROP_AMOUNT;
        foodPheromoneGrid[gridY][gridX] =
            std::min(foodPheromoneGrid[gridY][gridX], PHEROMONE_MAX);
      } else if (!hasFood && gridY < homePheromoneGrid.size() &&
                 gridX < homePheromoneGrid[gridY].size()) {
        homePheromoneGrid[gridY][gridX] += PHEROMONE_DROP_AMOUNT;
        homePheromoneGrid[gridY][gridX] =
            std::min(homePheromoneGrid[gridY][gridX], PHEROMONE_MAX);
      }
    }
  }

  // what to do when ant reaches food
  void reachedFood() {
    hasFood = true;
    directionRadians += M_PI;
    directionRadians += ((rand() / (double)RAND_MAX) - 0.5) * (M_PI / 4.0);
    while (directionRadians >= 2 * M_PI)
      directionRadians -= 2 * M_PI;
    while (directionRadians < 0)
      directionRadians += 2 * M_PI;
    std::cout << "food acquired by ant: " << antID << '\n';
  }

  // what to do when ant reaches home/colony
  void reachedHome() {
    hasFood = false;
    directionRadians += M_PI;
    directionRadians += ((rand() / (double)RAND_MAX) - 0.5) * (M_PI / 4.0);
    while (directionRadians >= 2 * M_PI)
      directionRadians -= 2 * M_PI;
    while (directionRadians < 0)
      directionRadians += 2 * M_PI;
    if (HOME_BOTTOM < SCREEN_HEIGHT - 20 && HOME_RIGHT < SCREEN_WIDTH) {
      HOME_WIDTH += 0.5;
      HOME_HEIGHT += 0.5;
      HOME_RIGHT += 0.5;
      HOME_BOTTOM += 0.5;
    } else if (HOME_RIGHT <
               SCREEN_WIDTH - (SCREEN_WIDTH - FOOD_RIGHT) - FOOD_WIDTH - 10) {
      HOME_RIGHT++;
      HOME_WIDTH++;
    }
    std::cout << "food brought home by ant: " << antID << '\n';
  }
}; // end class ant

} // end namespace Ant

namespace Food {

bool isInFoodRegion(Ant::ant &ant) {
  return (ant.x >= FOOD_LEFT && ant.x <= FOOD_RIGHT && ant.y >= FOOD_TOP &&
          ant.y <= FOOD_BOTTOM);
}
} // end namespace Food

namespace Home {
bool isInsideHome(Ant::ant &ant) {
  return (ant.x >= HOME_LEFT && ant.x <= HOME_RIGHT && ant.y >= HOME_TOP &&
          ant.y <= HOME_BOTTOM);
}
} // end namespace Home

#endif // end main.h

#include "include/main.h"

#include <SDL2/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <vector>

std::vector<std::vector<double>> foodPheromoneGrid;
std::vector<std::vector<double>> homePheromoneGrid;
std::vector<std::vector<int>> foodLevelGrid;

int main(int argc, char *argv[]) {

  srand(time(0));

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
    std::cerr << "SDL Init Error: " << SDL_GetError() << std::endl;
    return 1;
  }

  SDL_Window *window = SDL_CreateWindow("Ant Simulator", SDL_WINDOWPOS_CENTERED,
                                        SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH,
                                        SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
  if (!window) {
    std::cerr << "Window Creation Error: " << SDL_GetError() << std::endl;
    SDL_Quit();
    return 1;
  }

  // Create SDL Renderer
  SDL_Renderer *renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer) {
    std::cerr << "Renderer Creation Error: " << SDL_GetError() << std::endl;
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  foodPheromoneGrid.resize(GRID_HEIGHT);
  homePheromoneGrid.resize(GRID_HEIGHT);
  foodLevelGrid.resize(GRID_HEIGHT);
  for (int y = 0; y < GRID_HEIGHT; ++y) {
    foodPheromoneGrid[y].resize(GRID_WIDTH, 0.0);
    homePheromoneGrid[y].resize(GRID_WIDTH, 0.0);
    foodLevelGrid[y].resize(GRID_WIDTH, 0);

    for (int x = 0; x < GRID_WIDTH; ++x) {
      double cellWorldX = (x + 0.5) * GRID_SCALE;
      double cellWorldY = (y + 0.5) * GRID_SCALE;
      if (cellWorldX >= FOOD_LEFT && cellWorldX <= FOOD_RIGHT &&
          cellWorldY >= FOOD_TOP && cellWorldY <= FOOD_BOTTOM) {
        foodLevelGrid[y][x] = INITIAL_FOOD_HEALTH;
      }
    }
  }

  // create ants
  std::vector<Ant::ant> ants;
  std::cout << "Creating " << NUM_ANTS << " ants..." << std::endl;
  ants.reserve(NUM_ANTS);
  for (int i = 0; i < NUM_ANTS; ++i)
    ants.emplace_back(i);

  bool running = true;
  SDL_Event event;

  while (running) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = false;
      }
      if (event.type == SDL_WINDOWEVENT &&
          event.window.event == SDL_WINDOWEVENT_CLOSE &&
          event.window.windowID == SDL_GetWindowID(window)) {
        running = false;
      }
    }

    double evaporationMultiplier = 1.0 - PHEROMONE_EVAPORATION_RATE;
    for (int y = 0; y < GRID_HEIGHT; ++y) {
      for (int x = 0; x < GRID_WIDTH; ++x) {
        if (foodPheromoneGrid[y][x] > 0.1) {
          foodPheromoneGrid[y][x] *= evaporationMultiplier;
          if (foodPheromoneGrid[y][x] < 0.1)
            foodPheromoneGrid[y][x] = 0.0;
        }
        if (homePheromoneGrid[y][x] > 0.1) {
          homePheromoneGrid[y][x] *= evaporationMultiplier;
          if (homePheromoneGrid[y][x] < 0.1)
            homePheromoneGrid[y][x] = 0.0;
        }
      }
    }

    for (Ant::ant &currentAnt : ants) {
      if (!currentAnt.hasFood) {
        int gridX = static_cast<int>(currentAnt.x / GRID_SCALE);
        int gridY = static_cast<int>(currentAnt.y / GRID_SCALE);
        if (gridX >= 0 && gridX < GRID_WIDTH && gridY >= 0 &&
            gridY < GRID_HEIGHT) {
          if (gridY < foodLevelGrid.size() &&
              gridX < foodLevelGrid[gridY].size() &&
              foodLevelGrid[gridY][gridX] > 0) {
            currentAnt.reachedFood();
            foodLevelGrid[gridY][gridX]--;
          }
        }
      }

      else {
        if (Home::isInsideHome(currentAnt)) {
          currentAnt.reachedHome();
        }
      }
      currentAnt.updateAnt();
    }

    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_RenderClear(renderer);

    // render pheromones
    bool renderPheromones = true;
    if (renderPheromones) {
      SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
      for (int y = 0; y < GRID_HEIGHT; ++y) {
        for (int x = 0; x < GRID_WIDTH; ++x) {
          bool drewFoodPhero = false;
          if (foodPheromoneGrid[y][x] > 0.1) {
            Uint8 alpha = static_cast<Uint8>(std::min(
                200.0, (foodPheromoneGrid[y][x] / PHEROMONE_MAX) * 200.0));
            SDL_SetRenderDrawColor(renderer, 160, 32, 240, alpha);
            SDL_Rect pRect = {x * GRID_SCALE, y * GRID_SCALE, GRID_SCALE,
                              GRID_SCALE};
            SDL_RenderFillRect(renderer, &pRect);
            drewFoodPhero = true;
          }

          if (homePheromoneGrid[y][x] > 0.1) {
            Uint8 alpha = static_cast<Uint8>(std::min(
                200.0, (homePheromoneGrid[y][x] / PHEROMONE_MAX) * 200.0));
            SDL_SetRenderDrawColor(renderer, 0, 200, 220,
                                   drewFoodPhero ? (Uint8)(alpha * 0.7 + 50)
                                                 : alpha);
            SDL_Rect pRect = {x * GRID_SCALE, y * GRID_SCALE, GRID_SCALE,
                              GRID_SCALE};
            SDL_RenderFillRect(renderer, &pRect);
          }
        }
      }
      SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }

    // render home/colony
    SDL_SetRenderDrawColor(renderer, 173, 216, 230, 255);
    SDL_Rect homeRect = {(int)HOME_LEFT, (int)HOME_TOP, (int)HOME_WIDTH,
                         (int)HOME_HEIGHT};
    SDL_RenderFillRect(renderer, &homeRect);

    // render food cells
    for (int y = 0; y < GRID_HEIGHT; ++y) {
      for (int x = 0; x < GRID_WIDTH; ++x) {
        if (foodLevelGrid[y][x] > 0) {
          int intensity =
              100 + (int)(((double)foodLevelGrid[y][x] / INITIAL_FOOD_HEALTH) *
                          155.0);
          SDL_SetRenderDrawColor(renderer, 0, intensity, 0, 255);
          SDL_Rect foodCellRect = {x * GRID_SCALE, y * GRID_SCALE, GRID_SCALE,
                                   GRID_SCALE}; // Use const
          SDL_RenderFillRect(renderer, &foodCellRect);
        }
      }
    }

    // render ants
    for (const Ant::ant &currentAnt : ants) {
      if (currentAnt.hasFood) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
      } else {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
      }

      SDL_Rect antRect = {(int)(currentAnt.x - ANT_SIZE / 2.0), // Use const
                          (int)(currentAnt.y - ANT_SIZE / 2.0), // Use const
                          ANT_SIZE, ANT_SIZE};                  // Use const
      SDL_RenderFillRect(renderer, &antRect);
    }

    SDL_RenderPresent(renderer);
  }

  // end program
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}

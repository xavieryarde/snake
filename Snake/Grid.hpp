#ifndef GRID_HPP
#define GRID_HPP

#include <SDL2/SDL.h>

class Grid {
public:
    // Initialize grid properties
    Grid(int screenWidth, int screenHeight, int cellSize);

    // Draw the grid
    void draw(SDL_Renderer* renderer, int offsetY) const;

    // Draw only outside lines
    void drawBoundary(SDL_Renderer* renderer, int offsetY) const;

    // Getters for cell size and grid width/height in cells
    int getCellSize() const { return mCellSize; }
    int getGridWidth() const { return mGridWidth; }
    int getGridHeight() const { return mGridHeight; }

    // Snap a position to the nearest grid cell
    SDL_Point snapToGrid(int x, int y) const;

private:
    int mScreenWidth;
    int mScreenHeight;
    int mCellSize;
    int mGridWidth;
    int mGridHeight;
};

#endif

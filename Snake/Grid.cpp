#include "Grid.hpp"


Grid::Grid(int screenWidth, int screenHeight, int cellSize)
	: mScreenWidth(screenWidth)
	, mScreenHeight(screenHeight)
	, mCellSize(cellSize)
{
	mGridWidth = screenWidth / cellSize;
	mGridHeight = screenHeight / cellSize;
}

// Draw the grid lines on the renderer
void Grid::draw(SDL_Renderer* renderer, int offsetY) const {
    // Define two colors for the checkered pattern
    SDL_Color lightColor = { 75, 105, 47, SDL_ALPHA_OPAQUE }; // Light gray
    SDL_Color darkColor = { 34, 47, 23, SDL_ALPHA_OPAQUE };  // Dark gray

    // Loop through the rows and columns of the grid
    for (int y = 0; y < mScreenHeight; y += mCellSize) {
        for (int x = 0; x < mScreenWidth; x += mCellSize) {

            // Check if the current cell is in an "even" or "odd" position for checkered pattern
            bool isDark = ((x / mCellSize + y / mCellSize) % 2 == 0);

            // Set color based on the checkered pattern
            SDL_SetRenderDrawColor(renderer, isDark ? darkColor.r : lightColor.r,
                isDark ? darkColor.g : lightColor.g,
                isDark ? darkColor.b : lightColor.b,
                SDL_ALPHA_OPAQUE);

            // Draw the filled rectangle for each cell in the grid
            SDL_Rect cellRect = { x, y + offsetY, mCellSize, mCellSize };
            SDL_RenderFillRect(renderer, &cellRect);
        }
    }

    // Optionally, draw the grid border or outer lines
    SDL_SetRenderDrawColor(renderer, 34, 47, 23, SDL_ALPHA_OPAQUE);  // Dark gray border
    //SDL_RenderDrawLine(renderer, 0, offsetY, 0, mScreenHeight + offsetY); // Left edge
    //SDL_RenderDrawLine(renderer, mScreenWidth, offsetY, mScreenWidth, mScreenHeight + offsetY); // Right edge
    SDL_RenderDrawLine(renderer, 0, offsetY, mScreenWidth, offsetY); // Top edge
    //SDL_RenderDrawLine(renderer, 0, mScreenHeight + offsetY, mScreenWidth, mScreenHeight + offsetY); // Bottom edge
}


void Grid::drawBoundary(SDL_Renderer* renderer, int offsetY) const {
	SDL_SetRenderDrawColor(renderer, 211, 211, 211, SDL_ALPHA_OPAQUE); // Light gray color for border

	// Draw the vertical boundary lines (left and right edges) with "thickness"
	for (int i = -2; i <= 2; ++i) {  // Adjust number of iterations to control thickness
		SDL_RenderDrawLine(renderer, 0, offsetY + i, 0, mScreenHeight + offsetY + i); // Left edge
		SDL_RenderDrawLine(renderer, mScreenWidth, offsetY + i, mScreenWidth, mScreenHeight + offsetY + i); // Right edge
	}

	// Draw the horizontal boundary lines (top and bottom edges) with "thickness"
	for (int i = 0; i <= 4; ++i) {  // Adjust number of iterations to control thickness
		SDL_RenderDrawLine(renderer, 0, offsetY + i, mScreenWidth, offsetY + i); // Top edge
		SDL_RenderDrawLine(renderer, 0, mScreenHeight + offsetY + i, mScreenWidth, mScreenHeight + offsetY + i); // Bottom edge
	}

}

// Snap a position to the nearest cell on the grid
SDL_Point Grid::snapToGrid(int x, int y) const {
	SDL_Point snappedPosition;
	snappedPosition.x = (x / mCellSize) * mCellSize;
	snappedPosition.y = (y / mCellSize) * mCellSize;
	return snappedPosition;
}

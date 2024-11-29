#ifndef GAME_HPP
#define GAME_HPP

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include "Grid.hpp"
#include <vector>

#define SCREEN_WIDTH    950
#define SCREEN_HEIGHT   600
#define WINDOW_WIDTH    SCREEN_WIDTH
#define WINDOW_HEIGHT   SCREEN_HEIGHT + 50
#define CELL_SIZE       50

class Game {
private:
    bool isRunning;
    bool isGameOver;
    bool mIsMovingUp, mIsMovingDown, mIsMovingLeft, mIsMovingRight;
    int mDirectionX;
    int mDirectionY;
    int currentDirectionX;
    int currentDirectionY;
    int score;
    static const float PlayerSpeed;
    static const int FPS;
    static const int maxSnakeSize;
    static const int speedIncreaseThreshold;
    static Uint32 TimePerFrame;
    SDL_Window* mWindow;
    SDL_Renderer* mRenderer;
    SDL_Texture* snakeTexture;
    SDL_Texture* foodTexture;
    SDL_Rect headRect, bodyRect, tailRect;
    SDL_Rect mPlayer;
    SDL_Rect food;
    std::vector<SDL_Rect> snake;
    TTF_Font* gameOverFont;
    SDL_Rect playAgainButton;
    Grid mGrid;

    // Minimal additions for touch input and controller support
    float initialTouchX, initialTouchY;  // For swipe detection
    static const float swipeThreshold;  // Minimum movement for a swipe to be detected
    SDL_GameController* gameController;  // Game controller pointer
    static const int JOYSTICK_THRESHOLD;

private:
    void update(float deltaTime);
    void processEvent(); // Handle keyboard, touch, and controller input
    void handlePlayerInput(SDL_KeyboardEvent key, bool isPressed);
    void handleControllerInput(SDL_JoyButtonEvent button, bool isPressed);  // Handle controller input
    void handleJoystickMotion(SDL_JoyAxisEvent axis);  // Handle joystick motion
    void handleHatMotion(SDL_JoyHatEvent hat); // HAndle hat motion - actually xbox dpad... smh
    bool loadMedia();
    void placeFood();
    bool checkSelfCollision();
    void resetGame();
    void render();
    void clean();
    void increaseSpeed();
    static void emscripten_loop(void* arg);

    // Swipe detection functions
    void handleSwipeUp();
    void handleSwipeDown();
    void handleSwipeLeft();
    void handleSwipeRight();

public:
    Game();
    bool init();
    void run();
};

#endif // GAME_HPP

#include "Game.hpp"
#include <iostream>
#include <string>
#include <ctime>   
#include <cstdlib>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

const float Game::swipeThreshold = 0.25f;
const int Game::FPS = 7;
const int Game::maxSnakeSize = 30;
const int Game::speedIncreaseThreshold = 5;
Uint32 Game::TimePerFrame = 1000 / FPS;
const int Game::JOYSTICK_THRESHOLD = 25000;  // Joystick threshold (for movement sensitivity)


Game::Game()
	: mWindow(nullptr)
	, mRenderer(nullptr)
	, isRunning(false)
	, isGameOver(false)
	, gameOverFont(nullptr)
	, snakeTexture(nullptr)
	, foodTexture(nullptr)
	, mIsMovingUp(false)
	, mIsMovingRight(false)
	, mIsMovingDown(false)
	, mIsMovingLeft(false)
	, gameController(nullptr)
	, mDirectionX(0)
	, mDirectionY(0)
	, currentDirectionX(0)
	, currentDirectionY(0)
	, mPlayer()
	, playAgainButton()
	, food()
	, initialTouchX(0.0f)
	, initialTouchY(0.0f)
	, score(0)
	, mGrid(SCREEN_WIDTH, SCREEN_HEIGHT, CELL_SIZE)
{
	mPlayer = { (SCREEN_WIDTH - CELL_SIZE) / 2, (SCREEN_HEIGHT - CELL_SIZE) / 2, CELL_SIZE, CELL_SIZE };

	snake.push_back(mPlayer);

	srand(time(0));
	placeFood();
}

// Initialize Game
bool Game::init() {
	// Initialize SDL

#ifdef __EMSCRIPTEN__
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) {
		std::cout << "SDL could not be initialized!" << std::endl
			<< "SDL_Error: " << SDL_GetError() << std::endl;
		return false;
	}
#else
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {

		std::cout << "SDL could not be initialized!" << std::endl
			<< "SDL_Error: " << SDL_GetError() << std::endl;
		return false;

	}
#endif

	if (TTF_Init() == -1) {  // Initialize SDL_ttf
		std::cout << "TTF_Init failed: " << TTF_GetError() << std::endl;
		return false;
	}

	if (IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG) {
		std::cout << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << std::endl;
		return false;
	}

	// Create Window
	mWindow = SDL_CreateWindow("Snake", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);

	if (!mWindow) {
		std::cout << "Window could not be created!" << std::endl
			<< "SDL_Error: " << SDL_GetError() << std::endl;
		return false;
	}
	else {

		// Create Renderer
		mRenderer = SDL_CreateRenderer(mWindow, -1, SDL_RENDERER_ACCELERATED);

		if (!mRenderer) {
			std::cout << "Renderer could not be created!" << std::endl
				<< "SDL_Error: " << SDL_GetError() << std::endl;
			return false;
		}

	}

	for (int i = 0; i < SDL_NumJoysticks(); ++i) {
		if (SDL_IsGameController(i)) {
			std::cout << "Controller found: " << SDL_JoystickNameForIndex(i) << std::endl;
			gameController = SDL_GameControllerOpen(i);
			if (gameController) {
				std::cout << "Controller " << SDL_JoystickNameForIndex(i) << " successfully opened." << std::endl;
			}
		}
	}
	if (gameController == NULL) {
		std::cout << "No controller detected!" << std::endl;
	}

	if (!loadMedia()) {
		std::cout << "Failed to load media!" << std::endl;
		return false;
	}


	isRunning = true;
	return true;
}

// Make emscripten_loop static
void Game::emscripten_loop(void* arg) {
	Game* game = static_cast<Game*>(arg); // Cast the void pointer to Game* object

	Uint32 currentTime = SDL_GetTicks();
	static Uint32 previousTime = currentTime;  // Static variable to persist across calls
	Uint32 elapsedTime = currentTime - previousTime;
	previousTime = currentTime;

	static Uint32 timeSinceLastUpdate = 0;  // Static accumulator for fixed time step
	timeSinceLastUpdate += elapsedTime;

	game->processEvent(); // Process events in each loop iteration

	// Handle fixed time step updates
	while (timeSinceLastUpdate >= game->TimePerFrame) {
		timeSinceLastUpdate -= game->TimePerFrame;
		game->update(game->TimePerFrame / 1000.0f); // Update the game logic
	}

	game->render(); // Render the game
}



// Run Game
void Game::run() {

	Uint32 previousTime = SDL_GetTicks();
	Uint32 timeSinceLastUpdate = 0;

#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop_arg(emscripten_loop, this, 0, 1);
#else

	while (isRunning) {
		Uint32 currentTime = SDL_GetTicks();
		Uint32 elapsedTime = currentTime - previousTime;
		previousTime = currentTime;
		timeSinceLastUpdate += elapsedTime;

		processEvent();

		while (timeSinceLastUpdate >= TimePerFrame) {
			timeSinceLastUpdate -= TimePerFrame;
			processEvent();
			update(TimePerFrame / 1000.0f);
		}

		render();
	}
	clean();
#endif
}


// Poll and handle all events, such as input
void Game::processEvent() {
	SDL_Event event;
	if (SDL_PollEvent(&event)) {
		switch (event.type) {

		case SDL_CONTROLLERDEVICEADDED:
			// A controller was plugged in, try to open it
			if (SDL_IsGameController(event.cdevice.which)) {
				gameController = SDL_GameControllerOpen(event.cdevice.which);
				if (gameController) {
					std::cout << "Controller connected: " << SDL_JoystickName(SDL_GameControllerGetJoystick(gameController)) << std::endl;
				}
				else {
					std::cout << "Failed to open controller." << std::endl;
				}
			}
			break;

		case SDL_CONTROLLERDEVICEREMOVED:
			// A controller was removed, close it
			if (gameController && !SDL_GameControllerGetAttached(gameController)) {
				std::cout << "Controller disconnected: " << SDL_JoystickName(SDL_GameControllerGetJoystick(gameController)) << std::endl;
				SDL_GameControllerClose(gameController);
				gameController = nullptr; // Clear the controller object
			}
			break;

		case SDL_KEYDOWN:
			handlePlayerInput(event.key, true);
			break;

		case SDL_KEYUP:
			handlePlayerInput(event.key, false);
			break;

		case SDL_MOUSEBUTTONDOWN:
			if (isGameOver) {
				int mouseX = event.button.x;
				int mouseY = event.button.y;

				if (mouseX >= playAgainButton.x && mouseX <= (playAgainButton.x + playAgainButton.w) &&
					mouseY >= playAgainButton.y && mouseY <= (playAgainButton.y + playAgainButton.h)) {
					resetGame();
				}
			}
			break;

		case SDL_FINGERDOWN: {			

			initialTouchX = event.tfinger.x * WINDOW_WIDTH;
			initialTouchY = event.tfinger.y * WINDOW_HEIGHT;

			break;
		}

		
		case SDL_FINGERMOTION: {
					

			// Normalize release position
			float touchX = event.tfinger.x * WINDOW_WIDTH;
			float touchY = event.tfinger.y * WINDOW_HEIGHT;

			// Calculate swipe deltas
			float deltaX = touchX - initialTouchX;
			float deltaY = touchY - initialTouchY;

			// Determine if swipe exceeds threshold
			if (std::abs(deltaX) > std::abs(deltaY)) {
				// Horizontal swipe
				if (std::abs(deltaX) > swipeThreshold) {
					if (deltaX > 0) {
						handleSwipeRight(); // Trigger right swipe
					}
					else {
						handleSwipeLeft();  // Trigger left swipe
					}
				}
			}
			else {
				// Vertical swipe
				if (std::abs(deltaY) > swipeThreshold) {
					if (deltaY > 0) {
						handleSwipeDown();  // Trigger down swipe
					}
					else {
						handleSwipeUp();    // Trigger up swipe
					}
				}
			}

			break;
		}


		case SDL_JOYHATMOTION:
			handleHatMotion(event.jhat);
			break;

		case SDL_JOYBUTTONDOWN:
			handleControllerInput(event.jbutton, true);
			break;

		case SDL_JOYBUTTONUP:
			handleControllerInput(event.jbutton, false);
			break;

		case SDL_JOYAXISMOTION:
			handleJoystickMotion(event.jaxis);
			break;

		case SDL_QUIT:
			isRunning = false;
#ifdef __EMSCRIPTEN__
			emscripten_cancel_main_loop();
#endif
			break;

		default:
			break;
		}
	}
}

void Game::handleSwipeUp() {
	if (currentDirectionY != 1) {  // Prevent the snake from reversing direction
		mDirectionX = 0;
		mDirectionY = -1;  // Move up
	}
}

void Game::handleSwipeDown() {
	if (currentDirectionY != -1) {
		mDirectionX = 0;
		mDirectionY = 1;  // Move down
	}
}

void Game::handleSwipeLeft() {
	if (currentDirectionX != 1) {
		mDirectionX = -1;
		mDirectionY = 0;  // Move left
	}
}

void Game::handleSwipeRight() {
	if (currentDirectionX != -1) {
		mDirectionX = 1;
		mDirectionY = 0;  // Move right
	}
}


void Game::handleControllerInput(SDL_JoyButtonEvent button, bool isPressed) {
	if (isPressed) {
		switch (button.button) {

#ifdef __EMSCRIPTEN__
			//Web Gamepad Api

		case 12: // Directional Up	
			handleSwipeUp();
			break;
		case 13: // Directional Down	
			handleSwipeDown();
			break;
		case 14: // Directional Left	
			handleSwipeLeft();
			break;
		case 15: // Directional Right
			handleSwipeRight();
			break;
#else
		case SDL_CONTROLLER_BUTTON_DPAD_UP:
			handleSwipeUp();
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
			handleSwipeDown();
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
			handleSwipeLeft();
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
			handleSwipeRight();
			break;

#endif
		default:
			break;
		}

		if (isGameOver && button.button == SDL_CONTROLLER_BUTTON_A) {
			resetGame();
		}
	}
}


void Game::handleJoystickMotion(SDL_JoyAxisEvent axis) {
	if (axis.axis == SDL_CONTROLLER_AXIS_LEFTX) {
		if (axis.value < -JOYSTICK_THRESHOLD) {
			handleSwipeLeft();
		}
		else if (axis.value > JOYSTICK_THRESHOLD) {
			handleSwipeRight();
		}
	}
	else if (axis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
		if (axis.value < -JOYSTICK_THRESHOLD) {
			handleSwipeUp();
		}
		else if (axis.value > JOYSTICK_THRESHOLD) {
			handleSwipeDown();
		}
	}
}


// Xbox Controller
void Game::handleHatMotion(SDL_JoyHatEvent hat) {
	switch (hat.value) {
	case SDL_HAT_UP:
		handleSwipeUp();
		break;
	case SDL_HAT_DOWN:
		handleSwipeDown();
		break;
	case SDL_HAT_LEFT:
		handleSwipeLeft();
		break;
	case SDL_HAT_RIGHT:
		handleSwipeRight();
		break;
	default:
		break;
	}

}



void Game::handlePlayerInput(SDL_KeyboardEvent key, bool isPressed) {
	if (isPressed) {
		if ((key.keysym.sym == SDLK_w || key.keysym.sym == SDLK_UP) && currentDirectionY != 1) {
			handleSwipeUp();
		}
		else if ((key.keysym.sym == SDLK_s || key.keysym.sym == SDLK_DOWN) && currentDirectionY != -1) {
			handleSwipeDown();
		}
		else if ((key.keysym.sym == SDLK_a || key.keysym.sym == SDLK_LEFT) && currentDirectionX != 1) {
			handleSwipeLeft();
		}
		else if ((key.keysym.sym == SDLK_d || key.keysym.sym == SDLK_RIGHT) && currentDirectionX != -1) {
			handleSwipeRight();
		}
	}
}



// Updates the game logic
void Game::update(float deltaTime) {
	// Update player position
	mPlayer.x += mDirectionX * mGrid.getCellSize();
	mPlayer.y += mDirectionY * mGrid.getCellSize();

	currentDirectionX = mDirectionX;
	currentDirectionY = mDirectionY;

	// Snap the player's position to the grid
	SDL_Point snappedPos = mGrid.snapToGrid(mPlayer.x, mPlayer.y);
	mPlayer.x = snappedPos.x;
	mPlayer.y = snappedPos.y;

	SDL_Rect previousTail = snake.back();

	// Move each segment of the snake to follow the previous one
	for (size_t i = snake.size() - 1; i > 0; --i) {
		snake[i] = snake[i - 1];
	}
	snake[0] = mPlayer; // Update the head position in the snake vector

	// Check for food collision
	if (SDL_HasIntersection(&mPlayer, &food)) {

		// Grow the snake by adding a new segment
		if (snake.size() < maxSnakeSize) {
			SDL_Rect newSegment = { previousTail.x, previousTail.y, CELL_SIZE, CELL_SIZE };
			snake.push_back(newSegment);
		}

		// Place food in a new random location
		placeFood();

		score++;

		if (snake.size() <= maxSnakeSize && score % speedIncreaseThreshold == 0) {
			increaseSpeed();
		}
	}

	// Game Over if player goes out of screen bounds
	if (mPlayer.x < 0 || mPlayer.x + mPlayer.w > SCREEN_WIDTH || mPlayer.y < 0 || mPlayer.y + mPlayer.h > SCREEN_HEIGHT)

		isGameOver = true;


	// In the main update loop:
	if (checkSelfCollision()) {

		isGameOver = true; // End game if the snake collides with itself
	}
}

void Game::increaseSpeed() {
	if (TimePerFrame > 80) { // Prevent the game from becoming too fast
		TimePerFrame -= 10; // Decrease frame time to make it faster

	}
	std::cout << "Speed increased! Current TimePerFrame: " << TimePerFrame << " ms/frame" << std::endl;
}


void Game::placeFood() {
	int gridWidth = SCREEN_WIDTH / CELL_SIZE;
	int gridHeight = SCREEN_HEIGHT / CELL_SIZE;

	bool isOverlapping;

	do {

		food.x = (std::rand() % gridWidth) * CELL_SIZE;
		food.y = (std::rand() % gridHeight) * CELL_SIZE;
		food.w = CELL_SIZE;
		food.h = CELL_SIZE;


		isOverlapping = false;
		for (const SDL_Rect& segment : snake) {
			SDL_Rect foodRect = { food.x, food.y, food.w, food.h };

			// Check if the food rect intersects with the snake segment
			if (SDL_HasIntersection(&foodRect, &segment)) {
				isOverlapping = true;
				break;
			}
		}
	} while (isOverlapping);
}




// Renders Game to the window
void Game::render() {
	SDL_SetRenderDrawColor(mRenderer, 75, 105, 47, SDL_ALPHA_OPAQUE);  // Set background to white
	SDL_RenderClear(mRenderer);

	int gridYOffset = WINDOW_HEIGHT - (mGrid.getCellSize() * mGrid.getGridHeight());



	//mGrid.drawBoundary(mRenderer, gridYOffset);

	SDL_Color textColor = { 255, 255, 255, SDL_ALPHA_OPAQUE };  // Green color for Game Over text
	SDL_Color playAgainColor = { 255, 255, 255, SDL_ALPHA_OPAQUE };  // White color for Game Over text
	SDL_Color scoreColor = { 255, 255, 255, SDL_ALPHA_OPAQUE };


	if (isGameOver) {
		// Render "Game Over" text

		SDL_SetRenderDrawColor(mRenderer, 153, 229, 80, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(mRenderer);

		std::string scoreText = "Score: " + std::to_string(score);
		SDL_Surface* gameOverSurface = TTF_RenderText_Solid(gameOverFont, scoreText.c_str(), textColor);


		SDL_Texture* gameOverTexture = SDL_CreateTextureFromSurface(mRenderer, gameOverSurface);
		SDL_Rect gameOverRect;
		gameOverRect.x = (WINDOW_WIDTH - gameOverSurface->w) / 2;  // Center the text
		gameOverRect.y = (WINDOW_HEIGHT - gameOverSurface->h) / 2 - 50;  // Position above button
		gameOverRect.w = gameOverSurface->w;
		gameOverRect.h = gameOverSurface->h;

		SDL_RenderCopy(mRenderer, gameOverTexture, NULL, &gameOverRect);

		SDL_DestroyTexture(gameOverTexture);
		SDL_FreeSurface(gameOverSurface);


		// Centered and below "Game Over"


	   // Draw "Play Again" text centered within the button
		SDL_Surface* playAgainSurface = TTF_RenderText_Solid(gameOverFont, "Play Again", playAgainColor);
		if (playAgainSurface) {
			SDL_Texture* playAgainTexture = SDL_CreateTextureFromSurface(mRenderer, playAgainSurface);

			const int buttonPadding = 15;

			playAgainButton = { (WINDOW_WIDTH - playAgainSurface->w) / 2, (WINDOW_HEIGHT - playAgainSurface->h) / 2 + 50, playAgainSurface->w, playAgainSurface->h };

			SDL_Rect innerButtonRect = {
			playAgainButton.x + buttonPadding,
			playAgainButton.y + buttonPadding,
			playAgainButton.w - 2 * buttonPadding, // Subtract padding from width
			playAgainButton.h - 2 * buttonPadding  // Subtract padding from height
			};





			SDL_SetRenderDrawColor(mRenderer, 75, 105, 47, 255);  // Green color for the button
			SDL_RenderFillRect(mRenderer, &playAgainButton);
			SDL_SetRenderDrawColor(mRenderer, 75, 105, 47, 255);  //  Green for the inner button with padding
			SDL_RenderFillRect(mRenderer, &innerButtonRect);
			SDL_RenderCopy(mRenderer, playAgainTexture, NULL, &innerButtonRect);


			SDL_DestroyTexture(playAgainTexture);
			SDL_FreeSurface(playAgainSurface);
		}
	}
	else {
		mGrid.draw(mRenderer, gridYOffset);
		// Render the score
		SDL_Surface* scoreSurface = TTF_RenderText_Solid(gameOverFont, std::to_string(score).c_str(), scoreColor);
		SDL_Texture* scoreTexture = SDL_CreateTextureFromSurface(mRenderer, scoreSurface);

		SDL_Rect scoreRect;
		scoreRect.x = 10;  // Position the score at the top left corner
		scoreRect.y = 0;
		scoreRect.w = scoreSurface->w;
		scoreRect.h = scoreSurface->h;

		SDL_RenderCopy(mRenderer, scoreTexture, NULL, &scoreRect);

		// Clean up
		SDL_DestroyTexture(scoreTexture);
		SDL_FreeSurface(scoreSurface);


		// Render each segment of the snake using the sprite sheet
		for (size_t i = 0; i < snake.size(); ++i) {
			SDL_Rect segment = snake[i];  // The segment's position

			segment.y += gridYOffset;


			SDL_Rect* currentSprite = nullptr;

			// Select which sprite to use based on the segment's index
			if (i == 0) {
				currentSprite = &headRect;  // Head
			}
			//else if (i == snake.size() - 1) {
			//	currentSprite = &tailRect;  // Tail
			//}
			else {
				currentSprite = &bodyRect;  // Body
			}


			// Render the segment of the snake using the sprite
			SDL_RenderCopy(mRenderer, snakeTexture, currentSprite, &segment);
		}

		// Render food
		//SDL_SetRenderDrawColor(mRenderer, 0x00, 0xFF, 0x00, 0xFF);  // Green color for food
		//SDL_RenderFillRect(mRenderer, &food);
		SDL_Rect foodRenderRect = food;
		foodRenderRect.y += gridYOffset;

		// Render food
		SDL_RenderCopy(mRenderer, foodTexture, NULL, &foodRenderRect);
	}

	SDL_RenderPresent(mRenderer);
}

bool Game::loadMedia() {

	// Load the icon image
	SDL_Surface* iconSurface = IMG_Load("Assets/Snake_Head.png");
	if (iconSurface == nullptr) {
		std::cout << "Failed to load icon image:\n"
			<< IMG_GetError();
		// handle error
	}
	else {
		// Set the window icon
		SDL_SetWindowIcon(mWindow, iconSurface);

		// Free the icon surface
		SDL_FreeSurface(iconSurface);
	}

	// Load sprite sheet as PNG
	snakeTexture = IMG_LoadTexture(mRenderer, "Assets/Snake.png");  // Path to your PNG file
	if (!snakeTexture) {
		std::cout << "Unable to load texture! SDL_image Error: " << IMG_GetError() << std::endl;
		return false;
	}

	foodTexture = IMG_LoadTexture(mRenderer, "Assets/Food.png");
	if (!foodTexture) {
		std::cout << "Unable to load texture! SDL_image Error: " << IMG_GetError() << std::endl;
		return false;
	}

	gameOverFont = TTF_OpenFont("Assets/BigSpace.ttf", 48);

	if (!gameOverFont) {
		std::cout << "Failed to load font: " << TTF_GetError() << std::endl;
		return false;
	}

	// Define the source rectangles for the sprite sheet
	headRect = { 0, 0, 120, 120 };  // Head sprite (0,0) at 120x120
	bodyRect = { 120, 0, 120, 120 };  // Body sprite (120,0) at 120x120
	tailRect = { 240, 0, 120, 120 };  // Tail sprite (0,120) at 120x120

	return true;
}



bool Game::checkSelfCollision() {
	for (size_t i = 1; i < snake.size(); ++i) {
		if (SDL_HasIntersection(&mPlayer, &snake[i])) {
			return true;
		}
	}
	return false;
}


void Game::resetGame() {
	// Reset game state
	isGameOver = false;
	isRunning = true;

	// Reset player position and direction
	mPlayer = { (SCREEN_WIDTH - CELL_SIZE) / 2, (SCREEN_HEIGHT - CELL_SIZE) / 2, CELL_SIZE, CELL_SIZE };
	mDirectionX = 0;
	mDirectionY = 0;
	currentDirectionX = mDirectionX;
	currentDirectionY = mDirectionY;

	// Clear and reset snake segments
	snake.clear();
	snake.push_back(mPlayer);

	// Place the food in a new location
	placeFood();

	score = 0;

	TimePerFrame = 1000 / FPS;
}






// Cleans up Game 
void Game::clean()
{

	if (gameOverFont) {
		TTF_CloseFont(gameOverFont);
		gameOverFont = nullptr;
	}

	if (snakeTexture) {
		SDL_DestroyTexture(snakeTexture);
		snakeTexture = nullptr;
	}

	if (foodTexture) {
		SDL_DestroyTexture(foodTexture);
		foodTexture = nullptr;
	}

	if (gameController) {
		SDL_GameControllerClose(gameController);
		gameController = nullptr;
	}

	SDL_DestroyWindow(mWindow);
	SDL_DestroyRenderer(mRenderer);
	TTF_Quit();
	IMG_Quit();
	SDL_Quit();
}


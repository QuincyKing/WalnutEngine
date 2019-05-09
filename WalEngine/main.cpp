#include "./game/game.h"
#include "./src/render/RenderEngine.h"

int screen_width, screen_height;
std::shared_ptr<Game> game;

#include "./src/core/Quaternion.h"

void GetDesktopResolution(int& horizontal, int& vertical)
{
	RECT desktop;
	const HWND hDesktop = GetDesktopWindow();
	GetWindowRect(hDesktop, &desktop);
	horizontal = desktop.right;
	vertical = desktop.bottom;
}

int main()
{
	GetDesktopResolution(screen_width, screen_height);
	Window window(screen_width, screen_height);
	RenderEngine renderer(window);
	game = std::make_shared<Game>();
	window.mUpdateFun = [&renderer, &window]() { game->render(renderer, window); };
	window.mInitFun = []() { game->init(); };

	window.onrun();
	
	return 0;
}
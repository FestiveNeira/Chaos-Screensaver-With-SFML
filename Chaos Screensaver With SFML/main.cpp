#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <iostream>
#include <random>
#include <sstream>
#include <cassert>
#include <fstream>
#include "resource.h"

//Supposedly speeds along compilation
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

//Editable Data
static const bool fullscreen = true;					//Run in fullscreen?
static const bool usesincos = false;					//Allow sin and cos in function generation?
static const bool usefloatconstants = true;				//Allow decimal constants in equation generation?
static const float outsiderendermult = 1.3;				//How much extra space outside the borders should be rendered? (between 1 and 2)
static const float scale = 0.5;							//Default drawing size multiplier
static const float pixelSize = 1.25;					//Size of drawn pixels
static const int pixelCount = 1000;						//Number of pixel paths
static const int stepsPerFrame = 500;					//Number of iterations of pixels per frame (relates to trail length)
static const int coloralpha = 16;						//Alpha value of colors
static const sf::Color color(0, 0, 0, coloralpha);		//Pixel color (use (0,0,0,x) for random or (r,g,b,coloralpha) for selected)

//Global constants
static const int pixelArrCount = pixelCount * stepsPerFrame;
static const int equn = 18;
static const int equnsc = 8;
static const double stepInterval = 1e-5;
static const double stepSmall = 1e-7;
static const double tstart = -3.0;
static const double tend = 3.0;
static const char base26[26] = { 'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z' };

//Global variables
static double windowResW = 1920;
static double windowResH = 1080;
static double windowW = windowResW;
static double windowH = windowResH;
static float boundNegX, boundPosX, boundNegY, boundPosY;
static float minWidth, minHeight, maxWidth, maxHeight;
static bool allOffScreen = true;
static double math[equn];
static double sincos[equnsc];
static double dynamicStepSpeed = stepInterval;
static std::vector<sf::Vector2f> history(pixelCount);
static std::vector<sf::Vector2f> screenhistory(pixelCount);
static std::vector<sf::Vertex> pixels(pixelArrCount);
static std::mt19937 randgen;
static double speedmult = 1.0;
static unsigned int seedtime;
static int seediter = 0;
static std::string seed;

sf::Vector2f vctarget = { 0.0, 0.0 };
sf::Vector2f vstarget = { (float)windowW, (float)windowH };

sf::Vector2f viewCenter = { 0.0, 0.0 };
sf::Vector2f viewSize = { (float)maxWidth, (float)maxHeight };
double viewZoom = 0.25;

//Write the current seed to a file
static void saveSeed() {
	//Unimplemented
}

//Loads data from a given seed and nuumber of random value iterations to ignore
static void loadSeed(int newseed, int iter) {
	randgen.seed(newseed);
	for (int i = 0; i < iter; i++) {
		std::uniform_real_distribution<float> randfloat(-1, 3);
		float r = randfloat(randgen);
	}
	RandPosNegMultGen();
}

//Encodes the seed data to a string (Format (uppercase chars)-(positive integer))
static void encodeSeed() {
	std::string str = "";
	unsigned int temp = seedtime;
	int pow = 1;
	while (temp > 0)
	{
		int val = temp % (int)std::pow(26, pow);
		temp -= val;
		val = val / (int)std::pow(26, pow - 1);
		str = str + base26[val];
		pow++;
	}
	seed = str + "-" + std::to_string(seediter);
}

//Decodes a string (Format (uppercase chars)-(positive integer)) into seed data
static sf::Vector2i decodeSeed(std::string s) {
	int i = 0;
	int newseed = 0;
	int pow = 1;
	for (i = 0; i < s.length(); i++) {
		if (s[i] == '-') {
			break;
		}
		newseed += s[i] * std::pow(26, pow);
		pow++;
	}
	int val = std::stoi(s.substr(i + 1));
	return sf::Vector2i(newseed, val);
}

//Generate random color (not my code)
static sf::Color GetRandColor(int i) {
	i += 1;
	int r = std::min(255, 50 + (i * 11909) % 256);
	int g = std::min(255, 50 + (i * 52973) % 256);
	int b = std::min(255, 50 + (i * 44111) % 256);
	return sf::Color(r, g, b, 16);
}

//Generates an array of equn elements from -1 to 1 at a 0.25, 0.5, 0.25 ratio
static void RandPosNegMultGen() {
	std::uniform_real_distribution<float> randfloat(-1, 3);
	for (int i = 0; i < equn; i++) {
		float r = randfloat(randgen);
		seediter++;
		//If not using float remove decimals
		if (!usefloatconstants) {
			if (r < 0)
				r = -1.0f;
			else if (r <= 1)
				r = 1.0f;
		}
		if (r <= 1) {
			math[i] = r;
		}
		else {
			math[i] = 0.0f;
		}
	}
	for (int i = 0; i < equnsc; i++) {
		float r = randfloat(randgen);
		seediter++;
		//If not using float remove decimals
		if (!usefloatconstants) {
			if (r < 0)
				r = -1.0f;
			else if (r <= 1)
				r = 1.0f;
		}
		if (r <= 1 && usesincos) {
			sincos[i] = r;
		}
		else {
			sincos[i] = 0.0f;
		}
	}

	/*
	math[0] = 0;
	math[1] = 0;
	math[2] = 0.52123534679412842;
	math[3] = 0;
	math[4] = -0.18084508180618286;
	math[5] = -0.83450746536254883;
	math[6] = 0;
	math[7] = 0;
	math[8] = 0;
	math[9] = 0.093032360076904297;
	math[10] = 0;
	math[11] = -0.95438283681869507;
	math[12] = 0;
	math[13] = 0;
	math[14] = 0;
	math[15] = -0.65500193834304810;
	math[16] = 0;
	math[17] = 0.086771368980407715;
	sincos[0] = 0;
	sincos[1] = 0;
	sincos[2] = 0;
	sincos[3] = 0;
	sincos[4] = 0;
	sincos[5] = 0;
	sincos[6] = 0;
	sincos[7] = 0;
	//*/
}

//Resets zoom and translation at the start of a new simulation
static void ResetView(sf::RenderWindow& window) {
	sf::View nv = window.getView();

	viewCenter = { 0.0f, 0.0f };
	viewSize = { (float)maxWidth, (float)maxHeight };

	nv.setCenter(viewCenter);
	nv.setSize(viewSize);
}

//Creates the window!
static void CreateRenderWindow(sf::RenderWindow& window, bool full) {
	//OpenGL window settings
	sf::ContextSettings settings;
	settings.depthBits = 24;
	settings.stencilBits = 8;
	settings.antialiasingLevel = 8;
	settings.majorVersion = 3;
	settings.minorVersion = 0;

	//Create the window
	const sf::VideoMode screenData = sf::VideoMode::getDesktopMode();

	//Set resolution
	windowResW = screenData.width;
	windowResH = screenData.height;

	int div = 1;
	if (!full) div = 2;
	windowW = screenData.width / div;
	windowH = screenData.height / div;
	window.create(full ? screenData : sf::VideoMode(windowW, windowH), "Chaos", (full ? sf::Style::Fullscreen : sf::Style::Close), settings);
	window.setFramerateLimit(60);
	window.setVerticalSyncEnabled(true);
	window.setActive(false);
	window.setMouseCursorVisible(full ? false : true);
	window.requestFocus();

	//Set barrier variables
	boundPosX = windowResW * 2;
	boundNegX = -windowResW * 2;
	boundPosY = windowResH * 2;
	boundNegY = -windowResH * 2;
	minWidth = windowResW;
	minHeight = windowResH;
	maxWidth = minWidth * 4;
	maxHeight = minHeight * 4;

	//Set the view
	viewCenter = { 0.0, 0.0 };
	viewSize = { maxWidth, maxHeight };
	viewZoom = 0.25;

	sf::View view = window.getView();
	view.setCenter(viewCenter);
	view.setSize(viewSize);
	view.zoom(viewZoom);
	window.setView(view);
}

static void CreateRenderImage(sf::RenderTexture& image) {
	//OpenGL render settings
	sf::ContextSettings settings;
	settings.depthBits = 24;
	settings.stencilBits = 8;
	settings.antialiasingLevel = 8;
	settings.majorVersion = 3;
	settings.minorVersion = 0;

	image.create(maxWidth * outsiderendermult, maxHeight * outsiderendermult, settings);
	image.setActive(false);

	//Center the view
	sf::View view = image.getView();
	view.setCenter(0, 0);
	image.setView(view);
}

//Converts mathmatical positions of the pixels to screen coordinates
static sf::Vector2f convertPixelToScreen(double x, double y) {
	const float s = scale * (windowResH / 2);
	const float nx = (float(x)) * s;
	const float ny = (float(y)) * s;
	return sf::Vector2f(nx, ny);
}

//Takes in the previous pixel and appliies functions to generate the next one
static sf::Vector2f applyEquations(double x, double y, double t) {
	const double xx = x * x;
	const double yy = y * y;
	const double tt = t * t;
	const double xy = x * y;
	const double xt = x * t;
	const double yt = y * t;
	const double sinx = sin(x);
	const double siny = sin(y);
	const double cosx = cos(x);
	const double cosy = cos(y);
	const double nx = xx * math[0] + yy * math[1] + tt * math[2] + xy * math[3] + xt * math[4] + yt * math[5] + x * math[6] + y * math[7] + t * math[8] + sinx * sincos[0] + siny * sincos[1] + cosx * sincos[2] + cosy * sincos[3];
	const double ny = xx * math[9] + yy * math[10] + tt * math[11] + xy * math[12] + xt * math[13] + yt * math[14] + x * math[15] + y * math[16] + t * math[17] + sinx * sincos[4] + siny * sincos[5] + cosx * sincos[6] + cosy * sincos[7];
	return sf::Vector2f(nx, ny);
}

static void moveWindowToward(sf::RenderWindow& window, sf::Vector2f c, sf::Vector2f s) {
	sf::View nv = window.getView();

	//Smooth update
	viewCenter.x = (viewCenter.x * 0.99) + (c.x * 0.01);
	viewCenter.y = (viewCenter.y * 0.99) + (c.y * 0.01);
	viewSize.x = (viewSize.x * 0.99) + (s.x * 0.01);
	viewSize.y = (viewSize.y * 0.99) + (s.y * 0.01);
	viewZoom = windowResW / viewSize.x;

	//Move window position
	nv.setCenter(viewCenter);
	//Resize window
	nv.setSize(viewSize);

	window.setView(nv);
}

//Update the location and size of the screen (DEFINITELY REWORK)
static void updateView(sf::RenderWindow& window) {
	float minx = FLT_MAX;
	float maxx = -FLT_MAX;
	float miny = FLT_MAX;
	float maxy = -FLT_MAX;
	for (int i = 0; i < screenhistory.size(); i++) {
		minx = std::fmin(minx, screenhistory[i].x);
		maxx = std::fmax(maxx, screenhistory[i].x);
		miny = std::fmin(miny, screenhistory[i].y);
		maxy = std::fmax(maxy, screenhistory[i].y);
	}
	sf::Vector2f centernew((minx + maxx) / 2, (miny + maxy) / 2);
	sf::Vector2f sizenew((maxx - minx), (maxy - miny));

	//Calculate new window size to fit pixels
	if (maxWidth >= sizenew.x && maxHeight >= sizenew.y) {
		sizenew.x += sizenew.x * 0.1;
		sizenew.y += sizenew.y * 0.1;
		if (sizenew.x / sizenew.y > windowResW / windowResH) {
			sizenew.y = ((windowResH / windowResW) * sizenew.x);
		}
		else {
			sizenew.x = ((windowResW / windowResH) * sizenew.y);
		}
	}

	//Bound screen size
	sizenew.x = std::fmax(minWidth, sizenew.x);
	sizenew.y = std::fmax(minHeight, sizenew.y);
	sizenew.x = std::fmin(maxWidth, sizenew.x);
	sizenew.y = std::fmin(maxHeight, sizenew.y);

	//only use valid data
	if (!isnan(centernew.x) && !isnan(centernew.y) && centernew.x != INFINITY && centernew.y != INFINITY) {
		vctarget = centernew;
	}
	if (!isnan(sizenew.x) && !isnan(sizenew.y) && sizenew.x != INFINITY && sizenew.y != INFINITY) {
		vstarget = sizenew;
	}

	moveWindowToward(window, vctarget, vstarget);
}

//Update pixel positions
static void updatePixels(double& t) {
	//Smooth out the stepping speed.
	const int steps = stepsPerFrame;
	const double speed = stepInterval * speedmult;
	dynamicStepSpeed = dynamicStepSpeed * 0.99 + speed * 0.01;

	//Update the pixel positions
	for (int step = 0; step < steps; step++) {
		allOffScreen = true;
		sf::Vector2f np(t, t);

		for (int i = 0; i < pixelCount; i++) {
			np = applyEquations(np.x, np.y, t);
			sf::Vector2f sp = convertPixelToScreen(np.x, np.y);
			pixels[step * pixelCount + i].position = sp;

			//Check if dynamic speed should be adjusted
			if (sp.x > viewCenter.x - (viewSize.x / 2) && sp.y > viewCenter.y - (viewSize.y / 2) && sp.x < viewCenter.x + (viewSize.x / 2) && sp.y < viewCenter.y + (viewSize.y / 2)) {
				const float dx = history[i].x - float(np.x);
				const float dy = history[i].y - float(np.y);
				const double dist = double(500.0f * std::sqrt(dx * dx + dy * dy));
				dynamicStepSpeed = std::min(dynamicStepSpeed, std::max(speed / (dist + 1e-5), stepSmall * speedmult));
				allOffScreen = false;
			}

			//Store new pixel in history
			history[i].x = float(np.x);
			history[i].y = float(np.y);
			screenhistory[i].x = float(sp.x);
			screenhistory[i].y = float(sp.y);
		}

		//Update the t variable
		if (allOffScreen) {
			t += 0.01;
		}
		else {
			t += dynamicStepSpeed;
		}
	}
}

static void draw(sf::RenderTexture& image, sf::RenderWindow& window) {

	//Draw new points
	glEnable(GL_POINT_SMOOTH);
	float tem = std::powf(viewZoom, -1);
	glPointSize(pixelSize * std::powf(viewZoom, -1));
	image.draw(pixels.data(), pixels.size(), sf::PrimitiveType::Points);

	//Update the frame
	image.display();

	//Draw the frame to the window
	sf::Sprite sprite(image.getTexture());
	sprite.setPosition(boundNegX * outsiderendermult, boundNegY * outsiderendermult);
	window.draw(sprite);

	//Update the window with the new frame
	window.display();
}

//Main program
int main()
{
	//Set random seed
	seedtime = (unsigned int)time(0);
	randgen.seed(seedtime);
	encodeSeed();
	seed =  + "-1";

	//Create the window
	sf::RenderWindow window;
	CreateRenderWindow(window, fullscreen);

	//Create the render space
	sf::RenderTexture image;
	CreateRenderImage(image);

	sf::RectangleShape fullscreenrect;
	fullscreenrect.setPosition(boundNegX * 2, boundNegY * 2);
	fullscreenrect.setSize(sf::Vector2f(maxWidth * 2, maxHeight * 2));
	fullscreenrect.setFillColor(sf::Color(0, 0, 0));
	image.draw(fullscreenrect);

	//Simulation variables
	double t = tstart;
	bool paused = false;

	//Setup the vertex array
	for (int i = 0; i < pixels.size(); i++) {
		if (color == sf::Color(0, 0, 0, 16) || color.a != coloralpha)
			pixels[i].color = GetRandColor(i % pixelCount);
		else
			pixels[i].color = color;
	}

	//Generate initial equations
	RandPosNegMultGen();

	//Tracks mouse movement
	int mousepos[] = { -1, -1 };

	while (window.isOpen()) {
		//Event System
		sf::Event event;
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed /*|| event.type == sf::Event::MouseButtonPressed || event.type == sf::Event::MouseWheelScrolled*/) {
				window.close();
				break;
			}
			else if (event.type == sf::Event::MouseMoved) {
				if ((mousepos[0] < 0 && mousepos[1] < 0) || (event.mouseMove.x == mousepos[0] && event.mouseMove.y == mousepos[1])) {
					mousepos[0] = event.mouseMove.x;
					mousepos[1] = event.mouseMove.y;
				}
				else {
					//window.close();
					break;
				}
			}
			else if (event.type == sf::Event::KeyPressed) {
				const sf::Keyboard::Key keycode = event.key.code;
				//P = Pause
				if (keycode == sf::Keyboard::P) {
					paused = !paused;
				}
				//S = Save seed
				else if (keycode == sf::Keyboard::S) {
					std::ofstream fout("saved.txt", std::ios::app);
					fout << seed << std::endl;
					std::cout << "Saved: " << seed << std::endl;
				}
				//-> = Next
				else if (keycode == sf::Keyboard::Right) {
					ResetView(window);
					RandPosNegMultGen();
					t = tstart;
				}
				//<- = Previous (unimplemented)
				else if (keycode == sf::Keyboard::Left) {

				}
				else {
					//window.close();
					break;
				}
			}
		}

		//Change simulation speed if using shift modifiers
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) {
			speedmult = 0.1;
		}
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)) {
			speedmult = 10.0;
		}
		else {
			speedmult = 1.0;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
			speedmult = -speedmult;
		}

		//If paused do nothing
		if (paused) {
			image.display();
			window.display();
			continue;
		}

		//Restart with new equations after t > tend
		if (t > tend) {
			ResetView(window);
			RandPosNegMultGen();
			t = tstart;
		}

		//Set settings for trail (fadespeed editable)
		sf::BlendMode fade(sf::BlendMode::One, sf::BlendMode::One, sf::BlendMode::ReverseSubtract);
		sf::RenderStates renderBlur(fade);

		//fullscreenrect.setFillColor(sf::Color(0, 0, 0));
		//image.draw(fullscreenrect);

		const sf::Uint8 fadespeed = 10;
		fullscreenrect.setFillColor(sf::Color(fadespeed, fadespeed, fadespeed, 0));
		image.draw(fullscreenrect, renderBlur);

		/*/Test Rect (covers initial area)
		sf::RectangleShape test;
		test.setPosition(-1920 * 2, -1080 * 2);
		test.setSize(sf::Vector2f(1920 * 4, 1080 * 4));
		test.setFillColor(sf::Color(255, 255, 255));
		image.draw(test);
		test.setPosition(-1920, -1080);
		test.setSize(sf::Vector2f(1920 * 2, 1080 * 2));
		test.setFillColor(sf::Color(0, 0, 0));
		image.draw(test);
		test.setPosition(-1920 / 2, -1080 / 2);
		test.setSize(sf::Vector2f(1920, 1080));
		test.setFillColor(sf::Color(255, 255, 255));
		image.draw(test);
		test.setPosition(-1920 * 2, -1080 * 2);
		test.setSize(sf::Vector2f(1920 * 4, 1080 * 4));
		test.setFillColor(sf::Color(0, 255, 0));
		window.draw(test);
		//*/

		//Update pixels
		updatePixels(t);

		updateView(window);

		draw(image, window);
	}

	return 0;
}
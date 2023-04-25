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
#undef min
#undef max

//Editable Data
static const bool fullscreen = true;				//Run in fullscreen?
static const bool usesincos = false;				//Allow sin and cos in function generation?
static const bool usefloatconstants = true;			//Allow decimal constants in equation generation?
static const int pixelCount = 1000;					//Number of pixel paths
static const int stepsPerFrame = 500;				//Number of iterations of pixels per frame (relates to trail length)
static const int coloralpha = 16;					//Alpha value of colors
static sf::Color color(200, 80, 110, coloralpha);	//Pixel color (use (0,0,0,x) for random or (r,g,b,16) for selected)
static const sf::Vector2f ocenter = { 0.0, 0.0 };	//Starting center
static const float oscale = 1;						//Starting scale

//Global constants
static const int pixelArrCount = pixelCount * stepsPerFrame;
static const int equn = 18;
static const int equnsc = 8;
static const double stepInterval = 1e-5;
static const double stepSmall = 1e-7;
static const double tstart = -2.0;
static const double tend = 3.0;

//Global variables
static double windowResW = 1920;
static double windowResH = 1080;
static double windowW = windowResW;
static double windowH = windowResH;
static int boundNegX, boundPosX, boundNegY, boundPosY;
static int minWidth, minHeight, maxWidth, maxHeight;
static bool allOffScreen = true;
static double math[equn];
static double sincos[equnsc];
static double dynamicStepSpeed = stepInterval;
static std::vector<sf::Vector2f> history(pixelCount);
static std::vector<sf::Vector2f> screenhistory(pixelCount);
static std::vector<sf::Vertex> pixels(pixelArrCount);
static bool moving = false;
static std::mt19937 randgen;
static double speedmult = 1.0;
static std::string seed;

sf::Vector2f center = { 0.0, 0.0 };
sf::Vector2f size = { (float)maxWidth, (float)maxHeight };
static float scale = 1;

sf::Vector2f targetcenter = center;

static unsigned int seedtime;
static unsigned int seediter = 0;
static unsigned int nextseediter = 0;
static char base26[26] {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};

static void saveSeed() {

}
static void loadSeed(sf::Vector2i seedval) {

}
//Encodes the seed data to a string (Format (uppercase chars)-(positive integer))
static std::string encodeSeed() {
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
	return str + "-" + std::to_string(seediter);
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
static sf::Color getRandColor(int i) {
	i += 1;
	int r = std::min(255, 50 + (i * 11909) % 256);
	int g = std::min(255, 50 + (i * 52973) % 256);
	int b = std::min(255, 50 + (i * 44111) % 256);
	return sf::Color(r, g, b, 16);
}

//Generates an array of equn elements from -1 to 1 at a 0.25, 0.5, 0.25 ratio
static void randPosNegMultGen() {
	seediter = nextseediter;
	std::uniform_real_distribution<float> randfloat(-1, 3);
	for (int i = 0; i < equn; i++) {
		float r = randfloat(randgen);
		nextseediter++;
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
		nextseediter++;
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
	//Testing Set
	//*
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

//resets zoom and translation at the start of a new simulation
static void resetView(sf::RenderWindow& window) {
	scale = oscale;
	center.x = ocenter.x;
	center.y = ocenter.y;
}

//Creates the window!
static void createRenderWindow(sf::RenderWindow& window, bool full) {
	//OpenGL Window settings
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
	windowW = windowResW / div;
	windowH = windowResH / div;
	window.create(full ? screenData : sf::VideoMode(windowW, windowH), "Chaos", (full ? sf::Style::Fullscreen : sf::Style::Close), settings);
	window.setFramerateLimit(60);
	window.setVerticalSyncEnabled(true);
	window.setActive(false);
	window.setMouseCursorVisible(full ? false : true);
	window.requestFocus();

	//Center the view
	sf::View view = window.getView();
	view.setCenter(0, 0);
	window.setView(view);

	//Set barrier variables
	boundPosX = windowResW;
	boundNegX = -windowResW;
	boundPosY = windowResH;
	boundNegY = -windowResH;
	maxWidth = boundPosX - boundNegX;
	maxHeight = boundPosY - boundNegY;
	minWidth = maxWidth / 8;
	minHeight = maxHeight / 8;

	//Initialize window variables
	center = ocenter;
	size = { (float)maxWidth, (float)maxHeight };
	scale = oscale;
}

//Converts mathmatical positions of the pixels to screen coordinates
static sf::Vector2f convertPixelToScreen(double x, double y) {
	const float s = scale * float(windowResH / 2) * (1.0/8.0);
	float t1 = center.x;
	float t = center.y;
	const float nx = (float(x) * s) - center.x;
	const float ny = (float(y) * s) - center.y;
	return sf::Vector2f(nx, ny);
}

//Takes in the previous pixel and applies functions to generate the next one
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

//Update pixel positions
static void updatePixels(sf::RenderWindow& window, double& t) {
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
			if (sp.x > center.x - (windowResW / 2) && sp.y > center.y - (windowResH / 2) && sp.x < center.x + (windowResW / 2) && sp.y < center.y + (windowResH / 2)) {
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

static void draw(sf::RenderWindow& window, sf::Texture& screen) {
	//Draw new points
	glEnable(GL_POINT_SMOOTH);
	glPointSize(1.0f);
	window.draw(pixels.data(), pixels.size(), sf::PrimitiveType::Points);

	//Draw to window
	window.display();
}

static sf::Vector2f calcNewCenter() {
	float minx = FLT_MAX;
	float maxx = -FLT_MAX;
	float miny = FLT_MAX;
	float maxy = -FLT_MAX;
	//Calculate max distance in each direction
	for (int i = 0; i < screenhistory.size(); i++) {
		minx = std::fmin(minx, screenhistory[i].x - center.x);
		maxx = std::fmax(maxx, screenhistory[i].x - center.x);
		miny = std::fmin(miny, screenhistory[i].y - center.y);
		maxy = std::fmax(maxy, screenhistory[i].y - center.y);
	}

	//Bound distances to screen size
	minx = std::fmax(minx, -windowW);
	maxx = std::fmin(maxx, windowW);
	miny = std::fmax(miny, -windowH);
	maxy = std::fmin(maxy, windowH);

	sf::Vector2f centernew((minx + maxx) / 2, (miny + maxy) / 2);

	//Calculate the destination
	sf::Vector2f dest((centernew.x - center.x) * 0.01, (centernew.y - center.y) * 0.01);

	//create a moving global that is set to true when the screen wants to move more than 5 pixels and is set to false when it only wants to move 1
	//only move when moving is true else leave it alone, this will simulate a sort of escape velocity in order to start moving
	if (std::sqrt(std::pow(dest.x, 2) + std::pow(dest.y, 2)) >= 2.0) {
		moving = true;
	}
	if (std::sqrt(std::pow(dest.x, 2) + std::pow(dest.y, 2)) < 1.0) {
		moving = false;
	}

	//Move 0.01 percent of the distance to the destination
	if (moving) {
		return sf::Vector2f(std::floor(center.x + dest.x), std::floor(center.y + dest.y));
	}
	return center;
}

static void setCenter(sf::RenderWindow& window, sf::Texture& screen, sf::Vector2f c) {
	float tcx = center.x;
	float tcy = center.y;
	center.x = c.x;
	center.y = c.y;

	screen.update(window);
	sf::Sprite sp(screen);
	sp.setOrigin(sp.getLocalBounds().width/2, sp.getLocalBounds().height/2);
	sp.setPosition(tcx-c.x, tcy-c.y);
	window.clear();
	window.draw(sp);
}

//Main program
int main()
{
	//Set random seed
	seedtime = (unsigned int)time(0);
	randgen.seed(seedtime);

	//Create the window
	sf::RenderWindow window;
	createRenderWindow(window, fullscreen);

	sf::Texture screen;
	screen.create(windowResW, windowResH);

	//Simulation variables
	double t = tstart;
	bool paused = false;
	bool freeze = false;

	//Setup the vertex array
	for (int i = 0; i < pixels.size(); i++) {
		if (color == sf::Color(0, 0, 0, 16) || color.a != coloralpha)
			pixels[i].color = getRandColor(i % pixelCount);
		else
			pixels[i].color = color;
	}

	//Generate initial equations
	randPosNegMultGen();

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
				else if (keycode == sf::Keyboard::F) {
					freeze = !freeze;
				}
				else if (keycode == sf::Keyboard::S) {
					std::ofstream fout("saved.txt", std::ios::app);
					fout << seed << std::endl;
					std::cout << "Saved: " << seed << std::endl;
				}
				//-> = Next
				else if (keycode == sf::Keyboard::Right) {
					resetView(window);
					randPosNegMultGen();
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
			window.display();
			continue;
		}

		//Restart with new equations after t > tend
		if (t > tend) {
			resetView(window);
			randPosNegMultGen();
			t = tstart;
		}

		//Set settings for trail (fadespeed editable)
		sf::BlendMode fade(sf::BlendMode::One, sf::BlendMode::One, sf::BlendMode::ReverseSubtract);
		sf::RenderStates renderBlur(fade);

		sf::RectangleShape fullscreenrect;
		fullscreenrect.setPosition(center.x - windowResW, center.y - windowResH);
		fullscreenrect.setSize(sf::Vector2f(windowResW * 2, windowResH * 2));

		const sf::Uint8 fadespeed = 10;
		fullscreenrect.setFillColor(sf::Color(fadespeed, fadespeed, fadespeed, 0));
		window.draw(fullscreenrect, renderBlur);

		//Move drawing to keep it centered
		if (!freeze) {
			sf::Vector2f nc = calcNewCenter();
			setCenter(window, screen, sf::Vector2f(nc.x, nc.y));
		}

		//Update pixels
		updatePixels(window, t);

		draw(window, screen);
	}

	return 0;
}
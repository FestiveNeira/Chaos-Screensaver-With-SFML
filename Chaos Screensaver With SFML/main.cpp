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
static const int pixelCount = 1000;					//Number of pixel paths
static const int stepsPerFrame = 500;				//Number of iterations of pixels per frame (relates to trail length)
static const int coloralpha = 16;					//Alpha value of colors
static sf::Color color(0, 0, 0, coloralpha);		//Pixel color (use (0,0,0,x) for random or (r,g,b,16) for selected)
static const sf::Vector2f ocenter = { 0.0, 0.0 };	//Starting center
static const int oscaleind = 4;						//Starting scale
static bool usefloatconstants = true;				//Allow decimal constants in equation generation?
static bool usesincos = true;						//Allow sin and cos in function generation?

//Global constants
static const int pixelArrCount = pixelCount * stepsPerFrame;
static const int equn = 18;
static const int equnsc = 8;
static const double stepInterval = 1e-5;
static const double stepSmall = 1e-7;
static const double tstart = -3.0;
static const double tend = 3.0;
static const float scalearr[]{ 0.1, 0.25, 0.5, 0.75, 1.0, 2.5, 5.0, 7.5, 10.0 };

//Global variables
static double windowResW = 1920;
static double windowResH = 1080;
static double windowW = windowResW;
static double windowH = windowResH;
static bool allOffScreen = true;
static double dynamicStepSpeed = stepInterval;
static double speedmult = 1.0;
static std::mt19937 randgen;
static double math[equn];
static double sincos[equnsc];
static std::vector<sf::Vector2f> history(pixelCount);
static std::vector<sf::Vector2f> screenhistory(pixelCount);
static std::vector<sf::Vertex> pixels(pixelArrCount);

static bool moving = false;
sf::Vector2f center = { 0.0, 0.0 };
static int scaleind = oscaleind;

static std::vector<std::string> savedseeds;
static int ssi = 0;
static std::string seed;
static unsigned int seedtime;
static unsigned int seediter = 0;
static unsigned int nextseediter = 0;
static char base26[26] {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};

static void loadSeedDataArray() {
	std::ifstream file;
	file.open("saved.txt");
	if (file.is_open()) {
		std::string line;
		while (std::getline(file, line)) {
			savedseeds.push_back(line);
		}
		file.close();
	}
}
static int getIndex(char b26[], int len, char c) {
	for (int i = 0; i < len; i++) {
		if (b26[i] == c) {
			return i;
		}
	}
	return -1;
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
	unsigned int newseed = 0;
	int pow = 0;
	for (i = 0; i < s.length(); i++) {
		if (s[i] == '-') {
			break;
		}
		int ind = getIndex(base26, 26, s[i]);
		newseed += ind * std::pow(26, pow);
		pow++;
	}
	int val = std::stoi(s.substr(i + 1));
	return sf::Vector2i(newseed, val);
}

static void loadSeed(std::string s) {
	if (s != "") {
		seediter = 0;
		nextseediter = 0;
		sf::Vector2i seedval = decodeSeed(s);
		seedtime = seedval.x;
		randgen.seed(seedval.x);
		std::uniform_real_distribution<float> randfloat(-1, 3);
		for (int i = 0; i < seedval.y; i++) {
			float r = randfloat(randgen);
			nextseediter++;
		}
		seediter = nextseediter;
	}
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
}

//resets zoom and translation at the start of a new simulation
static void resetView(sf::RenderWindow& window) {
	scaleind = oscaleind;
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

	//Initialize window variables
	center = ocenter;
	scaleind = oscaleind;
}

//Converts mathmatical positions of the pixels to screen coordinates
static sf::Vector2f convertPixelToScreen(double x, double y) {
	const float s = scalearr[scaleind] * float(windowResH / 2) * (1.0/8.0);
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
	bool allOnScreen = true;
	bool move = true;
	float minx = FLT_MAX;
	float maxx = -FLT_MAX;
	float miny = FLT_MAX;
	float maxy = -FLT_MAX;
	int ac = 0;
	float ax = 0, ay = 0;
	//Calculate max distance in each direction
	for (int i = 0; i < screenhistory.size(); i++) {
		if (screenhistory[i].x > windowResW * 10 || screenhistory[i].x < -windowResW * 10 || screenhistory[i].y > windowResH * 10 || screenhistory[i].y < -windowResH * 10) {
			move = false;
		}
		ax += screenhistory[i].x;
		ay += screenhistory[i].y;
	}

	if (move) {
		//Calculate new center
		sf::Vector2f centernew(ax / screenhistory.size(), ay / screenhistory.size());

		//Calculate the destination
		sf::Vector2f dest((centernew.x - center.x) * 0.01, (centernew.y - center.y) * 0.01);

		//create a moving global that is set to true when the screen wants to move more than 5 pixels and is set to false when it only wants to move 1
		//only move when moving is true else leave it alone, this will simulate a sort of escape velocity in order to start moving
		if (std::sqrt(std::pow(dest.x, 2) + std::pow(dest.y, 2)) >= 0.5) {
			moving = true;
		}
		else if (std::sqrt(std::pow(dest.x, 2) + std::pow(dest.y, 2)) < 0.1) {
			moving = false;
		}

		//Move 0.01 percent of the distance to the destination
		if (moving) {
			return sf::Vector2f(std::floor(center.x + dest.x), std::floor(center.y + dest.y));
		}
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

static void colorPixels() {
	for (int i = 0; i < pixels.size(); i++) {
		if (color == sf::Color(0, 0, 0, 16) || color.a != coloralpha)
			pixels[i].color = getRandColor(i % pixelCount);
		else
			pixels[i].color = color;
	}
}

//Main program
int main()
{
	//Set random seed
	seedtime = (unsigned int)time(0);
	randgen.seed(seedtime);

	//Load saved seeds
	loadSeedDataArray();

	//Create the window
	sf::RenderWindow window;
	createRenderWindow(window, fullscreen);

	sf::Texture screen;
	screen.create(windowResW, windowResH);

	//Simulation variables
	double t = tstart;
	bool canInteract = false;
	bool paused = false;
	bool freeze = false;
	bool faves = false;

	//Color the vertex array
	colorPixels();

	//Generate initial equations
	randPosNegMultGen();
	seed = encodeSeed();

	//Tracks mouse movement
	int mousepos[] = { -1, -1 };

	while (window.isOpen()) {
		//Event System
		sf::Event event;
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed) {
				window.close();
				break;
			}
			else if (event.type == sf::Event::MouseButtonPressed || event.type == sf::Event::MouseWheelScrolled) {
				if (!canInteract) {
					window.close();
					break;
				}
			}
			else if (event.type == sf::Event::MouseMoved) {
				if ((mousepos[0] < 0 && mousepos[1] < 0) || (event.mouseMove.x == mousepos[0] && event.mouseMove.y == mousepos[1])) {
					mousepos[0] = event.mouseMove.x;
					mousepos[1] = event.mouseMove.y;
				}
				else if (!canInteract) {
					window.close();
					break;
				}
			}
			else if (event.type == sf::Event::KeyPressed) {
				const sf::Keyboard::Key keycode = event.key.code;
				//P = Pause
				if (keycode == sf::Keyboard::P) {
					paused = !paused;
				}
				//F = Prevent Auto-Center (Freeze Camera)
				else if (keycode == sf::Keyboard::F) {
					freeze = !freeze;
				}
				//I = Allows interaction without closing
				else if (keycode == sf::Keyboard::I) {
					canInteract = !canInteract;
				}
				//S = Save seed
				else if (keycode == sf::Keyboard::S) {
					std::ofstream fout("saved.txt", std::ios::app);
					fout << seed << std::endl;
					savedseeds.push_back(seed);
					std::cout << "Saved: " << seed << std::endl;
				}
				//T = Toggle Trig Functions
				else if (keycode == sf::Keyboard::T) {
					usesincos = !usesincos;
				}
				//C = Randomize Color
				else if (keycode == sf::Keyboard::C) {
					std::mt19937 rand((unsigned int)time(0));
					std::uniform_int_distribution<> randint(0, 255);
					color = sf::Color(randint(rand), randint(rand), randint(rand), coloralpha);
					colorPixels();
				}
				//X = Reset Color To Random
				else if (keycode == sf::Keyboard::X) {
					color = sf::Color(0, 0, 0, coloralpha);
					colorPixels();
				}
				//L = Load Saved Functions
				else if (keycode == sf::Keyboard::L) {
					faves = !faves;
				}
				//-> = Next
				else if (keycode == sf::Keyboard::Right) {
					if (faves) {
						loadSeed(savedseeds[ssi]);
						ssi++;
						if (ssi >= savedseeds.size()) {
							ssi = 0;
						}
					}
					resetView(window);
					randPosNegMultGen();
					seed = encodeSeed();
					t = tstart;
				}
				//<- = Restart Current
				else if (keycode == sf::Keyboard::Left) {
					t = tstart;
				}
				//^ = Zoom In
				else if (keycode == sf::Keyboard::Up) {
					if (scaleind < 8) {
						scaleind++;
						center = sf::Vector2f(center.x * (scalearr[scaleind] / scalearr[scaleind - 1]), center.y * (scalearr[scaleind] / scalearr[scaleind - 1]));
						moving = false;
					}
				}
				//v = Zoom Out
				else if (keycode == sf::Keyboard::Down) {
					if (scaleind > 0) {
						scaleind--;
						center = sf::Vector2f(center.x * (scalearr[scaleind] / scalearr[scaleind + 1]), center.y * (scalearr[scaleind] / scalearr[scaleind + 1]));
						moving = false;
					}
				}
				//R = Reset Settings To Default
				else if (keycode == sf::Keyboard::R) {
					paused = false;
					freeze = false;
					usesincos = true;
					color = sf::Color(0, 0, 0, coloralpha);
					colorPixels();
					faves == false;
				}
				//Nothing - Don't Close
				else if (keycode == sf::Keyboard::LShift || keycode == sf::Keyboard::RShift || keycode == sf::Keyboard::Space) {
				}
				//Esc = Close
				else if (keycode == sf::Keyboard::Escape || !canInteract) {
					window.close();
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
			if (faves) {
				loadSeed(savedseeds[ssi]);
				ssi++;
				if (ssi >= savedseeds.size()) {
					ssi = 0;
				}
			}
			resetView(window);
			randPosNegMultGen();
			seed = encodeSeed();
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
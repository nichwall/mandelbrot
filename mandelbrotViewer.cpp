#include "mandelbrotViewer.h"
#include <string.h>
#include <iostream>
#include <iomanip>
#include <math.h>
#include <sstream>
#include <thread>
#include <ctime>

# define PI 3.14159265358979323846

//initialize a couple of global objects
sf::Mutex mutex1;
sf::Mutex mutex2;

//Constructor
MandelbrotViewer::MandelbrotViewer(int res) {
    resolution = res;

    //create the window and view, then give them to the pointers
    static sf::RenderWindow win(sf::VideoMode(resolution, resolution), "Mandelbrot Explorer");
    static sf::View vw(sf::FloatRect(0, 0, resolution, resolution));
    window = &win;
    view = &vw;

    //initialize the viewport
    view->setViewport(sf::FloatRect(0, 0, 1, 1));
    window->setView(*view);
    
    //cap the framerate
    framerateLimit = 60;
    window->setFramerateLimit(framerateLimit);

    //initialize the mandelbrot parameters
    resetMandelbrot();

    //initialize the image
    texture.create(resolution, resolution);
    image.create(resolution, resolution, sf::Color::Black);
    sprite.setTexture(texture);
    scheme = 1;
    initPalette(); 

    //initialize the font for the overlay
	if (font.loadFromFile("cour.ttf"));
	else if (font.loadFromFile("C:\\Windows\\Fonts\\cour.ttf"));
	else std::cout << "ERROR: unable to load font\n";

    //initialize the image_array
    size_t size = resolution;
    std::vector< std::vector<int> > array(size, std::vector<int>(size));
    image_array = array;

    //get the number of supported concurrent threads
    max_threads = std::thread::hardware_concurrency();

    //disable repeated keys
    window->setKeyRepeatEnabled(false);
    rotation = 0;
}

MandelbrotViewer::~MandelbrotViewer() { }

//Accessors
sf::Vector2i MandelbrotViewer::getMousePosition() {
    return sf::Mouse::getPosition(*window);
}

//return the center of the area of the complex plane
sf::Vector2f MandelbrotViewer::getMandelbrotCenter() {
    sf::Vector2f center;
    center.x = area.left + area.width/2.0;
    center.y = area.top + area.height/2.0;
    return center;
}

//gets the next event from the viewer
bool MandelbrotViewer::getEvent(sf::Event& event) {
    return window->waitEvent(event);
}

//checks if the window is open
bool MandelbrotViewer::isOpen() {
    return window->isOpen();
}

//this is a setter function to change the color scheme
//it also handles all the regeneration and refreshing
void MandelbrotViewer::setColorScheme(int newScheme) {
    scheme = newScheme;
    initPalette();
    changeColor();
    updateMandelbrot();
    refreshWindow();
}

//sets the rotation and regenerates the mandelbrot
void MandelbrotViewer::setRotation(double radians) {
    rotation = radians;
    if (rotation >= 2 * PI) rotation -= 2 * PI;
    else if (rotation < 0) rotation += 2 * PI;
    generate();
    resetView();
    updateMandelbrot();
    refreshWindow();
}

//Functions to change parameters of mandelbrot

//regenerates the image with the new color multiplier, without regenerating
//the mandelbrot. Does not update the image (use updateImage())
void MandelbrotViewer::changeColor() {
    for (int i=0; i<resolution; i++) {
        for (int j=0; j<resolution; j++) {
            image.setPixel(j, i, findColor(image_array[i][j]));
        }
    }
}

//changes the parameters of the mandelbrot: sets new center and zooms accordingly
//does not regenerate or update the image
void MandelbrotViewer::changePos(sf::Vector2<double> new_center, double zoom_factor) {

    //rotate the mouse input first
    new_center = rotate(new_center);

    area.width = area.width * zoom_factor;
    area.height = area.height * zoom_factor;
    area.left = new_center.x - area.width / 2.0;
    area.top = new_center.y - area.height / 2.0;
    //NOTE: this is a relative zoom
}

//similar to changePos, but it's an absolute zoom and it only changes the view
//instead of setting new parameters to regenerate the mandelbrot
void MandelbrotViewer::changePosView(sf::Vector2f new_center, double zoom_factor) {
    //reset the view so that we can apply an absolute zoom, instead of relative
    resetView();

    //set new center and zoom
    view->setCenter(new_center);
    view->zoom(zoom_factor);

    //reload the new view
    window->setView(*view);
}

//generate the mandelbrot
void MandelbrotViewer::generate() {

    //make sure it starts at line 0
    nextLine = 0;

    //create the thread pool
    std::vector<std::thread> threadPool;
    for (int i=0; i<max_threads; i++) {
        threadPool.push_back(std::thread(&MandelbrotViewer::genLine, this));
    }

    //join the threads
    for (int i=0; i<max_threads; i++) {
        threadPool[i].join();
    }

    //reset last_max_iter to the new max_iter
    last_max_iter = max_iter;
}

//this is a private worker thread function. Each thread picks the next ungenerated
//row of pixels, generates it, then starts the next one
void MandelbrotViewer::genLine() {

    int iter, row, column;
    sf::Vector2<double> point;
    double x_inc = interpolate(area.width, resolution);
    double y_inc = interpolate(area.height, resolution);
    sf::Color color;

    while(true) {

        //the mutex avoids multiple threads writing to variables at the same time,
        //which can corrupt the data
        mutex1.lock();
        row = nextLine++; //get the next ungenerated line
        mutex1.unlock();

        //if all the rows have been generated, stop it from generating outside the bounds
        //of the image
        if (row >= resolution) return;

        //calculate the row height in the complex plane
        point.y = area.top + row * y_inc;

        //now loop through and generate all the pixels in that row
        for (column = 0; column < resolution; column++) {

            // Check if we increased iterations and if the pixel already diverged
            if ( last_max_iter < max_iter && image_array[row][column] < last_max_iter ) {
                iter = image_array[row][column];
            } // Check if we decreased iterations and if the pixel already converged
            else if ( last_max_iter > max_iter && image_array[row][column] > max_iter) {
                iter = image_array[row][column];
            } // Check if we zoomed, or didn't change iterations which means we need to recalculate the whole thing
            else {
                //calculate the next x coordinate of the complex plane
                point.x = area.left + column * x_inc;
                iter = escape(point);
            }

            //mutex this too so that the image is not accessed multiple times simultaneously
            mutex2.lock();
            image.setPixel(column, row, findColor(iter));
            image_array[row][column] = iter;
            mutex2.unlock();
        }
    }
}

//Reset/update functions:

//resets the mandelbrot to generate the starting area
void MandelbrotViewer::resetMandelbrot() {
    area.left = -1.5;
    area.top = -1.0;
    area.width = 2;
    area.height = 2;
    max_iter = 100;
    last_max_iter = 100;
    color_multiple = 1;
    rotation = 0;
}

//refreshes the window: clear, draw, display
void MandelbrotViewer::refreshWindow() {
    window->clear(sf::Color::Black);
    window->setView(*view);
    window->draw(sprite);
    window->display();
}

//reset the view to display the entire image
void MandelbrotViewer::resetView() {
    view->reset(sf::FloatRect(0, 0, resolution, resolution));
    window->setView(*view);
}

//close the window
void MandelbrotViewer::close() {
    window->close();
}

//update the mandelbrot image (use the already generated image to update the
//texture, so the next time the screen updates it will be displayed
void MandelbrotViewer::updateMandelbrot() {
    texture.update(image);
}

void MandelbrotViewer::setWindowActive(bool setting) {
    window->setActive(setting);
}

//saves the currently displayed image to a png with a timestamp in the title
void MandelbrotViewer::saveImage() {
    //set up the timestamp filename
    time_t currentTime = time(0);
    tm* currentDate = localtime(&currentTime);
    char filename[80];
    strftime(filename,80,"%Y-%m-%d.%H-%M-%S",currentDate);
    strcat(filename, ".png");

    //save the image and print confirmation
    image.saveToFile(filename);
    std::cout << "Saved image to " << filename << std::endl;
}

//enables an overlay that dims the screen and displays controls/stats/etc.
void MandelbrotViewer::enableOverlay(bool enable) {
    double angle = rotation * 180 / PI;
    if (angle > 180) angle -= 360;
    sf::Text controls;
    sf::Text stats;
    if (enable) {
        //set up the controls part
        controls.setFont(font);
        controls.setString("                 Help Menu (H)\n"
                        "Controls\n"
                        "------------------------------------------------\n"
                        "Left/Right arrows - Change colors\n"
                        "Up/Down arrows    - Increase/decrease iterations\n"
                        "Click and Drag    - Move around\n"
                        "Numbers 1-7       - Change color scheme\n"
                        "Scroll            - Zoom in/out\n"
                        "H                 - Help menu\n"
                        "S                 - Save image\n"
                        "R                 - Reset\n"
                        "Q                 - Quit\n"
                        "Page up           - Rotate counter-clockwise\n"
                        "Page down         - Rotate clockwise\n"
                        "Home              - Reset rotation\n"
                        "------------------------------------------------\n");
        controls.setCharacterSize(24);
        controls.setColor(sf::Color::White);
        controls.setPosition(40, 30);

        //set up the stats part
        std::stringstream ss;
        ss << std::fixed << std::setprecision(20);
        ss << "Resolution: " << resolution << "x" << resolution << "\n\n";
        ss << "Coordinates: \n";
        ss << "x: " << std::setw(23) << area.left << "  y: " << std::setw(23) << area.top << "\n";
        ss << "   " << std::setw(23) << area.left + area.width << "     " << std::setw(23) << area.top + area.height;
        ss << std::defaultfloat;
        int zoom_level = log2(2.0/area.width);
        ss << "\n\nZoom level: " << zoom_level;
		ss << "\n\nIterations: " << max_iter << std::fixed << std::setprecision(0);
        ss << "\n\nRotation: " << angle << " degrees";

        stats.setFont(font);
        stats.setString(ss.str());
        stats.setCharacterSize(24);
        stats.setPosition(40, 475);

        //set up the screen fade
        sf::RectangleShape rectangle;
        rectangle.setSize(sf::Vector2f(resolution, resolution));
        rectangle.setFillColor(sf::Color(0, 0, 0, 192));
        rectangle.setPosition(0, 0);

        //draw to the screen
        window->draw(sprite);
        window->draw(rectangle);
        window->draw(controls);
        window->draw(stats);
        window->display();
    } else {
        refreshWindow();
    }
}

//rotates the view relative to its current rotation
void MandelbrotViewer::rotateView(float angle) {
    view->setRotation(angle);
}

//Converts a vector from pixel coordinates to the corresponding
//coordinates on the complex plane
sf::Vector2<double> MandelbrotViewer::pixelToComplex(sf::Vector2f pix) {
    sf::Vector2<double> comp;
    comp.x = area.left + pix.x * interpolate(area.width, resolution);
    comp.y = area.top + pix.y * interpolate(area.height, resolution);
    return comp;
}

//this function calculates the escape-time of the given coordinate
//it is the brain of the mandelbrot program: it does the work to
//make the pretty pictures :)
int MandelbrotViewer::escape(sf::Vector2<double> point) {
    
    //rotate the point first
    point = rotate(point);

    double x = 0, y = 0, x_check = 0, y_check = 0;
    int iter = 0, period = 2;

    double x_square = 0;
    double y_square = 0;

    //this is a specialized version of z = z^2 + c. It only does three multiplications,
    //instead of the normal six. Multplications are very costly with such high precision
    while(period < max_iter) {
        x_check = x;
        y_check = y;
        period += period;

        if (period > max_iter) period = max_iter;
        for (; iter < period; iter++) {
            y = x * y;
            y += y; //multiply by two
            y += point.y;
            x = x_square - y_square + point.x;

            x_square = x*x;
            y_square = y*y;

            //if the magnitued is greater than 2, it will escape
            if (x_square + y_square > 4.0) return iter;

            //another optimization: it checks if the new 'z' is a repeat. If so,
            //it knows that it is in a loop and will not escape
            if ((x == x_check) && (y == y_check)){
                return max_iter;
            }
        }
    }
    return max_iter;
}

//findColor uses the number of iterations passed to it to look up a color in the palette
sf::Color MandelbrotViewer::findColor(int iter) {
    int i = fmod(iter * color_multiple, 255);
    sf::Color color;
    if (iter >= max_iter) color = sf::Color::Black;
    else {
        color.r = palette[0][i];
        color.g = palette[1][i];
        color.b = palette[2][i];
    }
    return color;
}

//this function handles rotation - it takes in a complex point with zero rotation
//and returns where that point is when rotated
sf::Vector2<double> MandelbrotViewer::rotate(sf::Vector2<double> rect) {

    //get some vectors ready
    sf::Vector2<double> polar;
    sf::Vector2<double> difference;
    sf::Vector2<double> center;

    //the function needs to use the given rectangular coordinates to generate a new vector
    //with the origin at the current center of the complex plane, convert that vector to
    //polar coordinates, convert it back to rectangular coordinates, and re-normalize it.

    //get the center of the complex plane in the viewer
    center.x = area.left + area.width/2.0;
    center.y = area.top + area.height/2.0;

    //subract the given point from the center, to get a vector with the center as the origin
    difference = rect - center;

    //convert that new vector to polar coordinates
    polar.x = hypot(difference.x, difference.y);
    polar.y = atan2(difference.y, difference.x);

    //rotate the polar vector
    polar.y += rotation;

    //convert back to rectangular
    difference.x = polar.x * cos(polar.y);
    difference.y = polar.x * sin(polar.y);

    //now put the center back where it belongs
    rect = center + difference;

    return rect;
}

//This is for initPalette, it makes sure the given number is between 0 and 255
int coerce(int number) {
    if (number > 255) number = 255;
    else if (number < 0) number = 0;
    return number;
}

//Sets up the palette array
void MandelbrotViewer::initPalette() {
    //define some non-standard colors
    sf::Color orange;
    orange.r = 255;
    orange.g = 165;
    orange.b = 0;
    sf::Color pink;
    pink.r = 255;
    pink.g = 135;
    pink.b = 135;
    sf::Color dark_red;
    dark_red.r = 209;
    dark_red.g = 2;
    dark_red.b = 2;
    sf::Color dark_green;
    dark_green.r = 8;
    dark_green.g = 172;
    dark_green.b = 8;
    sf::Color light_green;
    light_green.r = 115;
    light_green.g = 255;
    light_green.b = 115;
    switch (scheme) {
        //scheme one is black:blue:white:orange:black
        case 1:
            smoosh(sf::Color::Black, sf::Color::Blue, 0, 64);
            smoosh(sf::Color::Blue, sf::Color::White, 64, 144);
            smoosh(sf::Color::White, orange, 144, 196);
            smoosh(orange, sf::Color::Black, 196, 256);
            break;
        //scheme two is black:green:blue:black
        case 2:
            smoosh(sf::Color::Black, sf::Color::Green, 0, 85);
            smoosh(sf::Color::Green, sf::Color::Blue, 85, 170);
            smoosh(sf::Color::Blue, sf::Color::Black, 170, 256);
            break;
        //scheme three is black:red:orange:black
        case 3:
            smoosh(sf::Color::Black, sf::Color::Red, 0, 180);
            smoosh(sf::Color::Red, orange, 180, 215);
            smoosh(orange, sf::Color::Black, 215, 256);
            break;
        //scheme four is black:cyan:white:black
        case 4:
            smoosh(sf::Color::Black, sf::Color::Cyan, 0, 110);
            smoosh(sf::Color::Cyan, sf::Color::White, 110, 220);
            smoosh(sf::Color::White, sf::Color::Black, 220, 256);
            break;
        //scheme five is red:orange:yellow:green:blue:magenta:red
        case 5:
            smoosh(sf::Color::Red, orange, 0, 42);
            smoosh(orange, sf::Color::Yellow, 42, 84);
            smoosh(sf::Color::Yellow, sf::Color::Green, 84, 127);
            smoosh(sf::Color::Green, sf::Color::Blue, 127, 170);
            smoosh(sf::Color::Blue, sf::Color::Magenta, 170, 212);
            smoosh(sf::Color::Magenta, sf::Color::Red, 212, 256);
            break;
        //scheme six is dark_red:pink, then dark_green:light_green
        case 6:
            smoosh(dark_red, pink, 0, 128);
            smoosh(dark_green, light_green, 128, 256);
            break;
        //scheme seven is smoky... black:white
        case 7:
            smoosh(sf::Color::White, sf::Color::Black, 0, 256);
            break;
        default:
            int r, g, b;
            for (int i = 0; i <= 255; i++) {
                r = (int) (23.45 - 1.880*i + 0.0461*i*i - 0.000152*i*i*i);
                g = (int) (17.30 - 0.417*i + 0.0273*i*i - 0.000101*i*i*i);
                b = (int) (25.22 + 7.902*i - 0.0681*i*i + 0.000145*i*i*i);

                palette[0][i] = coerce(r);
                palette[1][i] = coerce(g);
                palette[2][i] = coerce(b);
            }
    }
}

//Smooshes two colors together, and writes them to the palette in the specified range
void MandelbrotViewer::smoosh(sf::Color c1, sf::Color c2, int min, int max) {
    int range = max-min;
    float r_inc = interpolate(c1.r, c2.r, range);
    float g_inc = interpolate(c1.g, c2.g, range);
    float b_inc = interpolate(c1.b, c2.b, range);

    //loop through the palette setting new colors
    for (int i=0; i < range; i++) {
        palette[0][min+i] = (int) (c1.r + i * r_inc);
        palette[1][min+i] = (int) (c1.g + i * g_inc);
        palette[2][min+i] = (int) (c1.b + i * b_inc);
    }
}

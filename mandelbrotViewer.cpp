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
MandelbrotViewer::MandelbrotViewer(int resX, int resY) {
    res_width = resX;
    res_height = resY;

    //create the window and view, then give them to the pointers
    static sf::RenderWindow win(sf::VideoMode(res_width, res_height), "Mandelbrot Explorer");
    static sf::View vw(sf::FloatRect(0, 0, res_width, res_height));
    window = &win;
    view = &vw;

    //initialize the viewport. It should never change
    view->setViewport(sf::FloatRect(0, 0, 1, 1));
    window->setView(*view);
    
    //cap the framerate
    framerateLimit = 60;
    window->setFramerateLimit(framerateLimit);

    //initialize the image
    texture.create(res_width, res_height);
    image.create(res_width, res_height, sf::Color::White);
    sprite.setTexture(texture);
    scheme = 1;
    
    //initialize the color palette
    color_locked = false;
    std::vector<int> pal_row(max_iter.load());
    palette.push_back(pal_row);
    palette.push_back(pal_row);
    palette.push_back(pal_row);

    //initialize the mandelbrot parameters
    resetMandelbrot();

    //initialize the font for the overlay
	if (font.loadFromFile("cour.ttf"));
	else if (font.loadFromFile("C:\\Windows\\Fonts\\cour.ttf"));
	else std::cout << "ERROR: unable to load font\n";

    //initialize the image_array
    size_t sizeX = res_width;
    size_t sizeY = res_height;
    std::vector< std::vector<int> > array(sizeY, std::vector<int>(sizeX));
    image_array = array;

    //get the number of supported concurrent threads
    // TODO change this back
    max_threads = std::thread::hardware_concurrency();
    //max_threads = 1;

    //disable repeated keys
    //window->setKeyRepeatEnabled(false);

    rotation = 0;
}

MandelbrotViewer::~MandelbrotViewer() { }

//random useful functions

//reset all elements of a 2D vector to 0
//uses template so it's a bit easier for custom vectors
template <typename T>
inline void zeroVector2(std::vector< std::vector<T> > &v, T const& zero) {
    for (unsigned int i=0; i<v.size(); i++) {
        std::fill(v[i].begin(), v[i].end(), zero);
    }
}
//Mutexed vector functions

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

//wait for and return the next event from the viewer
bool MandelbrotViewer::waitEvent(sf::Event& event) {
    return window->waitEvent(event);
}

//poll for events from the viewer
bool MandelbrotViewer::pollEvent(sf::Event& event) {
    return window->pollEvent(event);
}

//checks if the window is open
bool MandelbrotViewer::isOpen() {
    return window->isOpen();
}

void MandelbrotViewer::incIterations() {
    //if iterations is in the hundreds, add 100
    //if iterations is in the thousands, add 1000, etc.
    int magnitude = (int) log10(temp_max_iter.load());
    unsigned int inc = pow(10, magnitude);
    temp_max_iter.fetch_add(inc);
}

void MandelbrotViewer::decIterations() {
    //if iterations is in the hundreds, subtract 100
    //if iterations is in the thousands, subtract 1000, etc.
    if (temp_max_iter.load() > 100) {
        int magnitude = (int) log10(temp_max_iter.load());
        unsigned int dec = pow(10, magnitude);
        if (dec == temp_max_iter.load()) dec /= 10;
        temp_max_iter.fetch_sub(dec);
    }
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

void MandelbrotViewer::lockColor() {
    if (color_locked) {
        color_locked = false;
        initPalette();
        changeColor();
        updateMandelbrot();
        refreshWindow();
    } else {
        color_locked = true;
    }
}

//Functions to change parameters of mandelbrot

//regenerates the image with the new color multiplier, without regenerating
//the mandelbrot. Does not update the image (use updateImage())
void MandelbrotViewer::changeColor() {
    for (int i=0; i<res_height; i++) {
        for (int j=0; j<res_width; j++) {
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
    area_inc = area.width/res_width;
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
}

//handle resize events by modifying the area rectangle accordingly
void MandelbrotViewer::resizeWindow(int new_x, int new_y) {

    //save the old center and resolution
    double center_x = area.left + area.width/2.0;
    double center_y = area.top + area.height/2.0;

    res_width = new_x;
    res_height = new_y;

    //calculate the new area
    area.width = area_inc * res_width;
    area.height = area_inc * res_height;
    area.left = center_x - area.width/2.0;
    area.top = center_y - area.height/2.0;
    area_inc = area.width/res_width;

    //resize the image, texture, and sprite
    image.create(res_width, res_height, sf::Color::Black);
    texture.create(res_width, res_height);
    sprite.setTextureRect(sf::IntRect(0, 0, res_width, res_height));
    sprite.setTexture(texture);

    //resize the image_array
    size_t sizeX = res_width;
    size_t sizeY = res_height;
    std::vector< std::vector<int> > array(sizeY, std::vector<int>(sizeX));
    image_array = array;

    resetView();
}

//generate the mandelbrot
void MandelbrotViewer::generate() {

    quadtree_master();
    return;

    bool done = false;
    restart_gen = false;

    while (!done) {
        //make sure it starts at line 0
        nextLine = 0;

        //create and launch the thread pool
        std::vector<std::thread> threadPool;
        for (unsigned int i=0; i<max_threads; i++) {
            threadPool.push_back(std::thread(&MandelbrotViewer::genLine, this));
        }

        //wait for the threads to finish
        for (unsigned int i=0; i<max_threads; i++) {
            threadPool[i].join();
        }

        //when the threads finish prematurely, reset needed variables and restart generation
        if (restart_gen) {
            done = false;
            restart_gen = false;
        } else done = true;
    }

    //reset last_max_iter to the new max_iter
    last_max_iter.store(max_iter.load());
}

//this is a private worker thread function. Each thread picks the next ungenerated
//row of pixels, generates it, then starts the next one
void MandelbrotViewer::genLine() {

    int iter, row, column;
    sf::Vector2<double> point;
    sf::Color color;

    while(true) {

        //the mutex avoids multiple threads writing to variables at the same time,
        //which can corrupt the data
        mutex1.lock();
        row = nextLine++; //get the next ungenerated line
        mutex1.unlock();

        //return when it finishes the last row
        if (row >= res_height) break;

        for (column = 0; column < res_width; column++) {
            iter = escape(row, column);

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
    area.top = -1;
    area.height = 2;
    area_inc = area.height/res_height;
    area.width = area_inc * res_width;
    area.left = -0.5 - area.width/2.0;

    max_iter.store(100);
    last_max_iter.store(100);
    temp_max_iter.store(100);
    color_multiple = 1;
    rotation = 0;
    color_locked = false;
    initPalette();
}

//refreshes the window: clear, draw, display
void MandelbrotViewer::refreshWindow() {
    window->clear(sf::Color::White);
    window->setView(*view);
    window->draw(sprite);
    window->display();
}

//reset the view to display the entire image
void MandelbrotViewer::resetView() {
    view->reset(sf::FloatRect(0, 0, res_width, res_height));
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
                        "L                 - Lock Colors\n"
                        "Q                 - Quit\n"
                        "Page up           - Rotate counter-clockwise\n"
                        "Page down         - Rotate clockwise\n"
                        "Home              - Reset rotation\n"
                        "------------------------------------------------\n");
        controls.setCharacterSize(24);
        controls.setFillColor(sf::Color::White);
        controls.setPosition(40, 20);

        //set up the stats part
        std::stringstream ss;
        ss << std::fixed << std::setprecision(20);
        ss << "Resolution: " << res_width << "x" << res_height << "\n\n";
        ss << "Coordinates: \n";
        ss << "x: " << std::setw(23) << area.left << "  y: " << std::setw(23) << area.top << "\n";
        ss << "   " << std::setw(23) << area.left + area.width << "     " << std::setw(23) << area.top + area.height;
        ss << std::defaultfloat;
        int zoom_level = log2(2.0/area.width);
        ss << "\n\nZoom level: " << zoom_level;
        if (color_locked)
            ss << "\t\t\t\t\tColor is locked";
        else
            ss << "\t\t\t\t\tColor is unlocked";
		ss << "\n\nIterations: " << max_iter.load() << std::fixed << std::setprecision(0);
        ss << "\n\nRotation: " << angle << " degrees";

        stats.setFont(font);
        stats.setString(ss.str());
        stats.setCharacterSize(24);
        stats.setPosition(40, 485);

        //set up the screen fade
        sf::RectangleShape rectangle;
        rectangle.setSize(sf::Vector2f(res_width, res_height));
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
    comp.x = area.left + pix.x * area_inc;
    comp.y = area.top + pix.y * area_inc;
    return comp;
}

//this function calculates the escape-time of the given coordinate
//it is the brain of the mandelbrot program: it does the work to
//make the pretty pictures :)
int MandelbrotViewer::escape(int row, int column) {

    //check if we increased iterations and if the pixel already diverged
    if (last_max_iter.load() < max_iter.load() && image_array[row][column] < last_max_iter.load())
        return image_array[row][column];
    //check if we decreased iterations and if the pixel already converged
    else if (last_max_iter.load() > max_iter.load() && image_array[row][column] > max_iter.load())
        return image_array[row][column];
    //if not, use the escape-time algorithm to calculate iter
    else {

        //convert from pixel to complex coordinates
        sf::Vector2f pnt(column, row);
        sf::Vector2<double> point = pixelToComplex(pnt);
        //        printf("Point: (%-2.10f,%-2.10f)\t",point.x,point.y);

        //rotate the point
        if (rotation) point = rotate(point);

        double x = 0, y = 0;
        unsigned int iter = 0;

        double x_square = 0;
        double y_square = 0;

        //this is a specialized version of z = z^2 + c. It only does three multiplications,
        //instead of the normal six. Multplications are very costly with such high precision
        for (; iter < max_iter.load(); iter++) {
            y = x * y;
            y += y; //multiply by two
            y += point.y;
            x = x_square - y_square + point.x;

            x_square = x*x;
            y_square = y*y;

            //if the magnitude is greater than 2, it will escape
            if (x_square + y_square > 4.0) return iter;
        }
    }
    return max_iter.load();
}

//findColor uses the number of iterations passed to it to look up a color in the palette
sf::Color MandelbrotViewer::findColor(unsigned int iter) {
    int i = (int) fmod(iter * color_multiple, palette[0].size());
    sf::Color color;
    if (iter >= max_iter.load()) color = sf::Color::Black;
    else if (iter == 0) {
        color = sf::Color::White;
    } else {
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

//Sets up the palette array
void MandelbrotViewer::initPalette() {

    //if the color is locked, it shouldn't resize the palette
    //(that would change the color scale)
    if (!color_locked) {
        palette[0].resize(max_iter.load());
        palette[1].resize(max_iter.load());
        palette[2].resize(max_iter.load());
    }

    //define some non-standard colors
    sf::Color orange;
    orange.r = 255;
    orange.g = 165;
    orange.b = 0;

    switch (scheme) {
        //scheme one is black:blue:white:orange:black
        case 1:
            smoosh(sf::Color::Black, sf::Color::Blue, 0, 0.25);
            smoosh(sf::Color::Blue, sf::Color::White, 0.25, 0.56);
            smoosh(sf::Color::White, orange, 0.56, 0.75);
            smoosh(orange, sf::Color::Black, 0.75, 1);
            break;
        //scheme two is black:red:orange:black
        case 2:
            smoosh(sf::Color::Black, sf::Color::Red, 0, 0.7);
            smoosh(sf::Color::Red, orange, 0.7, 0.84);
            smoosh(orange, sf::Color::Black, 0.84, 1);
            break;
        //scheme three is black:cyan:white:black
        case 3:
            smoosh(sf::Color::Black, sf::Color::Cyan, 0, 0.43);
            smoosh(sf::Color::Cyan, sf::Color::White, 0.43, 0.86);
            smoosh(sf::Color::White, sf::Color::Black, 0.86, 1);
            break;
        //scheme four is red:orange:yellow:green:blue:magenta:red
        case 4:
            smoosh(sf::Color::Red, orange, 0, 0.17);
            smoosh(orange, sf::Color::Yellow, 0.17, 0.33);
            smoosh(sf::Color::Yellow, sf::Color::Green, 0.33, 0.5);
            smoosh(sf::Color::Green, sf::Color::Blue, 0.5, 0.67);
            smoosh(sf::Color::Blue, sf::Color::Magenta, 0.67, 0.83);
            smoosh(sf::Color::Magenta, sf::Color::Red, 0.83, 1);
            break;
        //scheme five is black:white
        case 5:
            smoosh(sf::Color::White, sf::Color::Black, 0, 1);
            break;
    }
}

//Smooshes two colors together, and writes them to the palette in the specified range
void MandelbrotViewer::smoosh(sf::Color c1, sf::Color c2, float min_per, float max_per) {
    int min = (int) (min_per * palette[0].size());
    int max = (int) (max_per * palette[0].size());
    int range = max-min;

    double r_inc = interpolate(c1.r, c2.r, range);
    double g_inc = interpolate(c1.g, c2.g, range);
    double b_inc = interpolate(c1.b, c2.b, range);

    //loop through the palette setting new colors
    for (int i=0; i < range; i++) {
        palette[0][min+i] = (int) (c1.r + i * r_inc);
        palette[1][min+i] = (int) (c1.g + i * g_inc);
        palette[2][min+i] = (int) (c1.b + i * b_inc);
    }
}

// N Stuff
template <typename T>
    inline void MandelbrotViewer::vector_put(std::vector<T> &r_vector, std::mutex &r_mutex, const T &r_value) {
        r_mutex.lock();
        r_vector.push_back(r_value);
        r_mutex.unlock();
    }
template <typename T>
    inline T MandelbrotViewer::vector_get(std::vector<T> &r_vector, std::mutex &r_mutex) {
        r_mutex.lock();
        if (r_vector.size() != 0) {
            T ret = r_vector[r_vector.size()-1];
            r_vector.pop_back();
            r_mutex.unlock();
            return ret;
        }
    }
template <typename T>
    inline T MandelbrotViewer::vector_get(std::vector<T> &r_vector, std::mutex &r_mutex, bool &r_failed) {
        r_mutex.lock();
        if (r_vector.size() != 0) {
            T ret = r_vector[r_vector.size()-1];
            r_vector.pop_back();
            r_mutex.unlock();
            r_failed = false;
            return ret;
        } else {
            r_failed = true;
        }
    }
template <typename T>
    inline int MandelbrotViewer::vector_size(std::vector<T> &r_vector, std::mutex &r_mutex) {
        r_mutex.lock();
        int size = r_vector.size();
        r_mutex.unlock();
        return size;
    }
void MandelbrotViewer::quadtree_createOutsideImage() {
    // Generate horizontal lines of image
    int iter1, iter2;
    for (int i=0; i<res_width; i++) {
        iter1 = escape(0, i);
        image.setPixel(i, 0, findColor(iter1));
        image_array[0][i] = iter1;

        iter2 = escape(res_height-1, i);
        image.setPixel(i, res_height-1, findColor(iter2));
        image_array[res_height-1][i] = iter2;
    }
    // Generate vertical lines of image
    for (int i=1; i<res_height-1; i++) {
        iter1 = escape(i, 0);
        image.setPixel(0, i, findColor(iter1));
        image_array[i][0] = iter1;

        iter2 = escape(i, res_width-1);
        image.setPixel(res_width-1, i, findColor(iter2));
        image_array[i][res_width-1] = iter2;
    }

    // Create first square to check
    Square firstSquare; // Outer edge of image
    firstSquare.min_x = 0;
    firstSquare.min_y = 0;
    firstSquare.max_x = res_width-1;
    firstSquare.max_y = res_height-1;
    squaresToCheck.push_back(firstSquare);
}
bool MandelbrotViewer::quadtree_masterDone() {
    mutex_squaresToSplit.lock();
    mutex_plusToWrite.lock();
    if (squaresToCheck.size() != 0 || squaresToWrite.size() != 0 || numberOfThreads.load() != 0) {
        mutex_squaresToSplit.unlock();
        mutex_plusToWrite.unlock();
        return false;
    }
    bool ret = (squaresToSplit.size() == 0 && plusToWrite.size() == 0);
    mutex_squaresToSplit.unlock();
    mutex_plusToWrite.unlock();
    return ret;
}
void MandelbrotViewer::quadtree_writePlus(Plus &r_plus) {
    // Write the vertical line
    for (unsigned int i=0; i<r_plus.vertical.size(); i++) {
        image_array[r_plus.min_y+i+1][r_plus.mid_x] = r_plus.vertical[i];
    }
    // Write the horizontal line
    for (unsigned int i=0; i<r_plus.horizontal.size(); i++) {
        image_array[r_plus.mid_y][r_plus.min_x+i+1] = r_plus.horizontal[i];
    }

    // Create the new squares to check
    Square temp = r_plus;
    
    // Top left
    temp.max_x = r_plus.mid_x;
    temp.max_y = r_plus.mid_y;
    squaresToCheck.push_back(temp);
    // Bottom left
    temp.min_y = r_plus.mid_y;
    temp.max_y = r_plus.max_y;
    squaresToCheck.push_back(temp);
    // Bottom right
    temp.min_x = r_plus.mid_x;
    temp.max_x = r_plus.max_x;
    squaresToCheck.push_back(temp);
    // Top Right
    temp.min_y = r_plus.min_y;
    temp.max_y = r_plus.mid_y;
    squaresToCheck.push_back(temp);
}
void MandelbrotViewer::quadtree_checkSquare(Square &r_square) {
    // Check whether too small
    if (r_square.max_x - r_square.min_x < 2)
        return;
    if (r_square.max_y - r_square.min_y < 2)
        return;

    bool toSplit = false;
    int iterCount = image_array[r_square.min_y][r_square.min_x];
    // Check horizontal borders
    for (unsigned int i=r_square.min_x; i<=r_square.max_x; i++) {
        if (image_array[r_square.min_y][i] != iterCount || image_array[r_square.max_y][i] != iterCount) {
            toSplit = true;
            break;
        }
    }
    // If didn't fail already, check vertical borders
    if (!toSplit) {
        for (unsigned int i=r_square.min_y+1; i<r_square.max_y; i++) {
            if (image_array[i][r_square.min_x] != iterCount || image_array[i][r_square.max_x] != iterCount) {
                toSplit = true;
                break;
            }
        }
    }

    // If we need to split, put in squaresToSplit
    if (toSplit)
        vector_put(squaresToSplit, mutex_squaresToSplit, r_square);
    else
        squaresToWrite.push_back(r_square);
}
void MandelbrotViewer::quadtree_writeSquare(Square &r_square) {
    //int iterCount = image_array[r_square.min_y][r_square.min_x];
    for (unsigned int i=r_square.min_y+1; i<r_square.max_y; i++) {
        for (unsigned int j=r_square.min_x+1; j<r_square.max_x; j++) {
            //image_array[i][j] = iterCount;
            image_array[i][j] = 0;
        }
    }
}
void MandelbrotViewer::quadtree_splitSquare(Square &r_square) {
    // Create initial plus
    Plus plus;
    plus.min_x = r_square.min_x;
    plus.max_x = r_square.max_x;
    plus.mid_x = (plus.max_x+plus.min_x)/2;
    plus.min_y = r_square.min_y;
    plus.max_y = r_square.max_y;
    plus.mid_y = (plus.max_y+plus.min_y)/2;

    // Create the vectors of the points
    // Vertical
    for (unsigned int i = plus.min_y+1; i < plus.max_y; i++) {
        plus.vertical.push_back(escape(i, plus.mid_x));
    }
    // Horizontal
    for (unsigned int i = plus.min_x+1; i < plus.max_x; i++) {
        plus.horizontal.push_back(escape(plus.mid_y, i));
    }

    vector_put(plusToWrite, mutex_plusToWrite, plus);
}
void MandelbrotViewer::quadtree_master() {
    // Read out what the iteration count should be
    unsigned int temp = temp_max_iter.load();
    if (temp != max_iter.load()) {
        max_iter.store(temp);
        initPalette();
    }
    printf("Starting generate at iteration: %u\n",max_iter.load());
    // Zero all the working variables
    plusToWrite.clear();
    squaresToWrite.clear();
    squaresToCheck.clear();
    squaresToSplit.clear();
    numberOfThreads.store(0);
    quadtree_done.store(false);

    // Generate the outer edge
    quadtree_createOutsideImage();

    // Create all of the slave threads
    std::vector<std::thread> threadPool;
    for (unsigned int i=0; i<max_threads; i++) {
        threadPool.push_back(std::thread(&MandelbrotViewer::quadtree_slave, this));
    }

    quadtree_done.store(false);
    restart_gen.store(false);
    Plus plus;
    Square square;
    while ( !quadtree_done.load() && !restart_gen.load() ) {
        // Write all the pluses
        mutex_plusToWrite.lock();
        while (plusToWrite.size() != 0) {
            plus = plusToWrite[plusToWrite.size()-1];
            plusToWrite.pop_back();
            mutex_plusToWrite.unlock();

            quadtree_writePlus(plus);

            mutex_plusToWrite.lock();
        }
        mutex_plusToWrite.unlock();
        // Check all of the squares in the vector
        while ( squaresToCheck.size() != 0 ) {
            square = squaresToCheck[squaresToCheck.size()-1];
            squaresToCheck.pop_back();

            quadtree_checkSquare(square);
        }
        // Write all the squares in the vector
        while ( squaresToWrite.size() != 0 ) {
            square = squaresToWrite[squaresToWrite.size()-1];
            squaresToWrite.pop_back();

            quadtree_writeSquare(square);
        }
        quadtree_done.store(quadtree_masterDone());

        if (restart_gen.load() == true) {
            printf("Restarted gen!\n");
            break;
        }
    }

    // Join all the threads in the pool
    for (unsigned int i=0; i<max_threads; i++) {
        threadPool[i].join();
    }
    // If we ended early, return before we draw half an image
    if (restart_gen.load() == true) {
        printf("Returning\n");
        last_max_iter.store( max_iter.load() );
        return;
    }
    
    for (int i=0; i<res_width; i++) {
        for (int j=0; j<res_height; j++) {
            image.setPixel(i, j, findColor(image_array[j][i]));
        }
    }
    printf("created image\n");
    last_max_iter.store( max_iter.load() );
}
void MandelbrotViewer::quadtree_slave() {
    Square square;
    while (!quadtree_done.load()) {
        mutex_squaresToSplit.lock();
        if ( squaresToSplit.size() != 0) {
            numberOfThreads++;
            square = squaresToSplit[squaresToSplit.size()-1];
            squaresToSplit.pop_back();
            mutex_squaresToSplit.unlock();

            quadtree_splitSquare(square);

            numberOfThreads--;
            mutex_squaresToSplit.lock();
        }
        mutex_squaresToSplit.unlock();

        // Check if we're trying to restart the generation
        if (restart_gen.load() == true) {
            printf("Slave died\n");
            return;
        }
    }
}
// End N Stuff

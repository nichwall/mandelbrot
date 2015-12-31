#include "mandelbrotViewer.h"
#include <string.h>
#include <iostream>
#include <iomanip>
#include <math.h>
#include <sstream>
#include <thread>
#include <ctime>

int wakeups = 0;

# define PI 3.14159265358979323846

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
    std::vector<int> pal_row(max_iter);
    palette.push_back(pal_row);
    palette.push_back(pal_row);
    palette.push_back(pal_row);

    //initialize the mandelbrot parameters
    resetMandelbrot();

    //initialize the font for the overlay
	if (font.loadFromFile("cour.ttf"));
	else if (font.loadFromFile("C:\\Windows\\Fonts\\cour.ttf"));
	else std::cout << "ERROR: unable to load font\n";

    //initialize the iter_array
    size_t sizeX = res_width;
    size_t sizeY = res_height;
    std::vector< std::vector<int> > array(sizeY, std::vector<int>(sizeX));
    iter_array = array;
    border_array = array;
    fill_array = array;

    //get the number of supported concurrent threads
    max_threads = std::thread::hardware_concurrency();

    //disable repeated keys
    //window->setKeyRepeatEnabled(false);

    rotation = 0;
}

MandelbrotViewer::~MandelbrotViewer() { }


//random useful functions

//reset all elements of a 2D vector to 0
void zeroVector2(std::vector< std::vector<int> > &v) {
    for (int i=0; i<v.size(); i++) {
        std::fill(v[i].begin(), v[i].end(), 0);
    }
}

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
    int magnitude = (int) log10(max_iter);
    unsigned int inc = pow(10, magnitude);
    max_iter += inc;
    initPalette();
}

void MandelbrotViewer::decIterations() {
    //if iterations is in the hundreds, subtract 100
    //if iterations is in the thousands, subtract 1000, etc.
    if (max_iter > 100) {
        int magnitude = (int) log10(max_iter);
        unsigned int dec = pow(10, magnitude);
        if (dec == max_iter) dec /= 10;
        max_iter -= dec;
        initPalette();
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

void MandelbrotViewer::setSkeleton(bool toggle) {
    skeleton = toggle;
    generate();
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
            image.setPixel(j, i, findColor(iter_array[i][j]));
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

    //save the old center
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

    //resize the iter_array
    size_t sizeX = res_width;
    size_t sizeY = res_height;
    std::vector< std::vector<int> > array(sizeY, std::vector<int>(sizeX));
    iter_array = array;
    border_array = array;
    fill_array = array;

    resetView();
}

//generate the mandelbrot
void MandelbrotViewer::generate() {
/*
    bool done = false;
    restart_gen = false;

    while (!done) {

        //finished_threads is incremented each time a thread finishes
        finished_threads = 0;
        nextLine = 0; //start at the top --TODO: remove this with quadtree

        //create and launch the thread pool
        std::vector<std::thread> threadPool;
        for (int i=0; i<max_threads; i++) {
            threadPool.push_back(std::thread(&MandelbrotViewer::genLine, this));
        }

        //wait for the threads to finish
        for (int i=0; i<max_threads; i++) {
            threadPool[i].join();
        }

        //when the threads finish prematurely, reset needed variables and restart generation
        if (restart_gen) {
            done = false;
            restart_gen = false;
        } else done = true;
    }
*/

    //zero the writtable arrays
    zeroVector2(border_array);
    zeroVector2(fill_array);

    //reset # of waiting threads to 0
    waiting_threads = 0;
    wakeups = 0;

    //generate the border
    for (int y=0; y<res_height; y++) {
        border_array[y][0] = escape(y, 0);
        border_array[y][res_width-1] = escape(y, res_width-1);
    }
    for (int x=0; x<res_width; x++) {
        border_array[0][x] = escape(0, x);
        border_array[res_height-1][x] = escape(res_height-1, x);
    }

    //create and launch the thread pool
    std::vector<std::thread> threadPool;
    for (int i=0; i<max_threads; i++) {
        if (i == 0)
            threadPool.push_back(std::thread(&MandelbrotViewer::quadTreeWorker, this, true));
        else
            threadPool.push_back(std::thread(&MandelbrotViewer::quadTreeWorker, this, false));
    }

    //wait for the threads to finish
    for (int i=0; i<max_threads; i++) {
        threadPool[i].join();
    }

    //combine border_array and fill_array into iter_array
    for (int row=0; row<res_height; row++) {
        for (int column=0; column<res_width; column++) {
            if (border_array[row][column])
                iter_array[row][column] = border_array[row][column];
            else if (fill_array[row][column])
                iter_array[row][column] = fill_array[row][column];
        }
    }

    //write the image using iter_array
    if (!skeleton) {
        for (int row=0; row<res_height; row++) {
            for (int column=0; column<res_width; column++) {
                image.setPixel(column, row, findColor(iter_array[row][column]));
            }
        }
     } else {
         for (int row=0; row<res_height; row++) {
             for (int column=0; column<res_width; column++) {
                 image.setPixel(column, row, findColor(border_array[row][column]));
             }
         }
     }
    std::cout << "wakeups: " << wakeups << std::endl;

    //reset last_max_iter to the new max_iter
    last_max_iter = max_iter;
}

struct Square {
    int x_min, x_max, y_min, y_max;
    bool wakeup = false;
} next_square;

//This function is for worker threads. The first instance of this function
//should be called with start = true, the others with start = false.
//Threads will wait until they are given a task by genSquare.
void MandelbrotViewer::quadTreeWorker(bool start) {
    bool done = false;
    int x_min, x_max, y_min, y_max;

    std::unique_lock<std::mutex> lk(thread_mutex);
    lk.unlock();

    if (start) genSquare(0, res_width-1, 0, res_height-1);

    while (!done) {
        //start waiting. It will wake up if signaled by another thread,
        //or if all the threads are waiting (i.e. it's done)
        lk.lock();
        waiting_threads++;

        //if the last thread starts waiting, notify the others that it's done and return
        if (waiting_threads >= max_threads) {
            lk.unlock();
            thread_cv.notify_all();
            done = true;
            return;
        }

        //wait for the program to finish or until the thread is needed again
        thread_cv.wait(lk, [&]{return (waiting_threads == max_threads || next_square.wakeup);});

        //if it's done, return
        if (waiting_threads >= max_threads) {
            lk.unlock(); //TODO is this necessary?
            done = true;
        }
        //otherwise, start generating the next square
        else if (next_square.wakeup) {
            next_square.wakeup = false;
            x_min = next_square.x_min;
            x_max = next_square.x_max;
            y_min = next_square.y_min;
            y_max = next_square.y_max;
            lk.unlock();
            genSquare(x_min, x_max, y_min, y_max);
            done = false;
        }
    }
}

//this function checks the border of the square handed to it. If all the border
//has the same # of iterations, it fills it in. If not, it splits it into four squares and
//recursively calls itself to check each of the smaller squares
void MandelbrotViewer::genSquare(int x_min, int x_max, int y_min, int y_max) {

    //shared mutex for reading border_array
    border_mutex.lock_shared();
    int reference = border_array[y_min][x_min];

    bool fillable = true;

    //check left and right sides
    for (int y=y_min; y<=y_max; y++) {
        if (border_array[y][x_min] != reference || border_array[y][x_max] != reference) {
            fillable = false;
            break;
        }
    }

    //check top and bottom
    if (fillable) {
        for (int x=x_min; x<=x_max; x++) {
            if (border_array[y_min][x] != reference || border_array[y_max][x] != reference) {
                fillable = false;
                break;
            }
        }
    }
    //done checking border, unlock shared border_mutex
    border_mutex.unlock_shared();

    //if all sides check out, fill it in
    if (fillable) {
        std::unique_lock<std::mutex> lk(fill_mutex); //lock mutex for writing to fill_array
        for (int x=x_min+1; x<x_max; x++) {
            for (int y=y_min+1; y<y_max; y++) {
                fill_array[y][x] = reference;
            }
        }
    }

    //if the sides do not check out, split the square into four and check each of them
    else {
        int x_mean = x_min + (x_max- x_min)/2;
        int y_mean = y_min + (y_max- y_min)/2;
        if (y_mean == y_min) return; //if it's only 2 pixels high, return
        if (x_mean == x_min) return; //if it's only 2 pixels wide, return

        int vert[y_max-y_min-1];
        int hori[y_max-y_min-1];
        
        //generate a '+' in the middle of the square, effectively splitting it into 4
        //save it to two temporary 1D arrays
        for (int x=x_min+1; x<x_max; x++) {
            hori[x-x_min-1] = escape(y_mean, x);
        }
        for (int y=y_min+1; y<y_max; y++) {
            vert[y-y_min-1] = escape(y, x_mean);
        }
        
        //now write those generated points to border_array
        border_mutex.lock();
        for (int y=y_min+1; y<y_max; y++) {
            //border_array[y][x_mean] = vert[y-y_min-1];
            border_array[y][x_mean] = vert[y-y_min-1]; 
        }
        for (int x=x_min+1; x<x_max; x++) {
            //border_array[y_mean][x] = hori[x-x_min-1];
            border_array[y_mean][x] = hori[x-x_min-1];
        }
        border_mutex.unlock();
        
        //check each of the sub-squares
        //if there are unoccupied threads, delegate to them

        //top left square
        thread_mutex.lock();
        if (waiting_threads && !next_square.wakeup && x_max-x_min > 100) {
            next_square.x_min = x_min;
            next_square.x_max = x_mean;
            next_square.y_min = y_min;
            next_square.y_max = y_mean;
            next_square.wakeup = true;
            waiting_threads--;
            wakeups++;
            thread_mutex.unlock();
            thread_cv.notify_one();
        } else {
            thread_mutex.unlock();
            genSquare(x_min, x_mean, y_min, y_mean);
        }
        
        //bottom left square
        thread_mutex.lock();
        if (waiting_threads && !next_square.wakeup && x_max-x_min > 100) {
            next_square.x_min = x_min;
            next_square.x_max = x_mean;
            next_square.y_min = y_mean;
            next_square.y_max = y_max;
            next_square.wakeup = true;
            waiting_threads--;
            wakeups++;
            thread_mutex.unlock();
            thread_cv.notify_one();
        } else {
            thread_mutex.unlock();
            genSquare(x_min, x_mean, y_mean, y_max);
        }

        //top right square
        thread_mutex.lock();
        if (waiting_threads && !next_square.wakeup && x_max-x_min > 100) {
            next_square.x_min = x_mean;
            next_square.x_max = x_max;
            next_square.y_min = y_min;
            next_square.y_max = y_mean;
            next_square.wakeup = true;
            waiting_threads--;
            wakeups++;
            thread_mutex.unlock();
            thread_cv.notify_one();
        } else {
            thread_mutex.unlock();
            genSquare(x_mean, x_max, y_min, y_mean);
        }

        //bottom right square
        genSquare(x_mean, x_max, y_mean, y_max);
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

    max_iter = 100;
    last_max_iter = 100;
    color_multiple = 1;
    rotation = 0;
    color_locked = false;
    initPalette();
    skeleton = false;
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
        controls.setString("                 Help Menu (Tab)\n"
                        "Controls\n"
                        "------------------------------------------------\n"
                        "Tab               - Help menu\n"
                        "Left/Right arrows - Change colors\n"
                        "Up/Down arrows    - Increase/decrease iterations\n"
                        "Click and Drag    - Move around\n"
                        "Numbers 1-7       - Change color scheme\n"
                        "Scroll            - Zoom in/out\n"
                        "S                 - Save image\n"
                        "R                 - Reset\n"
                        "L                 - Lock Colors\n"
                        "K                 - Toggle skeleton mode\n"
                        "Q                 - Quit\n"
                        "Page up           - Rotate counter-clockwise\n"
                        "Page down         - Rotate clockwise\n"
                        "Home              - Reset rotation\n"
                        "------------------------------------------------\n");
        controls.setCharacterSize(22);
        controls.setColor(sf::Color::White);
        controls.setPosition(40, 30);

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
		ss << "\n\nIterations: " << max_iter << std::fixed << std::setprecision(0);
        if (skeleton)
            ss << "\t\t\t\tSkeleton mode enabled";
        else
            ss << "\t\t\t\tSkeleton mode disabled";
        ss << "\n\nRotation: " << angle << " degrees";

        stats.setFont(font);
        stats.setString(ss.str());
        stats.setCharacterSize(22);
        stats.setPosition(40, 490);

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
    if (last_max_iter < max_iter && iter_array[row][column] < last_max_iter) 
        return iter_array[row][column];
    //check if we decreased iterations and if the pixel already converged
    else if (last_max_iter > max_iter && iter_array[row][column] > max_iter)
        return iter_array[row][column];
    //if not, use the escape-time algorithm to calculate iter
    else {
        //convert from pixel to complex coordinates
        sf::Vector2f pnt(column, row);
        sf::Vector2<double> point = pixelToComplex(pnt);
        
        //rotate the point
        if (rotation) point = rotate(point);

        double x = 0, y = 0;
        int iter = 0;

        double x_square = 0;
        double y_square = 0;

        //this is a specialized version of z = z^2 + c. It only does three multiplications,
        //instead of the normal six. Multplications are very costly with such high precision
        for (; iter < max_iter; iter++) {
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
    return max_iter;
}

//findColor uses the number of iterations passed to it to look up a color in the palette
sf::Color MandelbrotViewer::findColor(int iter) {
    int i = (int) fmod(iter * color_multiple, palette[0].size());
    sf::Color color;
    if (iter >= max_iter) color = sf::Color::Black;
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
        palette[0].resize(max_iter);
        palette[1].resize(max_iter);
        palette[2].resize(max_iter);
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

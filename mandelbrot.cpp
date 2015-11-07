#include "mandelbrot.h"
#include <math.h>
#include <string.h>
#include <ctime>
#include <iostream>

int nextLine = 0;
int palette[3][256];
void initPalette();
sf::Mutex mutex1;
sf::Mutex mutex2;

//constructors:
Mandelbrot::Mandelbrot(sf::RenderWindow *windowPointer, int resolution) {
    x_min = -1.5;
    x_max = 0.5;
    y_min = -1.0;
    y_max = 1.0;
    MAX_ITER = 100;
    RESOLUTION = resolution;
    window = windowPointer;
    texture.create(RESOLUTION, RESOLUTION);
    image.create(RESOLUTION, RESOLUTION, sf::Color::Black);
    initPalette();
    color_multiple = 1;
}

//accessors:
int Mandelbrot::getResolution() {return RESOLUTION;}

void Mandelbrot::setIterations(int iterations) {MAX_ITER = iterations;}

void Mandelbrot::setColorMultiple(int multiple) {color_multiple = multiple;}

int getNextLine() {
    return nextLine++;
}

//functions:
int coerce(int number) {
    if (number > 255) number = 255;
    else if (number < 0) number = 0;
    return number;
}

void initPalette() {
    int r, g, b;
    for (int i = 0; i <= 255; i++) {
        r = (int) (23.45 - 1.880*i + 0.0461*pow(i,2) - 0.000152*pow(i,3));
        g = (int) (17.30 - 0.417*i + 0.0273*pow(i,2) - 0.000101*pow(i,3));
        b = (int) (25.22 + 7.902*i - 0.0681*pow(i,2) + 0.000145*pow(i,3));

        palette[0][i] = coerce(r);
        palette[1][i] = coerce(g);
        palette[2][i] = coerce(b);
    }

}

double Mandelbrot::interpolate(double min, double max, int range) {
    return (max - min) / range;
}

int Mandelbrot::escape(double x0, double y0, int MAX) {
    double x = 0, y = 0, x_check = 0, y_check = 0;
    int iter = 0, period = 2;

    double x_square = 0;
    double y_square = 0;

    while(period < MAX_ITER) {
        x_check = x;
        y_check = y;
        period += period;
        if (period > MAX_ITER) period = MAX_ITER;
        for (; iter < period; iter++) {
            y = x * y;
            y += y; //multiply by two
            y += y0;
            x = x_square - y_square + x0;
            x_square = x*x;
            y_square = y*y;

            if (x_square + y_square > 4.0) return iter;
            if ((x == x_check) && (y == y_check)){
                return MAX_ITER;
            }
        }
    }
    return MAX_ITER;
}

sf::Color Mandelbrot::findColor(int iter) {
    int i = (iter * color_multiple) % 255;
    sf::Color color;
    if (iter >= MAX_ITER) color = sf::Color::Black;
    else {
        color.r = palette[0][i];
        color.g = palette[1][i];
        color.b = palette[2][i];
    }
    return color;
}

void Mandelbrot::generate() {
    nextLine = 0;

    sf::Thread thread1(&Mandelbrot::genLine, this);
    sf::Thread thread2(&Mandelbrot::genLine, this);
    sf::Thread thread3(&Mandelbrot::genLine, this);
    sf::Thread thread4(&Mandelbrot::genLine, this);

    thread1.launch();
    thread2.launch();
    thread3.launch();
    thread4.launch();

    thread1.wait();
    thread2.wait();
    thread3.wait();
    thread4.wait();

    texture.update(image);
    sprite.setTexture(texture);
}

void Mandelbrot::genLine() {
    int iter, row, column;
    double x, y;
    double x_inc = interpolate(x_min, x_max, RESOLUTION);
    double y_inc = interpolate(y_min, y_max, RESOLUTION);
    sf::Color color;

    while(true) {

        //mutex this!
        mutex1.lock();
        row = getNextLine();
        mutex1.unlock();
        if (row >= RESOLUTION) return;

        y = y_max - row * y_inc;

        for (column = 0; column < RESOLUTION; column++) {
            x = x_min + column * x_inc;

            iter = escape(x, y, MAX_ITER);

            //mutex this!
            mutex2.lock();
            image.setPixel(column, row, findColor(iter));
            mutex2.unlock();
        }
    }
}

void Mandelbrot::draw() {
    window->draw(sprite);
}

void Mandelbrot::zoomIn(int x, int y) {
    double x_center = x_min + x * interpolate(x_min, x_max, RESOLUTION);
    double y_center = y_max - y * interpolate(y_min, y_max, RESOLUTION);

    double x_length = ((x_max - x_min) / 4.0);
    double y_length = ((y_max - y_min) / 4.0);

    x_max = x_center + x_length;
    x_min = x_center - x_length;

    y_max = y_center + y_length;
    y_min = y_center - y_length;
}

void Mandelbrot::zoomOut(int x, int y) {
    double x_center = x_min + x * interpolate(x_min, x_max, RESOLUTION);
    double y_center = y_max - y * interpolate(y_min, y_max, RESOLUTION);

    double x_length = x_max - x_min;
    double y_length = y_max - y_min;

    x_max = x_center + x_length;
    x_min = x_center - x_length;

    y_max = y_center + y_length;
    y_min = y_center - y_length;
}

void Mandelbrot::reset() {
    x_min = -1.5;
    x_max = 0.5;
    y_min = -1.0;
    y_max = 1.0;
    MAX_ITER = 50;
    color_multiple = 1;
}

void Mandelbrot::saveImage() {
    time_t currentTime = time(0);
    tm* currentDate = localtime(&currentTime);
    char filename[80];
    strftime(filename,80,"%Y-%m-%d.%H-%M-%S",currentDate);
    strcat(filename, ".png");

    image.saveToFile(filename);
    std::cout << "Saved image to " << filename << std::endl;
}

void Mandelbrot::drag(sf::Vector2i old_position, sf::Vector2i new_position) {
    sf::Vector2i dif = new_position - old_position;
    sf::Vector2<double> old_center((x_min + x_max)/2.0, (y_min + y_max)/2.0);
    sf::Vector2<double> new_center((old_center.x - interpolate(0, dif.x, RESOLUTION)),
            old_center.y + interpolate(0, dif.y, RESOLUTION));
    double x_length = (x_max - x_min)/2.0;
    double y_length = (y_max - y_min)/2.0;
    x_max = new_center.x + x_length;
    x_min = new_center.x - x_length;
    y_max = new_center.y + y_length;
    y_min = new_center.y - y_length;

    generate();
    draw();
}

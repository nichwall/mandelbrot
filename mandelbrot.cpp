#include "mandelbrot.h"
#include <math.h>
#include <iostream>

int Mandelbrot::palette = new int[2][255];

//constructors:
Mandelbrot::Mandelbrot(sf::RenderWindow *windowPointer, int resolution) {
    x_min = -1.5;
    x_max = 0.5;
    y_min = -1.0;
    y_max = 1.0;
    MAX_ITER = 50;
    RESOLUTION = resolution;
    window = windowPointer;
    texture.create(RESOLUTION, RESOLUTION);
    image.create(RESOLUTION, RESOLUTION, sf::Color::Black);
}

//accessors:
int Mandelbrot::getResolution() {return RESOLUTION;}

void Mandelbrot::setIterations(int iterations) {MAX_ITER = iterations;}

//functions:

int coerce(int number) {
    if (number > 255) number = 255;
    else if (number < 0) number = 0;
    return number;
}

void Mandelbrot::initPalette() {
    int r, g, b;
    std::cout << "MAX_ITER before loop: " << MAX_ITER << "\n";
    std::cout << x_min << " " << y_max << " " << RESOLUTION << "\n";
    for (int i = 0; i <= 255; i++) {
        r = (int) (23.45 - 1.880*i + 0.0461*pow(i,2) - 0.000152*pow(i,3));
        g = (int) (17.30 - 0.417*i + 0.0273*pow(i,2) - 0.000101*pow(i,3));
        b = (int) (25.22 + 7.902*i - 0.0681*pow(i,2) + 0.000145*pow(i,3));

        palette[0][i] = coerce(r);
        palette[1][i] = coerce(g);
        palette[2][i] = coerce(b);
    }
    std::cout << "MAX_ITER after loop: " << MAX_ITER << "\n";
    std::cout << x_min << " " << y_max << " " << RESOLUTION << "\n";

}

double Mandelbrot::interpolate(double min, double max, int range) {
    return (max - min) / range;
}

double Mandelbrot::magnitude(double x, double y) {
    return sqrt(x * x + y * y);
}

int Mandelbrot::escape(double x0, double y0, int MAX) {
    double x = 0, y = 0;
    double x_temp;
    int iter;

//    double p = sqrt(pow((x0 - 0.25),2) + pow(y0,2));
//    if(x0 < p - 2 * p * p + 0.25 || pow((x0 + 1),2) + y0 * y0 < 0.0625) return MAX;

    for (iter = 0; iter < MAX && magnitude(x,y) < 2; iter++) {
        x_temp = x*x - y*y + x0;
        y = 2*x*y + y0;
        x = x_temp;
    }
    return iter;
}

sf::Color Mandelbrot::findColor(int iter) {
    int i = iter % 255;
//  sf::Color color(palette[0][i], palette[1][i], palette[2][i]);
    sf::Color color;
    if (iter >= MAX_ITER) color = sf::Color::Black;
    else if (iter < MAX_ITER) color = sf::Color::White;
    return color;
}

void Mandelbrot::generate() {
    int iter, row, column;
    double x, y;
    double x_inc = interpolate(x_min, x_max, RESOLUTION);
    double y_inc = interpolate(y_min, y_max, RESOLUTION);
    sf::Color color;

    for (row = 0; row < RESOLUTION; row++) {
        y = y_max - row * y_inc;

        for (column = 0; column < RESOLUTION; column++) {
            x = x_min + column * x_inc;

            iter = escape(x, y, MAX_ITER);

            std::cout << "col: " << column << " row: " << row << " iter: " << iter << std::endl;

            color = findColor(iter);
            std::cout << "color " << color.r << " " << color.g << " " << color.b << "\n";

            image.setPixel(column, row, sf::Color::White);
            std::cout << "wrote pixel" << std::endl;
        }
    }
    std::cout << "finished image gen, setting..." << std::endl;

    texture.update(image);
    sprite.setTexture(texture);
    std::cout << "updated texture" << std::endl;
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
}

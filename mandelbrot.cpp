#include "mandelbrot.h"
#include <math.h>
#include <iostream>

//constructors:
Mandelbrot::Mandelbrot(sf::RenderWindow *windowPointer, int resolution) {
    x_min = -1.5;
    x_max = 0.5;
    y_min = -1.0;
    y_max = 1.0;
    RESOLUTION = resolution;
    MAX_ITER = 50;
    window = windowPointer;
    texture.create(RESOLUTION, RESOLUTION);
    image.create(RESOLUTION, RESOLUTION, sf::Color::Black);

    for (int i = 0; i <= 255; i++) {
        palette[0][i] = (int) (23.45 - 1.880*i + 0.0461*pow(i,2) - 0.000152*pow(i,3));
        palette[1][i] = (int) (17.30 - 0.417*i + 0.0273*pow(i,2) - 0.000101*pow(i,3));
        palette[2][i] = (int) (25.22 + 7.902*i - 0.0681*pow(i,2) + 0.000145*pow(i,3));

        for (int j = 0; j <= 2; j++) {
            if (palette[j][i] > 255) palette[j][i] = 255;
            else if (palette[j][i] < 0) palette[j][i] = 0;
        }
    }
}

//accessors:
int Mandelbrot::getResolution() {return RESOLUTION;}

void Mandelbrot::setIterations(int iterations) {MAX_ITER = iterations;}

//functions:
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

    std::cout << "max iterations: " << MAX;
//    double p = sqrt(pow((x0 - 0.25),2) + pow(y0,2));
//    if(x0 < p - 2 * p * p + 0.25 || pow((x0 + 1),2) + y0 * y0 < 0.0625) return MAX;

    for (iter = 0; iter < MAX && magnitude(x,y) < 2; iter++) {
        x_temp = x*x - y*y + x0;
        y = 2*x*y + y0;
        x = x_temp;
    }
    std::cout << iter << " found iter\n";
    return iter;
}

sf::Color Mandelbrot::findColor(int iter) {
    int i = iter % 255;
//  sf::Color color(palette[0][i], palette[1][i], palette[2][i]);
    sf::Color color;
    if (iter >= MAX_ITER) color = sf::Color::Black;
    else if (iter < MAX_ITER) color = sf::Color::White;
    std::cout << "color " << color.r << " " << color.g << " " << color.b << "\n";
    return color;
}

void Mandelbrot::generate() {
    int iter, row, column;
    double x, y;
    double x_inc = interpolate(x_min, x_max, RESOLUTION);
    double y_inc = interpolate(y_min, y_max, RESOLUTION);
    std::cout << "starting image gen" << std::endl;

    for (row = 0; row < RESOLUTION; row++) {
        y = y_max - row * y_inc;

        for (column = 0; column < RESOLUTION; column++) {
            x = x_min + column * x_inc;

            std::cout << "MAX_ITER: " << MAX_ITER << std::endl;
            iter = escape(x, y, MAX_ITER);

            std::cout << "setting pixel..." << std::endl;
            std::cout << "col: " << column << " row: " << row << " iter: " << iter << std::endl;

            image.setPixel(column, row, findColor(iter));
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

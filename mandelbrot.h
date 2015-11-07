#ifndef MANDELBROT_H
#define MANDELBROT_H

#include <SFML/Graphics.hpp>

class Mandelbrot {
    public:
        Mandelbrot(sf::RenderWindow*, int resolution);
        void draw();
        void generate();
        void reset();
        void setIterations(int iterations);
        void zoomIn(int x, int y);
        void zoomOut(int x, int y);
        void initPalette();
        int getResolution();

    private:
        double interpolate(double min, double max, int range);
        double magnitude(double x, double y);
        int escape(double x, double y, int MAX);
        extern int palette[2][255];

        sf::Color findColor(int iter);
        sf::Image image;
        sf::Texture texture;
        sf::RenderWindow *window;
        sf::Sprite sprite;

        double x_min;
        double x_max;
        double y_min;
        double y_max;
        int RESOLUTION;
        int MAX_ITER;
};

#endif

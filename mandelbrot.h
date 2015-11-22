#ifndef MANDELBROT_H
#define MANDELBROT_H

#include <SFML/Graphics.hpp>

class Mandelbrot {
    public:
        Mandelbrot(int resolution);
        ~Mandelbrot();
        sf::Sprite generate(bool udpate = true);
        void reset();
        void setIterations(int iterations);
        void setColorMultiple(double multiple);
        void zoomIn(int x, int y);
        void zoomOut(int x, int y);
        void saveImage();
        void changeColor();
        void updateTexture();
        void drag(sf::Vector2i old_position, sf::Vector2i new_position);
        double getColorMultiple();
        int getResolution();

    private:
        double interpolate(double min, double max, int range);
        int escape(double x, double y, int MAX);
        void genLine();

        sf::Color findColor(int iter);
        sf::Image image;
        sf::Texture texture;
        sf::Sprite sprite;

        double x_min;
        double x_max;
        double y_min;
        double y_max;
        double color_multiple;
        int RESOLUTION;
        int MAX_ITER;
};

#endif

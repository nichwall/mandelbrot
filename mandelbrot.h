#ifndef MANDELBROT_H
#define MANDELBROT_H

#include <SFML/Graphics.hpp>

class Mandelbrot {
    public:
        Mandelbrot(int resolution);
        sf::Sprite generate(bool udpate = true);
        void reset();
        void setIterations(int iterations);
        void setColorMultiple(int multiple);
        void zoomIn(int x, int y);
        void zoomOut(int x, int y);
        void saveImage();
        void updateTexture();
        void drag(sf::Vector2i old_position, sf::Vector2i new_position);
        int getColorMultiple();
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
        int RESOLUTION;
        int MAX_ITER;
        int color_multiple;
};

#endif

#ifndef VIEWER_H
#define VIEWER_H

#include <SFML/Graphics.hpp>

class Viewer {
    public:
        Viewer(int resolution);
        int getResolution() {return resolution;}
        sf::Sprite getSprite() {return sprite;}
        void setSprite(sf::Sprite);
        void draw();
        void close();
        void clear();
        void display();
        bool isOpen();
        bool getEvent(sf::Event&);
        sf::Vector2i getMousePosition();

    private:
        int resolution;
        int framerateLimit;
        sf::RenderWindow *window;
        sf::Sprite sprite;

};


#endif

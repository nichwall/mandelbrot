#ifndef VIEWER_H
#define VIEWER_H

#include <SFML/Graphics.hpp>

class Viewer {
    public:
        Viewer(int res);
        int getResolution() {return resolution;}
        void setFramerate(int framerate);
        sf::Vector2f getCenter() {return view->getCenter();}
        sf::Sprite getSprite() {return sprite;}
        void setSprite(sf::Sprite);
        void zoom(sf::Vector2f center, double zoom);
        void refresh();
        void close();
        void resetView();
        void resetZoom(sf::Vector2f);
        bool isOpen();
        bool getEvent(sf::Event&);
        sf::Vector2i getMousePosition();

    private:
        int resolution;
        int framerateLimit;
        sf::RenderWindow *window;
        sf::View *view;
        sf::Sprite sprite;

};


#endif

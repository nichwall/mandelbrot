#include "viewer.h"
#include <iostream>

Viewer::Viewer(int res) {
    resolution = res;
    static sf::RenderWindow win(sf::VideoMode(resolution, resolution), "Mandelbrot Explorer");
    static sf::View vw(sf::FloatRect(0, 0, resolution, resolution));
    window = &win;
    view = &vw;
    view->setViewport(sf::FloatRect(0, 0, 1, 1));
    window->setView(*view);
    framerateLimit = 70;
    window->setFramerateLimit(framerateLimit);
}

void Viewer::setFramerate(int framerate) {
    framerateLimit = framerate;
    window->setFramerateLimit(framerateLimit);
}

void Viewer::refresh() {
    window->clear(sf::Color::Black);
    window->setView(*view);
    window->draw(sprite);
    window->display();
}

void Viewer::zoom(sf::Vector2f center, double zoom) {
    //resets the view to zoom into the given center

    resetView();

    view->setCenter(center);
    view->zoom(zoom);
    
    window->setView(*view);
}

void Viewer::resetView() {
    view->reset(sf::FloatRect(0, 0, resolution, resolution));
    window->setView(*view);
}

void Viewer::setSprite(sf::Sprite newSprite) {
    sprite = newSprite;
}

void Viewer::close() {
    window->close();
}

bool Viewer::isOpen() {
    return window->isOpen();
}

bool Viewer::getEvent(sf::Event& event) {
    return window->pollEvent(event);
}

sf::Vector2i Viewer::getMousePosition() {
    return sf::Mouse::getPosition(*window);
}

#include "viewer.h"
#include <iostream>

Viewer::Viewer(int resolution) {
    static sf::RenderWindow win(sf::VideoMode(resolution, resolution), "Mandelbrot Explorer");
    window = &win;
    framerateLimit = 10;
    window->setFramerateLimit(framerateLimit);
}

void Viewer::draw() {
    window->draw(sprite);
}

void Viewer::setSprite(sf::Sprite newSprite) {
    sprite = newSprite;
}

void Viewer::close() {
    window->close();
}

bool Viewer::isOpen() {
    std::cout << "isOpen function\n";
    return window->isOpen();
}

bool Viewer::getEvent(sf::Event& event) {
    return window->pollEvent(event);
}

sf::Vector2i Viewer::getMousePosition() {
    return sf::Mouse::getPosition(*window);
}

void Viewer::clear() {
    window->clear(sf::Color::Black);
}

void Viewer::display() {
    window->display();
}

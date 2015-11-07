#include "SFML/Graphics.hpp"
#include "mandelbrot.h"
#include <iostream>

int main() {
    int resolution = 768;
    int iterations = 50;
    int color_multiple = 1;
    sf::RenderWindow * windowPointer;
    sf::RenderWindow window(sf::VideoMode(resolution, resolution), "Mandelbrot Viewer");
    window.setFramerateLimit(10);

    windowPointer = &window;
    Mandelbrot brot(windowPointer, resolution);

    brot.generate();

    //main window loop
    while (window.isOpen()) {

        //main event loop
        sf::Event event;
        while (window.pollEvent(event)) {

            switch (event.type) {
            case sf::Event::KeyPressed:

                switch (event.key.code) {
                    case sf::Keyboard::Q:
                        window.close();
                        break;
                    case sf::Keyboard::Up:
                        iterations += 20;
                        brot.setIterations(iterations);
                        break;
                    case sf::Keyboard::Down:
                        if (iterations > 50) iterations -= 20;
                        brot.setIterations(iterations);
                        break;
                    case sf::Keyboard::Right:
                        color_multiple += 2;
                        brot.setColorMultiple(color_multiple);
                        break;
                    case sf::Keyboard::Left:
                        if (color_multiple > 2) color_multiple -= 2;
                        brot.setColorMultiple(color_multiple);
                        break;
                    case sf::Keyboard::R:
                        brot.reset();
                        break;
                    default:
                        break;
                }
                brot.generate();
                break;
            case sf::Event::MouseWheelScrolled:
                if (event.mouseWheelScroll.delta > 0)
                    brot.zoomIn(event.mouseWheelScroll.x, event.mouseWheelScroll.y);
                else if (event.mouseWheelScroll.delta < 0)
                    brot.zoomOut(event.mouseWheelScroll.x, event.mouseWheelScroll.y);
                brot.generate();
                break;
            default:
                break;
            }

        //end event loop
        }

        window.clear(sf::Color::Black);
        brot.draw();
        window.display();

    //end window loop
    }

    return 0;
}

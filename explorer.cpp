#include "viewer.h"
#include "mandelbrot.h"
#include <iostream>

int main() {
    int resolution = 768;
    int iterations = 100;

    sf::Vector2i old_position;
    sf::Vector2i new_position;

    //sf::RenderWindow * windowPointer;
    //sf::RenderWindow window(sf::VideoMode(resolution, resolution), "Mandelbrot Viewer");
    //window.setFramerateLimit(10);
    //windowPointer = &window;

    Viewer viewer(resolution);
    Mandelbrot brot(resolution);

    viewer.setSprite(brot.generate());
    viewer.clear();
    viewer.draw();

    //main window loop
    while (viewer.isOpen()) {


        //main event loop
        sf::Event event;
        while (viewer.getEvent(event)) {

            switch (event.type) {

            //if the event is a keypress:
            case sf::Event::KeyPressed:

                switch (event.key.code) {
                    case sf::Keyboard::Q:
                        viewer.close();
                        break;
                    case sf::Keyboard::Up:
                        iterations += 20;
                        brot.setIterations(iterations);
                        viewer.setSprite(brot.generate());
                        viewer.draw();
                        break;
                    case sf::Keyboard::Down:
                        if (iterations > 20) iterations -= 20;
                        viewer.setSprite(brot.generate());
                        viewer.draw();
                        break;
                    case sf::Keyboard::Right:
                        brot.setColorMultiple(brot.getColorMultiple()+1);
                        //TODO: now animate the color transition!!!
                        //maybe make a new explorer function for it?
                        //for now, just regenerate:
                        viewer.setSprite(brot.generate());
                        viewer.draw();
                        break;
                    case sf::Keyboard::Left:
                        if (brot.getColorMultiple() > 1)
                            brot.setColorMultiple(brot.getColorMultiple()-1);
                        //TODO: now animate the color transition!!!
                        viewer.setSprite(brot.generate());
                        viewer.draw();
                        break;
                    case sf::Keyboard::R:
                        brot.reset();
                        viewer.setSprite(brot.generate());
                        viewer.draw();
                        break;
                    case sf::Keyboard::S:
                        brot.saveImage();
                        break;
                    default:
                        break;
                }
                break;
                
            //if the event is a mousewheel scroll
            case sf::Event::MouseWheelScrolled:
                if (event.mouseWheelScroll.delta > 0)
                    brot.zoomIn(event.mouseWheelScroll.x, event.mouseWheelScroll.y);
                else if (event.mouseWheelScroll.delta < 0)
                    brot.zoomOut(event.mouseWheelScroll.x, event.mouseWheelScroll.y);
                //TODO: now animate the zoom transition!!!
                //make a new function for it?
                viewer.setSprite(brot.generate());
                viewer.draw();
                break;

            //if the event is a click:
            case sf::Event::MouseButtonPressed:
                old_position = viewer.getMousePosition();

                //don't do anything until it's released
                while (sf::Mouse::isButtonPressed(sf::Mouse::Left)) { }

                new_position = viewer.getMousePosition();
                brot.drag(old_position, new_position);
                viewer.setSprite(brot.generate());
                viewer.draw();
                break;
            default:
                break;
            }

        //end event loop
        }

        //clear-draw-display:
        //clear: clears the window to black
        //draw: draw the image (in memory)
        //display: render the image
        viewer.clear();
        viewer.draw();
        viewer.display();

    //end window loop
    }

    return 0;
}

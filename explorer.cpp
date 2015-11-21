#include "viewer.h"
#include "mandelbrot.h"
#include <unistd.h>
#include <iostream>

double interpolate(double min, double max, int range) { return (max-min)/range; }

int main() {
    int resolution = 960;
    int iterations = 100;

    double inc_zoom, inc_drag_x, inc_drag_y;

    sf::Vector2i old_position;
    sf::Vector2i new_position;
    sf::Vector2f old_center;
    sf::Vector2f new_center;

    Viewer viewer(resolution);
    Mandelbrot brot(resolution);

    viewer.setSprite(brot.generate());
    viewer.refresh();

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
                        viewer.refresh();
                        break;
                    case sf::Keyboard::Down:
                        if (iterations > 20) iterations -= 20;
                        viewer.setSprite(brot.generate());
                        viewer.refresh();
                        break;
                    case sf::Keyboard::Right:
                        brot.setColorMultiple(brot.getColorMultiple()+1);
                        //TODO: now animate the color transition!!!
                        //maybe make a new explorer function for it?
                        //for now, just regenerate:
                        viewer.setSprite(brot.generate());
                        viewer.refresh();
                        break;
                    case sf::Keyboard::Left:
                        if (brot.getColorMultiple() > 1)
                            brot.setColorMultiple(brot.getColorMultiple()-1);
                        //TODO: now animate the color transition!!!
                        viewer.setSprite(brot.generate());
                        viewer.refresh();
                        break;
                    case sf::Keyboard::R:
                        brot.reset();
                        viewer.setSprite(brot.generate());
                        viewer.resetView();
                        viewer.refresh();
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
                if (event.mouseWheelScroll.delta > 0) {
                    //TODO: new thread: generate and zoom simultaneously
                    old_center = viewer.getCenter();
                    new_center.x = event.mouseWheelScroll.x;
                    new_center.y = event.mouseWheelScroll.y;
                    brot.zoomIn(new_center.x, new_center.y);
                    inc_drag_x = interpolate(old_center.x, new_center.x, 50);
                    inc_drag_y = interpolate(old_center.y, new_center.y, 50);
                    inc_zoom = interpolate(1.0, 0.5, 50);

                    //animate the zoom
                    viewer.setFramerate(50);
                    for (int i=0; i<50; i++) {
                        new_center.x = old_center.x + i * inc_drag_x;
                        new_center.y = old_center.y + i * inc_drag_y;
                        viewer.zoom(new_center, 1 + i * inc_zoom);
                        viewer.refresh();
                        //sleep(.2);
                    }
                    viewer.setFramerate(10);
                }
                else if (event.mouseWheelScroll.delta < 0)
                    brot.zoomOut(event.mouseWheelScroll.x, event.mouseWheelScroll.y);
                //TODO: now animate the zoom transition!!!
                //make a new function for it?
                viewer.setSprite(brot.generate());
                viewer.resetView();
                viewer.refresh();
                break;

            //if the event is a click:
            case sf::Event::MouseButtonPressed:
                old_position = viewer.getMousePosition();

                //don't do anything until it's released
                while (sf::Mouse::isButtonPressed(sf::Mouse::Left)) { }

                new_position = viewer.getMousePosition();
                brot.drag(old_position, new_position);
                viewer.setSprite(brot.generate());
                viewer.refresh();
                break;
            default:
                break;
            }

        //end event loop
        }

        //clear-draw-display:
        viewer.refresh();

    //end window loop
    }

    return 0;
}

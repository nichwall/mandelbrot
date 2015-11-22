#include "viewer.h"
#include "mandelbrot.h"
#include <unistd.h>
#include <iostream>

struct zoomParameters {
    Viewer *view;
    sf::Vector2f oldc;
    sf::Vector2f newc;
    bool zoom;
} param;

double interpolate(double min, double max, int range) { return (max-min)/range; }

void zoom() {
    double inc_drag_x = interpolate(param.oldc.x, param.newc.x, 50);
    double inc_drag_y = interpolate(param.oldc.y, param.newc.y, 50);
    double inc_zoom;
    if (param.zoom) inc_zoom = interpolate(1.0, 0.5, 50);
    else inc_zoom = interpolate(1.0, 2.0, 50);

    //animate the zoom
    for (int i=0; i<50; i++) {
        param.newc.x = param.oldc.x + i * inc_drag_x;
        param.newc.y = param.oldc.y + i * inc_drag_y;
        param.view->zoom(param.newc, 1 + i * inc_zoom);
        param.view->refresh();
    }
}

int main() {
    int resolution = 960;
    int iterations = 100;

    bool zoom_in = true;

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
                        brot.generate();
                        viewer.refresh();
                        break;
                    case sf::Keyboard::Down:
                        if (iterations > 20) iterations -= 20;
                        brot.generate();
                        viewer.refresh();
                        break;
                    case sf::Keyboard::Right:
                        brot.setColorMultiple(brot.getColorMultiple()+1);
                        //TODO: now animate the color transition!!!
                        //maybe make a new explorer function for it?
                        //for now, just regenerate:
                        brot.generate();
                        viewer.refresh();
                        break;
                    case sf::Keyboard::Left:
                        if (brot.getColorMultiple() > 1)
                            brot.setColorMultiple(brot.getColorMultiple()-1);
                        //TODO: now animate the color transition!!!
                        brot.generate();
                        viewer.refresh();
                        break;
                    case sf::Keyboard::R:
                        brot.reset();
                        brot.generate();
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
            case sf::Event::MouseWheelScrolled: {
                old_center = viewer.getCenter();
                new_center.x = event.mouseWheelScroll.x;
                new_center.y = event.mouseWheelScroll.y;
                if (event.mouseWheelScroll.delta > 0) {
                    brot.zoomIn(new_center.x, new_center.y);
                    zoom_in = true;
                }
               else if (event.mouseWheelScroll.delta < 0) {
                   brot.zoomOut(event.mouseWheelScroll.x, event.mouseWheelScroll.y);
                   zoom_in = false;
               }
               param.view = &viewer;
               param.oldc = old_center;
               param.newc = new_center;
               param.zoom = zoom_in;
               sf::Thread thread(&zoom);
               thread.launch();
               brot.generate(false);
               thread.wait();
               brot.updateTexture();
               viewer.resetView();
               viewer.refresh();
               break; }

            //if the event is a click:
            case sf::Event::MouseButtonPressed:
                old_position = viewer.getMousePosition();

                //don't do anything until it's released
                while (sf::Mouse::isButtonPressed(sf::Mouse::Left)) { }

                new_position = viewer.getMousePosition();
                brot.drag(old_position, new_position);
                brot.generate();
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

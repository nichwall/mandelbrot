#ifndef MANDELBROTVIEWER_H
#define MANDELBROTVIEWER_H

#include <SFML/Graphics.hpp>
#include <vector>

class MandelbrotViewer {
    public:
        //This constructor creates a new viewer with specified resolution
        MandelbrotViewer(int resolution);
        ~MandelbrotViewer();

        //Accesor functions:
        int getResolution() {return resolution;}
        int getFramerate() {return framerateLimit;}
        double getColorMultiple() {return color_multiple;}
        sf::Vector2i getMousePosition();
        sf::Vector2f getCenter() {return view->getCenter();}
        bool getEvent(sf::Event&);
        bool isOpen();
        
        //Setter functions:
        void setIterations(int iter) {max_iter = iter;}
        void setColorMultiple(double mult) {color_multiple = mult;}
        void setFramerate(int rate) {framerateLimit = rate;}
        
        //Functions to change parameters for mandelbrot generation:
        void changeColor(); //TODO: is this one necessary?
        void changePos(sf::Vector2f new_center, double zoom_factor); //TODO: will spliting this into two make it faster?
        void changePosView(sf::Vector2f new_center, double zoom_factor);

        //Functions ot generate the mandelbrot:
        void generate();

        //Functions to reset or update:
        void resetMandelbrot();
        void refreshWindow();
        void resetView();
        void close();
        void updateMandelbrot();

        //Other functions:
        void saveImage();

    private:
        int resolution;
        int framerateLimit;
        int nextLine;

        //These are pointers to each instance's window and view
        //since we can't initialize them yet
        sf::RenderWindow *window;
        sf::View *view;

        sf::Sprite sprite;
        sf::Image image;
        sf::Texture texture;

        //Parameters to generate the mandelbrot:
        
        //this is the area of the complex plane to generate
        sf::Rect<double> area;
        
        //this changes how the colors are displayed
        double color_multiple;

        //this array stores the number of iterations for each pixel TODO
        std::vector< std::vector<int> > image_array;

        //maximum number of iterations to check for. Higher values are slower,
        //but more precise
        int max_iter;

        //Functions:
        
        //interpolate returns the increment to get from min to max in range iterations
        double interpolate(double min, double max, int range) {return (max-min)/range;}
        double interpolate(double length, int range) {return length/range;}

        //escape calculates the escape-time of given point of the mandelbrot
        int escape(double x, double y);

        //genLine is a function for worker threads: it generates the next line of the
        //mandelbrot, then moves onto the next, until the entire mandelbrot is generated
        void genLine();

        //This looks up a color to print according to the escape value given
        sf::Color findColor(int iter);

        //initialize the color palette. Having a palette helps avoid regenerating the
        //color scheme each time it is needed
        int palette[3][256];
        void initPalette();
};

#endif

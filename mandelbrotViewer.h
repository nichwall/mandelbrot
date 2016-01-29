#ifndef MANDELBROTVIEWER_H
#define MANDELBROTVIEWER_H

#include <SFML/Graphics.hpp>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <vector>
#include <atomic>

struct Color {
    int r;
    int g;
    int b;
};
// Empty struct to store the blocks for master
struct EmptySquare {
    int min_x, max_x, min_y, max_y;
};
struct Plus {
    int min_x, max_x, min_y, max_y;
    int mid_x, mid_y;
    std::vector<int> vertical;
    std::vector<int> horizontal;
};

class MandelbrotViewer {
    public:
        //This constructor creates a new viewer with specified resolution
        MandelbrotViewer(int res_x, int res_y);
        ~MandelbrotViewer();

        //Accesor functions:
        int getResWidth() {return res_width;}
        int getResHeight() {return res_height;}
        int getFramerate() {return framerateLimit;}
        int getIters() {return max_iter;}
        double getRotation() {return rotation;}
        double getColorMultiple() {return color_multiple;}
        sf::Vector2i getMousePosition();
        sf::Vector2f getViewCenter() {return view->getCenter();}
        sf::Vector2f getMandelbrotCenter();
        bool waitEvent(sf::Event&);
        bool pollEvent(sf::Event&);
        bool isColorLocked() {return color_locked;}
        bool isSkeleton() {return skeleton;}
        bool isOpen();
        
        //Setter functions:
        void incIterations();
        void decIterations();
        void setIterations(int iter) {max_iter = iter; initPalette();}
        void setColorMultiple(double mult) {color_multiple = mult;}
        void setFramerate(int rate) {framerateLimit = rate;}
        void setColorScheme(int newScheme);
        void setRotation(double radians);
        void setSkeleton(bool);
        void restartGeneration() {restart_gen = true;}
        void lockColor();
        
        //Functions to change parameters for mandelbrot generation:
        void changeColor();
        void changePos(sf::Vector2<double> new_center, double zoom_factor);
        void changePosView(sf::Vector2f new_center, double zoom_factor);
        void resizeWindow(int newX, int newY);

        //Functions to generate the mandelbrot:
        void generate();
        void quadTree();

        //Functions to reset or update:
        void resetMandelbrot();
        void refreshWindow();
        void resetView();
        void close();
        void updateMandelbrot();
        void setWindowActive(bool);

        //Other functions:
        void saveImage(); //save the image as a png in the local folder
        void enableOverlay(bool); //enable a help overlay with controls, etc.
        void rotateView(float angle);

        //Converts a vector from pixel coordinates to the corresponding
        //coordinates of the complex plane
        sf::Vector2<double> pixelToComplex(sf::Vector2f);

    private:
        int res_height;
        int res_width;
        int framerateLimit;

        sf::Sprite sprite;
        sf::Image image;
        sf::Texture texture;
        sf::Font font;

        //Mutex and condition variable for syncing threads
        std::mutex thread_mutex;
        std::mutex fill_mutex;
        std::shared_timed_mutex border_mutex;
        std::condition_variable thread_cv;
        unsigned int waiting_threads;

        //These are pointers to each instance's window and view
        //since we can't initialize them yet
        sf::RenderWindow *window;
        sf::View *view;

        //Parameters to generate the mandelbrot:
        bool restart_gen; //set to true to stop generation before it's finished
        
        //this is the area of the complex plane to generate
        sf::Rect<double> area;
        double area_inc; //this is complex plane area per pixel

        //this is the current rotation of the mandelbrot - 0 radians is positive x axis
        double rotation;
        
        //this changes how the colors are displayed
        double color_multiple;
        bool color_locked;
        bool skeleton; //if true, show skeleton view
        int scheme;

        //Holds the maximum number of concurrent threads suppported by the current CPU
        unsigned int max_threads;

        //this array stores the number of iterations for each pixel
        std::vector< std::vector<int> > iter_array;  //store iterations
        std::vector< std::vector<int> > border_array;  //store borders of quadtree algorithm
        std::vector< std::vector<int> > fill_array;  //store filled blocks of quadtree algorithm


        //maximum number of iterations to check for. Higher values are slower,
        //but more precise
        unsigned int max_iter;
        unsigned int last_max_iter;


        //Functions:
        
        //interpolate returns the increment to get from min to max in range iterations
        double interpolate(double min, double max, int range) {return (max-min)/range;}
        double interpolate(double length, int range) {return length/range;}

        //escape calculates the escape-time of given point of the mandelbrot
        int escape(int row, int column);

        //genLine is a function for worker threads: it generates the next line of the
        //mandelbrot, then moves onto the next, until the entire mandelbrot is generated
        void genSquare(int x_min, int x_max, int y_min, int y_max);
        void quadTreeWorker(bool start);
        
        // **********************************************************************************
        //N stuff
        std::mutex plus_to_write_mutex;
        std::mutex plus_queue_mutex;

        std::vector< EmptySquare > border_queue; //store empty squares for master thread
        std::vector< EmptySquare > plus_queue;  //store empty squares for slave  thread
        std::vector< Plus >        plus_to_write;      //store the plus from genPlus
        bool genDone;
        std::atomic<unsigned int> running_slaves;

        std::vector< std::vector<int> > master_generateBorder(const EmptySquare &square);
        void generateOutside();
        void slave_genPlus();
        void master_push_empty(const Plus &plus);
        bool master_checkBorder(const EmptySquare &square);
        void master_fillSquare(const EmptySquare &map);
        void master_write_plus(const Plus &plus);

        //End N stuff
        // **********************************************************************************

        //this looks up a color to print according to the escape value given
        sf::Color findColor(int iter);

        //this function handles rotation - it takes in a complex point with zero rotation
        //and returns where that point is when rotated
        sf::Vector2<double> rotate(sf::Vector2<double>);

        //initialize the color palette. Having a palette helps avoid regenerating the
        //color scheme each time it is needed
        std::vector< std::vector<int> > palette;
        void initPalette();
        void smoosh(sf::Color c1, sf::Color c2, float min, float max);
};

#endif


/*
 example shows how to run a CNN based face detector using dlib.  The
    example loads a pretrained model and uses it to find faces in images.  The
    CNN model is much more accurate than the HOG based model shown in the
    face_detection_ex.cpp example, but takes much more computational power to
    run, and is meant to be executed on a GPU to attain reasonable speed.  For
    example, on a NVIDIA Titan X GPU, this example program processes images at
    about the same speed as face_detection_ex.cpp.

    Also, users who are just learning about dlib's deep learning API should read
    the dnn_introduction_ex.cpp and dnn_introduction2_ex.cpp examples to learn
    how the API works.  For an introduction to the object detection method you
    should read dnn_mmod_ex.cpp


    
    TRAINING THE MODEL
        Finally, users interested in how the face detector was trained should
        read the dnn_mmod_ex.cpp example program.  It should be noted that the
        face detector used in this example uses a bigger training dataset and
        larger CNN architecture than what is shown in dnn_mmod_ex.cpp, but
        otherwise training is the same.  If you compare the net_type statements
        in this file and dnn_mmod_ex.cpp you will see that they are very similar
        except that the number of parameters has been increased.

        Additionally, the following training parameters were different during
        training: The following lines in dnn_mmod_ex.cpp were changed from
            mmod_options options(face_boxes_train, 40*40);
            trainer.set_iterations_without_progress_threshold(300);
        to the following when training the model used in this example:
            mmod_options options(face_boxes_train, 80*80);
            trainer.set_iterations_without_progress_threshold(8000);

        Also, the random_cropper was left at its default settings,  So we didn't
        call these functions:
            cropper.set_chip_dims(200, 200);
            cropper.set_min_object_height(0.2);

        The training data used to create the model is also available at 
        http://dlib.net/files/data/dlib_face_detection_dataset-2016-09-30.tar.gz
*/


#include <iostream>
#include <dlib/dnn.h>
#include <dlib/data_io.h>
#include <dlib/image_processing.h>
#include <dlib/image_transforms.h>
#include <dlib/gui_widgets.h>
#include <dlib/opencv.h>
#include <opencv2/highgui/highgui.hpp>


using namespace std;
using namespace dlib;

// ----------------------------------------------------------------------------------------
// Let's begin the network definition by creating some network blocks.

// A 5x5 conv layer that does 2x downsampling
template <long num_filters, typename SUBNET> using con5d = con<num_filters,5,5,2,2,SUBNET>;
// A 3x3 conv layer that doesn't do any downsampling
template <long num_filters, typename SUBNET> using con5  = con<num_filters,5,5,1,1,SUBNET>;

// Now we can define the 8x downsampling block in terms of conv5d blocks.  We
// also use relu and batch normalization in the standard way.
template <typename SUBNET> using downsampler  = relu<bn_con<con5d<32, relu<bn_con<con5d<32, relu<bn_con<con5d<16,SUBNET>>>>>>>>>;

// The rest of the network will be 3x3 conv layers with batch normalization and
// relu.  So we define the 3x3 block we will use here.
template <typename SUBNET> using rcon5  = relu<bn_con<con5<45,SUBNET>>>;

// Finally, we define the entire network.   The special input_rgb_image_pyramid
// layer causes the network to operate over a spatial pyramid, making the detector
// scale invariant.  
using net_type  = loss_mmod<con<1,9,9,1,1,rcon5<rcon5<rcon5<downsampler<input_rgb_image_pyramid<pyramid_down<6>>>>>>>>;


// ----------------------------------------------------------------------------------------


int main(int argc, char** argv) try
{
	
    if (argc == 1)
    {
        cout << "Call this program like this:" << endl;
        cout << "./dnn_mmod_face_detection_ex mmod_human_face_detector.dat faces/*.jpg" << endl;
        cout << "\nYou can get the mmod_human_face_detector.dat file from:\n";
        cout << "http://dlib.net/files/mmod_human_face_detector.dat.bz2" << endl;
        return 0;
    }
	
    net_type net;
    deserialize(argv[1]) >> net;  

    image_window win;
    //for (int i = 2; i < argc; ++i)
    for (int i = 2; i < argc; i++)
    {
	
        load_image(img, argv[i]);
	matrix<rgb_pixel> out_img;
	out_img.set_size(400,400);

	resize_image(img, out_img);

        // Upsampling the image will allow us to detect smaller faces but will cause the
        // program to use more RAM and run longer.
        //while(img.size() > 100*100)
            //pyramid_down(img);

        // Note that you can process a bunch of images in a std::vector at once and it runs
        // much faster, since this will form mini-batches of images and therefore get
        // better parallelism out of your GPU hardware.  However, all the images must be
        // the same size.  To avoid this requirement on images being the same size we
        // process them individually in this example.
        auto dets = net(out_img);
	
        win.clear_overlay();
        win.set_image(out_img);
        for (auto&& d : dets)
	{
	    // This is an example output of d
	    // [(95, 145) (228, 292)]
            win.add_overlay(d);
	    cout << d;
	}
        //cout << "Hit enter to process the next image." << endl;
        //cin.get();
    }
}
catch(std::exception& e)
{
    cout << e.what() << endl;
}




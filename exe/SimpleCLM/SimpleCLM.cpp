///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2014, University of Southern California and University of Cambridge,
// all rights reserved.
//
// THIS SOFTWARE IS PROVIDED �AS IS� AND ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY. OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Notwithstanding the license granted herein, Licensee acknowledges that certain components
// of the Software may be covered by so-called �open source� software licenses (�Open Source
// Components�), which means any software licenses approved as open source licenses by the
// Open Source Initiative or any substantially similar licenses, including without limitation any
// license that, as a condition of distribution of the software licensed under such license,
// requires that the distributor make the software available in source code format. Licensor shall
// provide a list of Open Source Components for a particular version of the Software upon
// Licensee�s request. Licensee will comply with the applicable terms of such licenses and to
// the extent required by the licenses covering Open Source Components, the terms of such
// licenses will apply in lieu of the terms of this Agreement. To the extent the terms of the
// licenses applicable to Open Source Components prohibit any of the restrictions in this
// License Agreement with respect to such Open Source Component, such restrictions will not
// apply to such Open Source Component. To the extent the terms of the licenses applicable to
// Open Source Components require Licensor to make an offer to provide source code or
// related information in connection with the Software, such offer is hereby made. Any request
// for source code or related information should be directed to cl-face-tracker-distribution@lists.cam.ac.uk
// Licensee acknowledges receipt of notices for the Open Source Components for the initial
// delivery of the Software.

//     * Any publications arising from the use of this software, including but
//       not limited to academic journal and conference publications, technical
//       reports and manuals, must cite one of the following works:
//
//       Tadas Baltrusaitis, Peter Robinson, and Louis-Philippe Morency. 3D
//       Constrained Local Model for Rigid and Non-Rigid Facial Tracking.
//       IEEE Conference on Computer Vision and Pattern Recognition (CVPR), 2012.    
//
//       Tadas Baltrusaitis, Peter Robinson, and Louis-Philippe Morency. 
//       Constrained Local Neural Fields for robust facial landmark detection in the wild.
//       in IEEE Int. Conference on Computer Vision Workshops, 300 Faces in-the-Wild Challenge, 2013.    
//
///////////////////////////////////////////////////////////////////////////////

// SimpleCLM.cpp : Defines the entry point for the console application.
#include "CLM_core.h"

#include <fstream>
#include <sstream>

#include <opencv2/videoio/videoio.hpp>  // Video write
#include <opencv2/videoio/videoio_c.h>  // Video write

#define INFO_STREAM( stream ) \
std::cout << stream << std::endl

#define WARN_STREAM( stream ) \
std::cout << "Warning: " << stream << std::endl

#define ERROR_STREAM( stream ) \
std::cout << "Error: " << stream << std::endl

static void printErrorAndAbort( const std::string & error )
{
    std::cout << error << std::endl;
    abort();
}

#define FATAL_STREAM( stream ) \
printErrorAndAbort( std::string( "Fatal error: " ) + stream )

using namespace std;
using namespace cv;

vector<string> get_arguments(int argc, char **argv)
{

	vector<string> arguments;

	for(int i = 0; i < argc; ++i)
	{
		arguments.push_back(string(argv[i]));
	}
	return arguments;
}

int main (int argc, char **argv)
{

	vector<string> arguments = get_arguments(argc, argv);

	// Some initial parameters that can be overriden from command line	
	vector<string> files, depth_directories, pose_output_files, tracked_videos_output, landmark_output_files, landmark_3D_output_files;
	
	// By default try webcam 0
	int device = 0;

	// cx and cy aren't necessarilly in the image center, so need to be able to override it (start with unit vals and init them if none specified)
    float fx = 500, fy = 500, cx = 0, cy = 0;
			
	CLMTracker::CLMParameters clm_parameters(arguments);

	// Get the input output file parameters
	
	// Indicates that rotation should be with respect to camera plane or with respect to camera
	bool use_camera_plane_pose;
	CLMTracker::get_video_input_output_params(files, depth_directories, pose_output_files, tracked_videos_output, landmark_output_files, landmark_3D_output_files, use_camera_plane_pose, arguments);
	// Get camera parameters
	CLMTracker::get_camera_params(device, fx, fy, cx, cy, arguments);    
	
	// The modules that are being used for tracking
	CLMTracker::CLM clm_model(clm_parameters.model_location);	
	
	CLMTracker::CLMParameters clm_parameters_eye;
	clm_parameters_eye.validate_detections = false;
	vector<int> windows_large;
	windows_large.push_back(9);
	windows_large.push_back(7);
	windows_large.push_back(5);
	vector<int> windows_small;
	// TODO these might differ for real and synth
	windows_small.push_back(5);
	windows_small.push_back(5);
	windows_small.push_back(5);
	clm_parameters_eye.window_sizes_init = windows_large;
	clm_parameters_eye.window_sizes_small = windows_small;
	clm_parameters_eye.window_sizes_current = windows_large;

	clm_parameters_eye.model_location = "model_eye/main_ccnf_synth_right.txt";
	CLMTracker::CLM clm_right_eye_model(clm_parameters_eye.model_location);

	clm_parameters_eye.model_location = "model_eye/main_ccnf_synth_left.txt";
	CLMTracker::CLM clm_left_eye_model(clm_parameters_eye.model_location);

	// The correspondences in the main model
	std::vector<int> right_eye_inds;
	right_eye_inds.push_back(42); right_eye_inds.push_back(43); right_eye_inds.push_back(44);
	right_eye_inds.push_back(45); right_eye_inds.push_back(46);	right_eye_inds.push_back(47);

	std::vector<int> left_eye_inds;
	left_eye_inds.push_back(36); left_eye_inds.push_back(37); left_eye_inds.push_back(38);
	left_eye_inds.push_back(39); left_eye_inds.push_back(40);	left_eye_inds.push_back(41);

	// The specialist correspondences
	std::vector<int> right_eye_inds_sp;
	std::vector<int> left_eye_inds_sp;
	if(clm_left_eye_model.pdm.NumberOfPoints() == 6)
	{
		right_eye_inds_sp.push_back(0); right_eye_inds_sp.push_back(1); right_eye_inds_sp.push_back(2);
		right_eye_inds_sp.push_back(3); right_eye_inds_sp.push_back(4); right_eye_inds_sp.push_back(5);
		left_eye_inds_sp.push_back(0); left_eye_inds_sp.push_back(1); left_eye_inds_sp.push_back(2);
		left_eye_inds_sp.push_back(3); left_eye_inds_sp.push_back(4); left_eye_inds_sp.push_back(5);
		clm_parameters_eye.reg_factor = 0.02;
		clm_parameters_eye.sigma = 0.5;
	}
	else
	{
		right_eye_inds_sp.push_back(8); right_eye_inds_sp.push_back(10); right_eye_inds_sp.push_back(12);
		right_eye_inds_sp.push_back(14); right_eye_inds_sp.push_back(16); right_eye_inds_sp.push_back(18);
		left_eye_inds_sp.push_back(8); left_eye_inds_sp.push_back(10); left_eye_inds_sp.push_back(12);
		left_eye_inds_sp.push_back(14); left_eye_inds_sp.push_back(16); left_eye_inds_sp.push_back(18);
		clm_parameters_eye.reg_factor = 2;
		clm_parameters_eye.sigma = 4.0;
	}

	// If multiple video files are tracked, use this to indicate if we are done
	bool done = false;	
	int f_n = -1;

	// If cx (optical axis centre) is undefined will use the image size/2 as an estimate
	bool cx_undefined = false;
	if(cx == 0 || cy == 0)
	{
		cx_undefined = true;
	}		

	while(!done) // this is not a for loop as we might also be reading from a webcam
	{
		
		string current_file;

		// We might specify multiple video files as arguments
		if(files.size() > 0)
		{
			f_n++;			
		    current_file = files[f_n];
		}
		else
		{
			// If we want to write out from webcam
			f_n = 0;
		}

		bool use_depth = !depth_directories.empty();	

		// Do some grabbing
		VideoCapture video_capture;
		if( current_file.size() > 0 )
		{
			INFO_STREAM( "Attempting to read from file: " << current_file );
			video_capture = VideoCapture( current_file );
		}
		else
		{
			INFO_STREAM( "Attempting to capture from device: " << device );
			video_capture = VideoCapture( device );

			// Read a first frame often empty in camera
			Mat captured_image;
			video_capture >> captured_image;
		}

		if( !video_capture.isOpened() ) FATAL_STREAM( "Failed to open video source" );
		else INFO_STREAM( "Device or file opened");

		Mat captured_image;
		video_capture >> captured_image;		
		
		// If optical centers are not defined just use center of image
		if(cx_undefined)
		{
			cx = captured_image.cols / 2.0f;
			cy = captured_image.rows / 2.0f;
		}
	
		// Creating output files
		std::ofstream pose_output_file;
		if(!pose_output_files.empty())
		{
			pose_output_file.open (pose_output_files[f_n], ios_base::out);
		}
	
		std::ofstream landmarks_output_file;		
		if(!landmark_output_files.empty())
		{
			landmarks_output_file.open(landmark_output_files[f_n], ios_base::out);
		}

		std::ofstream landmarks_3D_output_file;
		if(!landmark_3D_output_files.empty())
		{
			landmarks_3D_output_file.open(landmark_3D_output_files[f_n], ios_base::out);
		}
	
		int frame_count = 0;
		
		// saving the videos
		VideoWriter writerFace;
		if(!tracked_videos_output.empty())
		{
			writerFace = VideoWriter(tracked_videos_output[f_n], CV_FOURCC('D','I','V','X'), 30, captured_image.size(), true);		
		}
		
		// For measuring the timings
		int64 t1,t0 = cv::getTickCount();
		double fps = 10;

		INFO_STREAM( "Starting tracking");
		while(!captured_image.empty())
		{		

			// Reading the images
			Mat_<float> depth_image;
			Mat_<uchar> grayscale_image;

			if(captured_image.channels() == 3)
			{
				cvtColor(captured_image, grayscale_image, CV_BGR2GRAY);				
			}
			else
			{
				grayscale_image = captured_image.clone();				
			}
		
			// Get depth image
			if(use_depth)
			{
				char* dst = new char[100];
				std::stringstream sstream;

				sstream << depth_directories[f_n] << "\\depth%05d.png";
				sprintf(dst, sstream.str().c_str(), frame_count + 1);
				// Reading in 16-bit png image representing depth
				Mat_<short> depth_image_16_bit = imread(string(dst), -1);

				// Convert to a floating point depth image
				if(!depth_image_16_bit.empty())
				{
					depth_image_16_bit.convertTo(depth_image, CV_32F);
				}
				else
				{
					WARN_STREAM( "Can't find depth image" );
				}
			}
			
			// The actual facial landmark detection / tracking
			bool detection_success = CLMTracker::DetectLandmarksInVideo(grayscale_image, depth_image, clm_model, clm_parameters);

			// If face detection succeeded use a specialised model(s)
			if(clm_model.detection_success)
			{

				// Do the eye landmark detection refinement
				// First extract eye landmarks				

				// TODO if too far from orig reinit

				// TODO the wide params might be unnecessary if not doing tracking
				if(!clm_left_eye_model.detection_success || !clm_right_eye_model.detection_success)
				{
					int n_right_eye_points = clm_right_eye_model.pdm.NumberOfPoints();
					Mat_<double> eye_locs(n_right_eye_points * 2, 1, 0.0);
					for (size_t eye_ind = 0; eye_ind < right_eye_inds.size(); ++eye_ind)
					{
						eye_locs.at<double>(right_eye_inds_sp[eye_ind]) = clm_model.detected_landmarks.at<double>(right_eye_inds[eye_ind]);
						eye_locs.at<double>(right_eye_inds_sp[eye_ind] + n_right_eye_points) = clm_model.detected_landmarks.at<double>(right_eye_inds[eye_ind] + clm_model.pdm.NumberOfPoints());
					}

					// First need to estimate the local and global parameters from the landmark detection of CLM
					clm_right_eye_model.params_local.setTo(0);
					clm_right_eye_model.pdm.CalcParams(clm_right_eye_model.params_global, clm_right_eye_model.params_local, eye_locs);		

					int n_left_eye_points = clm_left_eye_model.pdm.NumberOfPoints();
					eye_locs.create(n_left_eye_points * 2, 1);
					for (size_t eye_ind = 0; eye_ind < left_eye_inds.size(); ++eye_ind)
					{
						eye_locs.at<double>(left_eye_inds_sp[eye_ind]) = clm_model.detected_landmarks.at<double>(left_eye_inds[eye_ind]);
						eye_locs.at<double>(left_eye_inds_sp[eye_ind] + n_left_eye_points) = clm_model.detected_landmarks.at<double>(left_eye_inds[eye_ind] + clm_model.pdm.NumberOfPoints());
					}

					// First need to estimate the local and global parameters from the landmark detection of CLM
					clm_left_eye_model.params_local.setTo(0);
					clm_left_eye_model.pdm.CalcParams(clm_left_eye_model.params_global, clm_left_eye_model.params_local, eye_locs);		


					clm_parameters_eye.window_sizes_current = clm_parameters_eye.window_sizes_init;
				}
				else
				{
					clm_parameters_eye.window_sizes_current = clm_parameters_eye.window_sizes_small;
				}

				// Do the actual landmark detection (and keep it only if successful)
				clm_right_eye_model.DetectLandmarks(grayscale_image, depth_image, clm_parameters_eye);
				clm_left_eye_model.DetectLandmarks(grayscale_image, depth_image, clm_parameters_eye);

				double cumm_err_left = 0;
				double cumm_err_right = 0;

				// Reincorporate the models into main tracker
				for (size_t eye_ind = 0; eye_ind < right_eye_inds.size(); ++eye_ind)
				{
					double x_err = cv::abs(clm_model.detected_landmarks.at<double>(right_eye_inds[eye_ind]) - clm_right_eye_model.detected_landmarks.at<double>(right_eye_inds_sp[eye_ind]));
					double y_err = cv::abs(clm_model.detected_landmarks.at<double>(right_eye_inds[eye_ind] + clm_model.pdm.NumberOfPoints()) - clm_right_eye_model.detected_landmarks.at<double>(right_eye_inds_sp[eye_ind] + clm_right_eye_model.pdm.NumberOfPoints()));
					cumm_err_right += cv::sqrt(x_err * x_err + y_err * y_err);

					x_err = cv::abs(clm_model.detected_landmarks.at<double>(left_eye_inds[eye_ind]) - clm_left_eye_model.detected_landmarks.at<double>(left_eye_inds_sp[eye_ind]));
					y_err = cv::abs(clm_model.detected_landmarks.at<double>(left_eye_inds[eye_ind] + clm_model.pdm.NumberOfPoints()) - clm_left_eye_model.detected_landmarks.at<double>(left_eye_inds_sp[eye_ind] + clm_left_eye_model.pdm.NumberOfPoints()));
					cumm_err_left += cv::sqrt(x_err * x_err + y_err * y_err);

					clm_model.detected_landmarks.at<double>(right_eye_inds[eye_ind]) = clm_right_eye_model.detected_landmarks.at<double>(right_eye_inds_sp[eye_ind]);
					clm_model.detected_landmarks.at<double>(right_eye_inds[eye_ind] + clm_model.pdm.NumberOfPoints()) = clm_right_eye_model.detected_landmarks.at<double>(right_eye_inds_sp[eye_ind] + clm_right_eye_model.pdm.NumberOfPoints());
					clm_model.detected_landmarks.at<double>(left_eye_inds[eye_ind]) = clm_left_eye_model.detected_landmarks.at<double>(left_eye_inds_sp[eye_ind]);
					clm_model.detected_landmarks.at<double>(left_eye_inds[eye_ind] + clm_model.pdm.NumberOfPoints()) = clm_left_eye_model.detected_landmarks.at<double>(left_eye_inds_sp[eye_ind] + clm_left_eye_model.pdm.NumberOfPoints());
				}

				if(cumm_err_left / clm_model.params_global[0] > 30)
				{
					clm_left_eye_model.Reset();
				}
				if(cumm_err_right / clm_model.params_global[0] > 30)
				{
					clm_right_eye_model.Reset();
				}
				//cout << cumm_err_left / clm_model.params_global[0] << endl;
				//cout << cumm_err_right / clm_model.params_global[0] << endl;

				clm_model.pdm.CalcParams(clm_model.params_global, clm_model.params_local, clm_model.detected_landmarks);		
				clm_model.pdm.CalcShape2D(clm_model.detected_landmarks, clm_model.params_local, clm_model.params_global);
			}
			else
			{
				clm_right_eye_model.Reset();
				clm_left_eye_model.Reset();
				clm_right_eye_model.detection_success = false;
				clm_left_eye_model.detection_success = false;
			}

			// Work out the pose of the head from the tracked model
			Vec6d pose_estimate_CLM;
			if(use_camera_plane_pose)
			{
				pose_estimate_CLM = CLMTracker::GetCorrectedPoseCameraPlane(clm_model, fx, fy, cx, cy, clm_parameters);
			}
			else
			{
				pose_estimate_CLM = CLMTracker::GetCorrectedPoseCamera(clm_model, fx, fy, cx, cy, clm_parameters);
			}

			// Visualising the results
			// Drawing the facial landmarks on the face and the bounding box around it if tracking is successful and initialised
			double detection_certainty = clm_model.detection_certainty;

			double visualisation_boundary = 0.2;
			
			// Only draw if the reliability is reasonable, the value is slightly ad-hoc
			if(detection_certainty < visualisation_boundary)
			{
				//CLMTracker::Draw(captured_image, clm_model);

				CLMTracker::Draw(captured_image, clm_left_eye_model);
				CLMTracker::Draw(captured_image, clm_right_eye_model);

				if(detection_certainty > 1)
					detection_certainty = 1;
				if(detection_certainty < -1)
					detection_certainty = -1;

				detection_certainty = (detection_certainty + 1)/(visualisation_boundary +1);

				// A rough heuristic for box around the face width
				int thickness = (int)std::ceil(2.0* ((double)captured_image.cols) / 640.0);
				
				Vec6d pose_estimate_to_draw = CLMTracker::GetCorrectedPoseCameraPlane(clm_model, fx, fy, cx, cy, clm_parameters);

				// Draw it in reddish if uncertain, blueish if certain
				//CLMTracker::DrawBox(captured_image, pose_estimate_to_draw, Scalar((1-detection_certainty)*255.0,0, detection_certainty*255), thickness, fx, fy, cx, cy);

			}

			// Work out the framerate
			if(frame_count % 10 == 0)
			{      
				t1 = cv::getTickCount();
				fps = 10.0 / (double(t1-t0)/cv::getTickFrequency()); 
				t0 = t1;
			}
			
			// Write out the framerate on the image before displaying it
			char fpsC[255];
			sprintf(fpsC, "%d", (int)fps);
			string fpsSt("FPS:");
			fpsSt += fpsC;
			cv::putText(captured_image, fpsSt, cv::Point(10,20), CV_FONT_HERSHEY_SIMPLEX, 0.5, CV_RGB(255,0,0));		
			
			if(!clm_parameters.quiet_mode)
			{
				namedWindow("tracking_result",1);		
				imshow("tracking_result", captured_image);

				if(!depth_image.empty())
				{
					// Division needed for visualisation purposes
					imshow("depth", depth_image/2000.0);
				}
			}

			// Output the detected facial landmarks
			if(!landmark_output_files.empty())
			{
				landmarks_output_file << frame_count + 1 << " " << detection_success;
				for (int i = 0; i < clm_model.pdm.NumberOfPoints() * 2; ++ i)
				{
					landmarks_output_file << " " << clm_model.detected_landmarks.at<double>(i) << " ";
				}
				landmarks_output_file << endl;
			}

			// Output the detected facial landmarks
			if(!landmark_3D_output_files.empty())
			{
				landmarks_3D_output_file << frame_count + 1 << " " << detection_success;
				Mat_<double> shape_3D = clm_model.GetShape(fx, fy, cx, cy);
				for (int i = 0; i < clm_model.pdm.NumberOfPoints() * 3; ++i)
				{
					landmarks_3D_output_file << " " << shape_3D.at<double>(i);
				}
				landmarks_3D_output_file << endl;
			}

			// Output the estimated head pose
			if(!pose_output_files.empty())
			{
				double confidence = 0.5 * (1 - detection_certainty);
				pose_output_file << frame_count + 1 << " " << confidence << " " << detection_success << " " << pose_estimate_CLM[0] << " " << pose_estimate_CLM[1] << " " << pose_estimate_CLM[2] << " " << pose_estimate_CLM[3] << " " << pose_estimate_CLM[4] << " " << pose_estimate_CLM[5] << endl;
			}				

			// output the tracked video
			if(!tracked_videos_output.empty())
			{		
				writerFace << captured_image;
			}

			video_capture >> captured_image;
		
			// detect key presses
			char character_press = cv::waitKey(1);
			
			// restart the tracker
			if(character_press == 'r')
			{
				clm_model.Reset();
				clm_left_eye_model.Reset();
				clm_right_eye_model.Reset();
			}
			// quit the application
			else if(character_press=='q')
			{
				return(0);
			}

			// Update the frame count
			frame_count++;

		}
		
		frame_count = 0;

		// Reset the model, for the next video
		clm_model.Reset();

		pose_output_file.close();
		landmarks_output_file.close();

		// break out of the loop if done with all the files (or using a webcam)
		if(f_n == files.size() -1 || files.empty())
		{
			done = true;
		}
	}

	return 0;
}


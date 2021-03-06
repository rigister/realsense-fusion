
#ifdef ENABLE_INPUT_REALSENSE
#include "frame.h"
#include "realsense_input.h"

#include <iostream>

RealSenseInput::RealSenseInput(const rs2::config &config)
{
	try
	{
		auto profile = pipe.start(config);
		intrinsics = profile.get_stream(RS2_STREAM_DEPTH).as<rs2::video_stream_profile>().get_intrinsics();
		IntrinsicsColor = profile.get_stream(RS2_STREAM_COLOR).as<rs2::video_stream_profile>().get_intrinsics();
		auto sensor = profile.get_device().first<rs2::depth_sensor>();
		depth_scale = sensor.get_depth_scale();
	}
	catch(const rs2::error &e)
	{
		std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
		throw std::exception();
	}
}

RealSenseInput::~RealSenseInput()
{
	pipe.stop();
}


bool RealSenseInput::WaitForFrame(Frame *frame)
{
	try
	{
		auto frames = pipe.wait_for_frames();
		auto depth = frames.get_depth_frame();
		if (depth.get_profile().stream_type() != RS2_STREAM_DEPTH || depth.get_profile().format() != RS2_FORMAT_Z16)
		{
			std::cerr << "Frame from RealSense has invalid stream type or format." << std::endl;
			return false;
		}
		if (filters_active)
		{
			
			//rs2::decimation_filter dec_filter;  // Decimation - "Intelligently reduces the resolution of a depth frame." dont sure if we want that
			rs2::spatial_filter spat_filter;    // Spatial    - edge-preserving spatial smoothing
			rs2::temporal_filter temp_filter;   // Temporal   - reduces temporal noise
			//depth = dec_filter.process(depth);
			depth = spat_filter.process(depth);
			depth = temp_filter.process(depth);
			
		}
		frame->SetDepthMap(depth.get_width(), depth.get_height(), (GLushort *)depth.get_data(), depth_scale,
				Eigen::Vector2f(intrinsics.fx, intrinsics.fy), Eigen::Vector2f(intrinsics.ppx, intrinsics.ppy));

		if (color_active)
		{
			auto color = frames.get_color_frame();
			if (color.get_profile().stream_type() != RS2_STREAM_COLOR || color.get_profile().format() != RS2_FORMAT_RGB8)
			{
				std::cerr << "Frame from RealSense has invalid stream type or format." << std::endl;
				return false;
			}
			frame->SetColorMap(color.get_width(), color.get_height(), (GLushort *)color.get_data(),
				Eigen::Vector2f(IntrinsicsColor.fx, IntrinsicsColor.fy), Eigen::Vector2f(IntrinsicsColor.ppx, IntrinsicsColor.ppy));
		}
		
		
	}
	catch(const rs2::error &e)
	{
		std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
		return false;
	}

	/*auto color = frames.get_color_frame();
	// For cameras that don't have RGB sensor, we'll map the pointcloud to infrared instead of color
	if (!color)
		color = frames.get_infrared_frame();
	pc.map_to(color);*/

	//auto cloud = frame->GetCloud();
	//PointsToPCL(points, cloud);


	return true;
}


/*void PointsToPCL(const rs2::points& points, pcl::PointCloud<pcl::PointXYZ>::Ptr &cloud);
void PointsToPCL(const rs2::points& points, pcl::PointCloud<pcl::PointXYZ>::Ptr &cloud)
{
	auto sp = points.get_profile().as<rs2::video_stream_profile>();
	cloud->width = sp.width();
	cloud->height = sp.height();
	cloud->is_dense = false;
	cloud->points.resize(points.size());
	auto ptr = points.get_vertices();
	for (auto& p : cloud->points)
	{
		p.x = ptr->x;
		p.y = -ptr->y;
		p.z = -ptr->z;
		ptr++;
	}
}*/

#endif
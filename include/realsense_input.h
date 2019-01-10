
#ifdef ENABLE_INPUT_REALSENSE
#ifndef _REALSENSE_INPUT_H
#define _REALSENSE_INPUT_H

#include <librealsense2/rs.hpp>

#include "input.h"

class RealSenseInput : public Input
{
	private:
		rs2::pipeline pipe;
		//rs2::pointcloud pc;
		//rs2::points points;

		float depth_scale;

	public:
		RealSenseInput();
		~RealSenseInput();

		rs2_intrinsics intrinsics;
		bool WaitForFrame(Frame *frame) override;

		float GetPpx() { return intrinsics.ppx; }
		float GetPpy() { return intrinsics.ppy; }
		float GetFx() { return intrinsics.fx; }
		float GetFy() { return intrinsics.fy; }
};

#endif //INPUT_H
#endif
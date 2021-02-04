#pragma once

#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <stdio.h>
#include <opencv2/core/cuda.hpp>

#include "Volume.h"
namespace Wrapper
{
	void wrapper(cv::cuda::GpuMat &img, Volume &model);
	void updateReconstruction(Volume &model,
														const CameraParameters &cameraParams,
														const float *const depthMap,
														const MatrixXf &poseInverse);
	void rayCast(Volume &model,
							 const CameraParameters &cameraParams,
							 const MatrixXf &cameraPose);

	void poseEstimation(const Matrix4f &modelToFramePose, const CameraParameters &cameraParams, cv::cuda::GpuMat surfacePoints, cv::cuda::GpuMat surfaceNormals,
		PointCloud &pcd);
	Matrix4f estimatePosePointToPlane(const std::vector<Vector3f> &sourcePoints, const std::vector<Vector3f> &targetPoints, const std::vector<Vector3f> &targetNormals);

} // namespace Wrapper
//
// Created by kerem on 28/12/2020.
//
#include "Volume.h"
#include "CameraParameters.h"
#include "PointCloud.h"
#include <vector>
#include <opencv2/opencv.hpp>
Volume::Volume(int xdim, int ydim, int zdim, float voxelSize, float minDepth) : voxSize(voxelSize), gridSize(Vector3i(xdim, ydim, zdim)), minimumDepth(minDepth)
{
    grid = new Voxel[xdim * ydim * zdim];
}

Volume::~Volume()
{
    delete grid;
}

// TODO: Return reference to pointcloud not a copy
PointCloud Volume::getPointCloud()
{
    return pcd;
}

void Volume::setPointCloud(PointCloud &pointCloud)
{
    pcd = pointCloud;
}

const Voxel *Volume::get(int x, int y, int z)
{
    x+=(gridSize.x()+1)/2;
    y+=(gridSize.y()+1)/2;
    z+=(gridSize.z()+1)/2;

    
    return &grid[x + gridSize.x() * (y + gridSize.z() * z)];
}

void Volume::set(int x, int y, int z, const Voxel &value)
{
    x+=(gridSize.x()+1)/2;
    y+=(gridSize.y()+1)/2;
    z+=(gridSize.z()+1)/2;
    grid[x + gridSize.x() * (y + gridSize.z() * z)].distance = value.distance;
    grid[x + gridSize.x() * (y + gridSize.z() * z)].weight = value.weight;
}

void Volume::rayCast(const MatrixXf &cameraPose, const CameraParameters &params,std::vector<cv::Point3d> &rays)
{
    // TODO: Search for possible optimizations here...
    std::vector<Vector3f> surfacePoints;
    std::vector<Vector3f> surfaceNormals;

    for (int x = 0; x < params.depthImageWidth; x++)
    {
        for (int y = 0; y < params.depthImageHeight; y++)
        {
            Vector3f currPoint,currNormal;
            bool exists = pointRay(cameraPose, params, x, y, currPoint,currNormal,rays);
            if (exists)
            {
                surfacePoints.push_back(currPoint);
                surfaceNormals.push_back(currNormal);
            }
        }
    }
    std::cout<<"Surface predicted with " << surfacePoints.size() << " vertices \n";
    pcd = PointCloud(surfacePoints,surfaceNormals);
    
}
bool Volume::pointRay(const MatrixXf &cameraPose, const CameraParameters &params,
                      int x, int y, Vector3f &surfacePoint,Vector3f &currNormal,std::vector<cv::Point3d> &rays)
{
    const Vector3f pixelInCameraCoords(
        (x - params.cX) / params.fovX,
        (y - params.cY) / params.fovY,
        1.0);

    Vector3f currPositionInCameraWorld = pixelInCameraCoords.normalized() * minimumDepth;
    // TODO: Check if - or +
    // Translate point to 3D world
    currPositionInCameraWorld -= cameraPose.block<3, 1>(0, 3);

    Vector3f rayStepVec = pixelInCameraCoords.normalized() * voxSize;
    // Rotate rayStepVec to 3D world
    rayStepVec = cameraPose.block<3, 3>(0, 0) * rayStepVec;

    const Vector3f shiftWorldCenterToVoxelCoords(
        gridSize.x() / 2,
        gridSize.y() / 2,
        gridSize.z() / 2);

    //change of basis
    Vector3f voxelInGridCoords = (currPositionInCameraWorld) / voxSize;//+ shiftWorldCenterToVoxelCoords - Vector3f(0.5f,0.5f,0.5f);

    // TODO: Interpolation for points
    float currTSDF = get((int)voxelInGridCoords.x(),
                         (int)voxelInGridCoords.y(),
                         (int)voxelInGridCoords.z())
                         ->distance;


    int sign =1 ;
    if(currTSDF < 0 )
        sign = -1;
    // TODO: Is it necessary to check voxelInGridCoords.x.y.z < 0 ?


    // std::cout <<"LEZ GO " <<currTSDF<< std::endl;
    //cv::waitKey(0);
    //TODO make this proper
    int prevSign = sign;
    while ((prevSign == sign) && voxelInGridCoords.x() < gridSize.x()/2 &&
           voxelInGridCoords.y() < gridSize.y()/2 && voxelInGridCoords.z() < gridSize.z()/2
           && voxelInGridCoords.x() >-gridSize.x()/2 &&
           voxelInGridCoords.y() >- gridSize.y()/2 && voxelInGridCoords.z() >- gridSize.z()/2)
    {
         voxelInGridCoords = (currPositionInCameraWorld) / voxSize;// + shiftWorldCenterToVoxelCoords - Vector3f(0.5f,0.5f,0.5f);
        currPositionInCameraWorld += rayStepVec;

        // TODO: Interpolation for points...
        currTSDF = get((int)voxelInGridCoords.x(),
                       (int)voxelInGridCoords.y(),
                       (int)voxelInGridCoords.z())
                       ->distance;


        prevSign = sign;
        
        if(currTSDF<0)
            sign = -1;
        else
        {
            sign = 1;
        }
        

    }

    // TODO: Is it necessary to check voxelInGridCoords.x.y.z < 0 ?
    if ( (sign != prevSign) && voxelInGridCoords.x() < gridSize.x()/2 &&
           voxelInGridCoords.y() < gridSize.y()/2 && voxelInGridCoords.z() < gridSize.z()/2
           && voxelInGridCoords.x() >-gridSize.x()/2 &&
           voxelInGridCoords.y() >- gridSize.y()/2 && voxelInGridCoords.z() >- gridSize.z()/2 )
    {
        surfacePoint = currPositionInCameraWorld;
        
        return true;
    }   
    else
    {
        return false;
    }


    Vector3f neighbor = voxelInGridCoords;
    neighbor.x() += 1;
    if (neighbor.x() >= gridSize.x() - 1)
        return false;
    const float Fx1 = get((int)neighbor.x(),
                       (int)neighbor.y(),
                       (int)neighbor.z())
                       ->distance;

    neighbor = voxelInGridCoords;
    neighbor.x() -= 1;
    if (neighbor.x() < 1)
        return false;
    const float Fx2 = get((int)neighbor.x(),
                       (int)neighbor.y(),
                       (int)neighbor.z())
                       ->distance;

    currNormal.x() = (Fx1 - Fx2);

    neighbor = voxelInGridCoords;
    neighbor.y() += 1;
    if (neighbor.y() >= gridSize.y() - 1)
        return false;
    const float Fy1 = get((int)neighbor.x(),
                       (int)neighbor.y(),
                       (int)neighbor.z())
                       ->distance;

    neighbor = voxelInGridCoords;
    neighbor.y() -= 1;
    if (neighbor.y() < 1)
        return false;
    const float Fy2 = get((int)neighbor.x(),
                       (int)neighbor.y(),
                       (int)neighbor.z())
                       ->distance;

    currNormal.y() = (Fy1 - Fy2);

    neighbor = voxelInGridCoords;
    neighbor.z() += 1;
    if (neighbor.z() >= gridSize.z() - 1)
        return false;
    const float Fz1 = get((int)neighbor.x(),
                       (int)neighbor.y(),
                       (int)neighbor.z())
                       ->distance;

    neighbor = voxelInGridCoords;
    neighbor.z() -= 1;
    if (neighbor.z() < 1)
        return false;
    const float Fz2 = get((int)neighbor.x(),
                       (int)neighbor.y(),
                       (int)neighbor.z())
                       ->distance;

    currNormal.z() = (Fz1 - Fz2);

    if (currNormal.norm() == 0)
        return false;

    currNormal.normalize();

    return true;
}

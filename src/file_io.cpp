/* Distributed Directional Fast Multipole Method
   Copyright (C) 2013 Austin Benson, Lexing Ying, and Jack Poulson

 This file is part of DDFMM.

    DDFMM is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    DDFMM is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with DDFMM.  If not, see <http://www.gnu.org/licenses/>. */
#include "file_io.h"
#include "vec3t.hpp"
#include "commoninc.hpp"
#include "nummat.hpp"

#include <algorithm>
#include <stdio>
#include <string>
#include <vector>

#include <math.h>
#include <stdio.h>
#include <string.h>

#define MAX_LINE_LENGTH 256
#define MAX_FILE_NAME_LENGTH 256

int ReadWrl(std::string fname, std::vector<Point3>& points,
            std::vector<Point3>& coords) {
    char filename[MAX_FILE_NAME_LENGTH];
    sprintf(filename, "data/%s", fname.c_str());
    FILE *geom_file;
    geom_file = fopen (filename, "r");
    if (geom_file == NULL) {
	std::cerr << "Failed to open geometry file: " << filename << std::endl;
	return -1;
    }

    points.clear();
    coords.clear();
    char pts[MAX_LINE_LENGTH];
    fscanf(geom_file, "%s", &pts);
    if (strncmp(pts, "points", 6)) {
	std::cerr << "Did not find 'points' at the beginning of geometry file"
                  << std::endl;
	return -1;
    }

    // Format of points is:
    //         x1   x2   x3
    float x1, x2, x3;
    while (fscanf(geom_file, "%f %f %f", &x1, &x2, &x3) == 3) {
	points.push_back(Point3(x1, x2, x3));
    }
    // Should fail on an empty line.  The next line is coords.
    fscanf(geom_file, "%s", &pts);
    if (strncmp(pts, "coords", 6)) {
	std::cerr << "Did not find 'coords' in geometry file" << std::endl;
	return -1;
    }

    // Format of coords is:
    //          xcoord ycoord zcoord -1
    int c1, c2, c3, c4;
    while (fscanf(geom_file, "%d %d %d %d", &c1, &c2, &c3, &c4) == 4) {
	if (c4 != -1) {
	    std::cerr << "Expected -1 in fourth coordinate" << std::endl;
	    return -1;	    
	}
	coords.push_back(Point3(c1, c2, c3));
    }

    // Normalize points to [-1, 1]
    // Find min and max points along each row
    Point3 min(points[0][0], points[0][1], points[0][2]);
    Point3 max(points[0][0], points[0][1], points[0][2]);
    for (int i = 0; i < points.size(); ++i) {
	Point3 p = points[i];
	for (int j = 0; j < 3; ++j) {
	    min[j] = std::min(min[j], p[j]);
	    max[j] = std::max(max[j], p[j]);
	}
    }
    std::cout << "min: " << min << std::endl;
    std::cout << "max: " << max << std::endl;
    Point3 mid = max + min;
    mid /= 2;
    std::cout << "midpoint: " << mid << std::endl;
    Point3 diff = max - min;
    diff /= 2;
    double avg_diff = std::max(std::max(diff[0], diff[1]), diff[2]);
    std::cout << "avg_diff: " << avg_diff << std::endl;
    for (int i = 0; i < points.size(); ++i) {
	points[i] -= mid;
	points[i] /= avg_diff;
    }

    return 0;
}

// Sort pairs of ints by the second item in the pair in descending order.
bool CompPairDescend(std::pair<int, int> a, std::pair<int, int> b) {
    return a.second > b.second;
}

// Sort pairs of ints by the second item in the pair in ascending order.
bool CompPairAscend(std::pair<int, int> a, std::pair<int, int> b) {
    return a.second < b.second;
}

int LloydsAlgorithm(std::vector<int>& assignment, int num_its, int NCPU,
                    std::vector<Point3>& centers, std::vector<int> num_points,
                    std::vector<Point3>& proc_centers, 
		    std::vector< std::pair<int, int> >& sorted_num_points,
                    int num_coords) {
    for (int iter = 0; iter < num_its; ++iter) {
	// Fill in distances
        NumMat<double> distances(num_coords, NCPU);
	for (int i = 0; i < NCPU; ++i) {
	    Point3 proc_center = proc_centers[i];
	    for (int j = 0; j < num_coords; ++j) {
		Point3 curr_center = centers[j];
		Point3 diff = curr_center - proc_center;
		distances(j, i) = diff.l2();
	    }
	}
	
	assignment.insert(assignment.begin(), num_coords, -1);
	int total_points = 0;
	for (int i = 0; i < num_points.size(); ++i) {
	    total_points += num_points[i];
	}
	double avg_weight = ((double) total_points) / NCPU;
	std::vector<double> curr_weights;
	curr_weights.insert(curr_weights.begin(), NCPU, 0);
	
	for (int k = 0; k < num_coords; ++k) {
	    int curr_index = sorted_num_points[k].first;
	    int curr_points = sorted_num_points[k].second;
	    // Get distances from the k-th center
	    std::vector< std::pair<int, double> > dist;
	    for (int m = 0; m < NCPU; ++m) {
		dist.push_back(std::pair<int, double>(m, distances(curr_index, m)));
	    }
	    std::sort(dist.begin(), dist.end(), CompPairAscend);
	    for (int p = 0; p < NCPU; ++p) {
		int curr_proc = dist[p].first;
		if (curr_weights[curr_proc] <= avg_weight * 1.05) {
		    assignment[curr_index] = curr_proc;
		    curr_weights[curr_proc] += curr_points;
		    break;
		}
	    }
	}
	for (int i = 0; i < assignment.size(); ++i) {
	    if (assignment[i] < 0 || assignment[i] >= NCPU) {
		std::cerr << "Assignment incorrect!" << std::endl;
		return -1;
	    }
	}
    }
    return 0;
}

int AssignPoints(std::vector<int>& assignment, int NCPU,
                 std::vector<Point3>& centers, std::vector<int>& num_points,
                 int num_coords) {
    // Pick NCPU random centers
    if (NCPU > num_coords) {
	std::cerr << "More processors than coordinates!" << std::cout;
	return -1;
    }
    std::vector<Point3> tmp = centers;
    std::random_shuffle(tmp.begin(), tmp.end());
    std::vector<Point3> proc_centers(NCPU);
    for (int i = 0; i < NCPU; ++i) {
	proc_centers[i] = tmp[i];
    }

    std::vector< std::pair<int, int> > sorted_num_points;
    for (int i = 0; i < num_points.size(); ++i) {
	sorted_num_points.push_back(std::pair<int, int>(i, num_points[i]));
    }
    std::sort(sorted_num_points.begin(), sorted_num_points.end(),
              CompPairDescend);
    
    iC( LloydsAlgorithm(assignment, 4 * NCPU, NCPU, centers, num_points,
                        proc_centers, sorted_num_points, num_coords) );

    return 0;
}

int NewData(std::string fname, double K, double NPW, int NCPU, int NC) {
    iA (NCPU > 0 && NPW > 0 && NC > 0 && K >= 1);

    std::vector<Point3> points;
    std::vector<Point3> coords;
    ReadWrl(fname, points, coords);

    // Scaling of points
    for (int i = 0; i < points.size(); ++i) {
	points[i] *= (K / 2) * 0.875;
    }
    
    int num_coords = coords.size();
    std::vector<Point3> centers(num_coords);
    std::vector<int> num_points(num_coords);

    for (int k = 0; k < num_coords; ++k) {
	Point3 index = coords[k];
	Point3 p1 = points[index[0]];
	Point3 p2 = points[index[1]];
	Point3 p3 = points[index[2]];
	centers[k] = p1 + p2 + p3;
	centers[k] /= 3;

	// Compute number of points near that point
	Point3 a = p1 - p2;
	Point3 b = p2 - p3;
	Point3 c = p3 - p1;
        double s = (a.l2() + b.l2() + c.l2()) / 2;
	double area = sqrt(s * (s - a.l2()) * (s - b.l2()) * (s - c.l2()));
	num_points[k] = ceil(area * NPW * NPW);
    }

    std::vector<int> assignment;
    iC( AssignPoints(assignment, NCPU, centers, num_points, num_coords) );
    for (int i = 0; i < assignment.size(); ++i) {
	std::cout << assignment[i] << std::endl;
    }
}
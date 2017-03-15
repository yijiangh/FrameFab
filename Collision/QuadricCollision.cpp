#include "QuadricCollision.h"


QuadricCollision::QuadricCollision()
{

}


QuadricCollision::QuadricCollision(WireFrame *ptr_frame)
{
	ptr_frame_ = ptr_frame;
	divide_ = 60;

	int halfM = ptr_frame->SizeOfEdgeList() / 2;
	colli_map_.resize(halfM*halfM);
	for (int i = 0; i < halfM*halfM; i++)
	{
		colli_map_[i] = NULL;
	}
}


QuadricCollision::~QuadricCollision()
{
	int Nc = colli_map_.size();
	for (int i = 0; i < Nc; i++)
	{
		delete colli_map_[i];
		colli_map_[i] = NULL;
	}
}


void QuadricCollision::DetectCollision(WF_edge *target_e, DualGraph *ptr_subgraph, 
	vector<lld> &result_map)
{
	Init(result_map);
	target_e_ = target_e;

	/* collision with edge */
	int Nd = ptr_subgraph->SizeOfVertList();
	for (int i = 0; i < Nd; i++)
	{
		WF_edge *e = ptr_frame_->GetEdge(ptr_subgraph->e_orig_id(i));
		if (e != target_e_ && e != target_e_->ppair_)
		{
			DetectEdge(e, result_map);
		}
	}
}


void QuadricCollision::DetectCollision(WF_edge *target_e, WF_edge *order_e, 
	vector<lld> &result_map)
{
	Init(result_map);
	target_e_ = target_e;

	/* collision with edge */
	DetectEdge(order_e, result_map);
}


void QuadricCollision::DetectCollision(WF_edge *target_e,
	vector<WF_edge*> exist_edge, vector<GeoV3> &output)
{
	output.clear();

	double theta, phi;
	target_e_ = target_e;
	//North Point
	if (!DetectEdges(exist_edge, 0, 0))
		output.push_back(Orientation(0, 0));
	for (int j = 0; j < 3; j++)
	{
		for (int i = 0; i < divide_; i++)
		{
			if (i < 20)
			{
				theta = (j * 3 + 1)*18.0 / 180.0*F_PI;
				phi = i*18.0 / 180.0*F_PI;
			}

			if (i>19 && i < 40)
			{
				theta = (j * 3 + 2) * 18.0 / 180.0*F_PI;
				phi = (i - 20)*18.0 / 180.0*F_PI;
			}

			if (i>39)
			{
				theta = (j * 3 + 3)* 18.0 / 180.0*F_PI;
				phi = (i - 40)*18.0 / 180.0*F_PI;
			}
			if (DetectEdges(exist_edge, theta, phi))
				continue;
			output.push_back(Orientation(theta, phi));
		}
	}

	//South Point not consider
	/*if (!DetectEdges(exist_edge_, 0, 0))
	temp.push_back(Orientation(F_PI, 0));*/
}


void QuadricCollision::Init(vector<lld> &colli_map)
{
	lld temp = 0;
	colli_map.clear();
	colli_map.push_back(temp);
	colli_map.push_back(temp);
	colli_map.push_back(temp);
}


void QuadricCollision::DetectEdge(WF_edge *order_e, vector<lld> &result_map)
{
	if (Distance(order_e) > (extruder_.CyclinderLenth() + extruder_.Height()))
	{
		return;
	}
	
	int halfM = ptr_frame_->SizeOfEdgeList() / 2;
	int mi = order_e->ID() / 2 * halfM + target_e_->ID() / 2;
	if (colli_map_[mi] == NULL)
	{
		colli_map_[mi] = new vector<lld> ;
		Init(*colli_map_[mi]);

		double theta;								// angle with Z axis (rad)
		double phi;								// angle with X axis (rad)

		for (int j = 0; j < 3; j++)
		{
			for (int i = 0; i < divide_; i++)
			{
				if (i < 20)
				{
					theta = (j * 3 + 1)*18.0 / 180.0*F_PI;
					phi = i*18.0 / 180.0*F_PI;
				}

				if (i > 19 && i < 40)
				{
					theta = (j * 3 + 2) * 18.0 / 180.0*F_PI;
					phi = (i - 20)*18.0 / 180.0*F_PI;
				}

				if (i > 39)
				{
					theta = (j * 3 + 3)* 18.0 / 180.0*F_PI;
					phi = (i - 40)*18.0 / 180.0*F_PI;
				}
				lld mask = ((lld)1 << i);
				if (DetectBulk(order_e, theta, phi))
				{
					(*colli_map_[mi])[j] |= mask;
				}
			}
		}

		//North Point
		lld mask = ((lld)1 << 60);
		if (DetectBulk(order_e, 0, 0))
		{
			(*colli_map_[mi])[2] |= mask;
		}

		//South Point
		mask = ((lld)1 << 61);
		if (DetectBulk(order_e, F_PI, 0))
		{
			(*colli_map_[mi])[2] |= mask;
		}
	}


	for (int i = 0; i < 3; i++)
	{
		result_map[i] |= (*colli_map_[mi])[i];
	}
}


bool QuadricCollision::DetectEdges(vector<WF_edge*> exist_edge, double theta, double phi)
{
	for (int i = 0; i < exist_edge.size(); i++)
	{
		if (DetectBulk(exist_edge[i], theta, phi))
		{

			return true;
		}
	}
	return false;
}


bool QuadricCollision::DetectBulk(WF_edge *order_e, double theta, double phi)
{
	GeoV3 target_start = target_e_->pvert_->Position();
	GeoV3 target_end = target_e_->ppair_->pvert_->Position();
	GeoV3 order_start = order_e->pvert_->Position();
	GeoV3 order_end = order_e->ppair_->pvert_->Position();
	GeoV3 normal = Orientation(theta, phi);

	//0
	if (Parallel(normal, target_start - target_end))
	{
		if (ParallelCase(target_start, target_end, order_start, order_end, normal))
		{
			return true;
		}
		return false;
	}

	//1
	if ((target_start - order_end).norm() < GEO_EPS)
	{
		if ( SpecialCase(target_start,target_end,order_start,normal ))
		{
			return true;
		}
		return false;
	}

	if ((target_start - order_start).norm() < GEO_EPS)
	{
		if (SpecialCase(target_start, target_end, order_end, normal))
		{
			return true;
		}
		return false;
	}

	if ((target_end - order_end).norm() < GEO_EPS)
	{
		if (SpecialCase(target_end, target_start, order_start, normal))
		{
			return true;
		}
		return false;
	}

	if ((target_end - order_start).norm() < GEO_EPS)
	{
		if (SpecialCase(target_end, target_start, order_end, normal))
		{
			return true;
		}
		return false;
	}

	//2
	if (Case(target_start, target_end, order_start, order_end, normal))
	{
		return true;
	}

	return false;
}


bool QuadricCollision::DetectAngle(GeoV3 connect, GeoV3 end, GeoV3 target_end, GeoV3 normal)
{
	if (angle(normal, target_end - connect) < extruder_.Angle())
		return true;
	return false;
}


bool QuadricCollision::Case(GeoV3 target_start, GeoV3 target_end, 
	GeoV3 order_start, GeoV3 order_end, GeoV3 normal)
{
	

	//Cone
	if (DetectCone(target_start, normal, order_start, order_end))
		return true;

	if (DetectCone(target_end, normal, order_start, order_end))
		return true;

	//Cylinder
	if (DetectCylinder(target_start, normal, order_start, order_end))
		return true;

	if (DetectCylinder(target_end, normal, order_start, order_end))
		return true;

#ifdef STRICT_COLLISION
	//Top
	if (DetectTopCylinder(target_start, normal, order_start, order_end))
		return true;
	if (DetectTopCylinder(target_end, normal, order_start, order_end))
		return true;
#endif // STRICT_COLLISION



	//Face
	GenerateVolume(target_start, target_end, order_start, order_end, normal);
	for (int i = 0; i < bulk_.size(); i++)
	{
		if (DetectTriangle(bulk_[i], order_start, order_end))
			return true;
	}

	return false;
}

bool QuadricCollision::SpecialCase(GeoV3 connect, GeoV3 target_s, GeoV3 order_s, GeoV3 normal)
{


	if (angle(normal, order_s - connect) < extruder_.Angle())
	{
		
		return true;
	}

	if (DetectCone(target_s, normal, connect, order_s))
		return true;

	if (DetectCylinder(target_s, normal, connect, order_s))
		return true;
#ifdef STRICT_COLLISION
	//Top
	if (DetectTopCylinder(connect, normal, connect, order_s))
		return true;
	if (DetectTopCylinder(target_s, normal, connect, order_s))
		return true;
#endif
	//Face
	GenerateVolume(connect, target_s, order_s, normal);
	for (int i = 0; i < bulk_.size(); i++)
	{
		if (DetectTriangle(bulk_[i], connect, order_s))
			return true;
	}

	return false;

}





bool QuadricCollision::ParallelCase(GeoV3 target_start, GeoV3 target_end,
	GeoV3 order_start, GeoV3 order_end, GeoV3 normal)
{

	//Exception situation
	if ((target_start - order_end).norm() < GEO_EPS)
	{
		if (DetectAngle(target_start, target_end, order_start, normal))
		{
			return true;
		}
	}
	if ((target_start - order_start).norm() < GEO_EPS)
	{
		if (DetectAngle(target_start, target_end, order_end, normal))
		{

			return true;
		}
	}
	if ((target_end - order_end).norm() < GEO_EPS)
	{
		if (DetectAngle(target_end, target_start, order_start, normal))
		{

			return true;
		}
	}
	if ((target_end - order_start).norm() < GEO_EPS)
	{
		if (DetectAngle(target_end, target_start, order_end, normal))
		{

			return true;
		}
	}

	//Normal situation
	//Cone
	if (DetectCone(target_start, normal, order_start, order_end))
		return true;

	if (DetectCone(target_end, normal, order_start, order_end))
		return true;

	//Cylinder
	if (DetectCylinder(target_start, normal, order_start, order_end))
		return true;

	if (DetectCylinder(target_end, normal, order_start, order_end))
		return true;
#ifdef STRICT_COLLISION
	//Top
	if (DetectTopCylinder(target_start, normal, order_start, order_end))
		return true;
	if (DetectTopCylinder(target_end, normal, order_start, order_end))
		return true;
#endif
	return false;
}


bool QuadricCollision::DetectCone(GeoV3 start, GeoV3 normal, GeoV3 target_start, GeoV3 target_end)
{
	gte::Cone3<float> start_cone;
	start_cone.angle = extruder_.Angle();
	start_cone.height = extruder_.Height();

	gte::Ray3<float> start_ray;

	std::array<float, 3>s;
	s[0] = start.getX(); s[1] = start.getY(); s[2] = start.getZ();
	start_ray.origin = s;
	s[0] = normal.getX(); s[1] = normal.getY(); s[2] = normal.getZ();
	start_ray.direction = s;
	start_cone.ray = start_ray;
	gte::Segment<3, float> segment;
	segment = Seg(target_start, target_end);
	gte::FIQuery<float, gte::Segment<3, float>, gte::Cone3<float>> intersection;
	auto result = intersection(segment, start_cone);


	return result.intersect;

}

bool QuadricCollision::DetectCylinder(GeoV3 start, GeoV3 normal, GeoV3 target_start, GeoV3 target_end)
{


	gte::Cylinder3<float> cylinder;
	cylinder.axis;
	gte::Line3<float> cylinder_line;

	std::array<float, 3>s;

	GeoV3 cylin_center;
	cylin_center = start + normal*(extruder_.Height() + extruder_.CyclinderLenth()/2);

	s[0] = cylin_center.getX(); s[1] = cylin_center.getY(); s[2] = cylin_center.getZ();
	cylinder_line.origin = s;
	s[0] = normal.getX(); s[1] = normal.getY(); s[2] = normal.getZ();
	cylinder_line.direction = s;

	cylinder.axis = cylinder_line;

	cylinder.height = extruder_.CyclinderLenth();
	cylinder.radius = extruder_.Radii();


	gte::Segment<3, float> segment;
	segment = Seg(target_start, target_end);
	gte::FIQuery<float, gte::Segment<3, float>, gte::Cylinder3<float>> intersection;
	auto result = intersection(segment, cylinder);

	return result.intersect;
}

bool QuadricCollision::DetectTriangle(Triangle triangle, GeoV3 target_start, GeoV3 target_end)
{
	gte::Triangle<3, float> triangle_ = Tri(triangle.v0(), triangle.v1(), triangle.v2());

	gte::FIQuery<float, gte::Segment<3, float>, gte::Triangle3<float>> intersection;
	gte::Segment<3, float> segment = Seg(target_start, target_end);

	auto result = intersection(segment, triangle_);
	return result.intersect;
}


bool QuadricCollision::DetectTopCylinder(GeoV3 start, GeoV3 normal, GeoV3 target_start, GeoV3 target_end)
{
	gte::Cylinder3<float> cylinder;
	cylinder.axis;
	gte::Line3<float> cylinder_line;
	std::array<float, 3>s;
	GeoV3 cylin_center;
	cylin_center = start + normal*extruder_.TopCenter();
	s[0] = cylin_center.getX(); s[1] = cylin_center.getY(); s[2] = cylin_center.getZ();
	cylinder_line.origin = s;
	s[0] = normal.getX(); s[1] = normal.getY(); s[2] = normal.getZ();
	cylinder_line.direction = s;
	cylinder.axis = cylinder_line;

	cylinder.height = extruder_.ToolLenth();
	cylinder.radius = extruder_.TopRadii();


	gte::Segment<3, float> segment;
	segment = Seg(target_start, target_end);
	gte::FIQuery<float, gte::Segment<3, float>, gte::Cylinder3<float>> intersection;
	auto result = intersection(segment, cylinder);

	return result.intersect;
}



void QuadricCollision::GenerateVolume(GeoV3 start, GeoV3  end, 
	GeoV3 target_start, GeoV3  target_end, GeoV3 normal)
{
	//face: front (up, down) back(up,down)
	GeoV3 t = end - start;
	double edge_length = t.norm();
	t.normalize();

	GeoV3 p = cross(t, normal);
	p.normalize();

	GeoV3 start_cone_center = normal*extruder_.Height() + start;
	GeoV3 end_cone_center = normal*extruder_.Height() + end;

	//face front
	GeoV3 start_front_cone = start_cone_center + p*extruder_.Radii();
	GeoV3 start_front_cylinder = start_front_cone + normal*extruder_.CyclinderLenth();
	GeoV3 end_front_cone = end_cone_center + p*extruder_.Radii();
	GeoV3 end_front_cylinder = end_front_cone + normal*extruder_.CyclinderLenth();

	//face back
	GeoV3 start_back_cone = start_cone_center - p*extruder_.Radii();
	GeoV3 start_back_cylinder = start_back_cone + normal*extruder_.CyclinderLenth();
	GeoV3 end_back_cone = end_cone_center - p*extruder_.Radii();
	GeoV3 end_back_cylinder = end_back_cone + normal*extruder_.CyclinderLenth();

	bulk_.clear();
#ifdef  STRICT_COLLISION
	//Circle 
	GeoV3 q = cross(normal, p);
	q.normalize();
	vector<GeoV3> start_circle_point,end_circle_point;
	for (int i = 0; i < 16; i++)
	{
		double part = 2 * F_PI / 16;
		double theta = i*part;
		start_circle_point.push_back(start_cone_center + p*cos(theta)*extruder_.Radii() + q*sin(theta)*extruder_.Radii());
		end_circle_point.push_back(start_circle_point[i] + end - start);
	}

	for (int i = 0; i < 15; i++)
	{	
		bulk_.push_back(Triangle(start_circle_point[i], start_circle_point[i + 1], end_circle_point[i]));
		bulk_.push_back(Triangle(end_circle_point[i], end_circle_point[i + 1], start_circle_point[i + 1]));
	}
	bulk_.push_back(Triangle(start_circle_point[15], start_circle_point[0], end_circle_point[15]));
	bulk_.push_back(Triangle(end_circle_point[15], end_circle_point[0], start_circle_point[0]));

		for (int i = 0; i < 15; i++)
		{
			bulk_.push_back(Triangle(start_circle_point[i], start_circle_point[i + 1], end_circle_point[i]));
			bulk_.push_back(Triangle(end_circle_point[i], end_circle_point[i + 1], start_circle_point[i + 1]));
		}
		bulk_.push_back(Triangle(start_circle_point[15], start_circle_point[0], end_circle_point[15]));
		bulk_.push_back(Triangle(end_circle_point[15], end_circle_point[0], start_circle_point[0]));

	//Top face

	GeoV3 start_top_front_up = start + normal*(extruder_.TopLenth() / 2 + extruder_.TopCenter())+p*extruder_.TopRadii();
	GeoV3 start_top_front_down = start + normal*( extruder_.TopCenter() -extruder_.TopLenth() / 2 )+ p*extruder_.TopRadii();

	GeoV3 start_top_back_up =  start + normal*(extruder_.TopLenth() / 2 + extruder_.TopCenter()) - p*extruder_.TopRadii();
	GeoV3 start_top_back_down = start + normal*(extruder_.TopCenter() - extruder_.TopLenth() / 2) - p*extruder_.TopRadii();

	GeoV3 end_top_front_up = end + normal*(extruder_.TopLenth() / 2 + extruder_.TopCenter()) + p*extruder_.TopRadii();
	GeoV3 end_top_front_down = end + normal*(extruder_.TopCenter() - extruder_.TopLenth() / 2) + p*extruder_.TopRadii();

	GeoV3 end_top_back_up = end + normal*(extruder_.TopLenth() / 2 + extruder_.TopCenter()) - p*extruder_.TopRadii();
	GeoV3 end_top_back_down = end + normal*(extruder_.TopCenter() - extruder_.TopLenth() / 2) - p*extruder_.TopRadii();

	bulk_.push_back(Triangle(start_top_front_up, start_top_front_down, end_top_front_down));
	bulk_.push_back(Triangle(start_top_front_up, end_top_front_up, end_top_front_down));
	bulk_.push_back(Triangle(start_top_back_up, start_top_back_down, end_top_back_down));
	bulk_.push_back(Triangle(start_top_back_up, end_top_back_up, end_top_back_down));
#endif
	//front
	bulk_.push_back(Triangle(start, end, start_front_cone));
	bulk_.push_back(Triangle(end, end_front_cone, start_front_cone));
	bulk_.push_back(Triangle(start_front_cone, end_front_cone, start_front_cylinder));
	bulk_.push_back(Triangle(end_front_cone, end_front_cylinder, start_front_cylinder));

	//back
	bulk_.push_back(Triangle(start, end_back_cone, end));
	bulk_.push_back(Triangle(start, start_back_cone, end_back_cone));
	bulk_.push_back(Triangle(start_back_cone, end_back_cylinder, end_back_cone));
	bulk_.push_back(Triangle(start_back_cone, start_back_cylinder, end_back_cylinder));
}


void QuadricCollision::GenerateVolume(GeoV3 connect, GeoV3 target_s, GeoV3 order_s, GeoV3 normal)
{
	GeoV3 t = target_s - connect;
	double edge_length = t.norm();
	t.normalize();

	GeoV3 p = cross(t, normal);
	p.normalize();

	double eps = 1;

	GeoV3 start_cone_center = normal*extruder_.Height() + connect;

	//face front
	GeoV3 start_front_cone = start_cone_center + p*extruder_.Radii() + t*eps;
	GeoV3 start_front_cylinder = start_front_cone + normal*extruder_.CyclinderLenth() + t*eps;

	//face back
	GeoV3 start_back_cone = start_cone_center - p*extruder_.Radii() + t*eps;
	GeoV3 start_back_cylinder = start_back_cone + normal*extruder_.CyclinderLenth() + t*eps;
	GeoV3 start = connect + t*eps;

	//front
	bulk_.clear();
	bulk_.push_back(Triangle(start, start_back_cone, start_front_cone));
	bulk_.push_back(Triangle(start_front_cylinder, start_back_cone, start_front_cone));
	bulk_.push_back(Triangle(start_front_cylinder, start_back_cylinder, start_back_cone));





#ifdef  STRICT_COLLISION
	//Circle 
	start = connect;
	GeoV3 end = target_s;

	GeoV3 q = cross(normal, p);
	q.normalize();
	vector<GeoV3> start_circle_point, end_circle_point;
	for (int i = 0; i < 16; i++)
	{
		double part = 2 * F_PI / 16;
		double theta = i*part;
		start_circle_point.push_back(start_cone_center + p*cos(theta)*extruder_.Radii() + q*sin(theta)*extruder_.Radii());
		end_circle_point.push_back(start_circle_point[i] + end - start);
	}

	for (int i = 0; i < 15; i++)
	{
		bulk_.push_back(Triangle(start_circle_point[i], start_circle_point[i + 1], end_circle_point[i]));
		bulk_.push_back(Triangle(end_circle_point[i], end_circle_point[i + 1], start_circle_point[i + 1]));
	}
	bulk_.push_back(Triangle(start_circle_point[15], start_circle_point[0], end_circle_point[15]));
	bulk_.push_back(Triangle(end_circle_point[15], end_circle_point[0], start_circle_point[0]));

	//Top face
	GeoV3 start_top_front_up = start + normal*(extruder_.TopLenth() / 2 + extruder_.TopCenter()) + p*extruder_.TopRadii();
	GeoV3 start_top_front_down = start + normal*(extruder_.TopCenter() - extruder_.TopLenth() / 2) + p*extruder_.TopRadii();

	GeoV3 start_top_back_up = start + normal*(extruder_.TopLenth() / 2 + extruder_.TopCenter()) - p*extruder_.TopRadii();
	GeoV3 start_top_back_down = start + normal*(extruder_.TopCenter() - extruder_.TopLenth() / 2) - p*extruder_.TopRadii();

	GeoV3 end_top_front_up = end + normal*(extruder_.TopLenth() / 2 + extruder_.TopCenter()) + p*extruder_.TopRadii();
	GeoV3 end_top_front_down = end + normal*(extruder_.TopCenter() - extruder_.TopLenth() / 2) + p*extruder_.TopRadii();

	GeoV3 end_top_back_up = end + normal*(extruder_.TopLenth() / 2 + extruder_.TopCenter()) - p*extruder_.TopRadii();
	GeoV3 end_top_back_down = end + normal*(extruder_.TopCenter() - extruder_.TopLenth() / 2) - p*extruder_.TopRadii();

	bulk_.push_back(Triangle(start_top_front_up, start_top_front_down, end_top_front_down));
	bulk_.push_back(Triangle(start_top_front_up, end_top_front_up, end_top_front_down));
	bulk_.push_back(Triangle(start_top_back_up, start_top_back_down, end_top_back_down));
	bulk_.push_back(Triangle(start_top_back_up, end_top_back_up, end_top_back_down));
#endif
}


bool QuadricCollision::Parallel(GeoV3 a, GeoV3 b)
{
	if (abs(angle(a, b)) < GEO_EPS || abs(angle(a, b) - F_PI) < GEO_EPS)
		return true;
	return false;
}


double QuadricCollision::Distance(WF_edge* order_e)
{
	gte::Segment<3, float> segment, segment_target;
	segment = Seg(order_e->pvert_->Position(), order_e->ppair_->pvert_->Position());
	segment_target = Seg(target_e_->pvert_->Position(), target_e_->ppair_->pvert_->Position());
	gte::DCPQuery<float, gte::Segment<3, float>, gte::Segment<3, float>> distance;
	auto result = distance(segment, segment_target);
	return result.distance;
}

gte::Segment<3, float> QuadricCollision::Seg(point target_start, point target_end)
{
	gte::Segment<3, float> segment;
	segment.p[0][0] = target_start.x();
	segment.p[0][1] = target_start.y();
	segment.p[0][2] = target_start.z();
	segment.p[1][0] = target_end.x();
	segment.p[1][1] = target_end.y();
	segment.p[1][2] = target_end.z();
	return segment;
}

gte::Segment<3, float> QuadricCollision::Seg(GeoV3 target_start, GeoV3 target_end)
{
	gte::Segment<3, float> segment;
	segment.p[0][0] = target_start.getX();
	segment.p[0][1] = target_start.getY();
	segment.p[0][2] = target_start.getZ();
	segment.p[1][0] = target_end.getX();
	segment.p[1][1] = target_end.getY();
	segment.p[1][2] = target_end.getZ();
	return segment;

}

gte::Triangle<3, float> QuadricCollision::Tri(GeoV3 a, GeoV3 b, GeoV3 c)
{
	gte::Triangle<3, float> triangle;
	triangle.v[0][0] = a.getX();
	triangle.v[0][1] = a.getY();
	triangle.v[0][2] = a.getZ();
	triangle.v[1][0] = b.getX();
	triangle.v[1][1] = b.getY();
	triangle.v[1][2] = b.getZ();
	triangle.v[2][0] = c.getX();
	triangle.v[2][1] = c.getY();
	triangle.v[2][2] = c.getZ();
	return triangle;

}


void QuadricCollision::Debug()
{

//
//	int seq[100] = { 192, 190, 194, 196, 198, 10, 80, 126, 158, 178, 12, 182,
//		162, 164, 130, 132, 84, 86, 16, 18, 14, 82, 128, 160, 180, 38, 186, 170, 172,
//		142, 144, 102, 104, 42, 44, 40, 100, 140, 168, 184, 60, 188, 174, 176, 150, 152, 118,
//		120, 62, 64, 112, 30, 52, 72, 92, 76, 78, 56, 58, 34, 36, 32, 116, 96, 98, 114, 20, 54, 74, 94,
//		136, 138, 108, 110, 68, 70, 26, 28, 22, 156, 24, 66, 106, 134, 154, 46, 166, 146, 148, 122, 124, 88, 90, 48, 50, 0, 2, 4, 6, 8 };
//	vector<WF_edge*> exist_edge;
//	for (int i = 0; i < 21; i++)
//	{
//		exist_edge.push_back(ptr_frame_->GetEdge(seq[i]));
//	}
//
//	vector<GeoV3> angle;
////	angle = DetectStructure(ptr_frame_->GetEdge(seq[22]), exist_edge);
//		 
//
//
//	//-------------------------
//	point b;
//	
//	WF_edge *a = ptr_frame_->GetEdge(22);
//	b = a->pvert_->Position();
//	cout << b.x() << ", " <<b.y() << ", " <<b.z()<< ", " << endl;
//	a = a->ppair_;
//	b = a->pvert_->Position();
//	cout << b.x() << ", " << b.y() << ", " << b.z() << ", " << endl;
//	a = ptr_frame_->GetEdge(20);
//	b = a->pvert_->Position();
//	cout << b.x() << ", " << b.y() << ", " << b.z() << ", " << endl;
//	a = a->ppair_;
//	b = a->pvert_->Position();
//	cout << b.x() << ", " << b.y() << ", " << b.z() << ", " << endl;
//	
//	vector<lld> col_map;
//	DetectCollision(ptr_frame_->GetEdge(22), ptr_frame_->GetEdge(20),col_map);
//	cout << "           ---" << endl;
//	cout << DetectBulk(ptr_frame_->GetEdge(20), 0.314159, 1.25664) << endl;
//	double theta, phi;
//	for (int j = 0; j < 3; j++)
//	{
//		for (int i = 0; i < divide_; i++)
//		{
//				
//				if (i < 20)
//				{
//					theta = (j * 3 + 1)*18.0 / 180.0*F_PI;
//					phi = i*18.0 / 180.0*F_PI;
//				}
//				if (i>19 && i < 40)
//				{
//					theta = (j * 3 + 2) * 18.0 / 180.0*F_PI;
//					phi = (i - 20)*18.0 / 180.0*F_PI;
//				}
//				if (i>39)
//				{
//					theta = (j * 3 + 3)* 18.0 / 180.0*F_PI;
//					phi = (i - 40)*18.0 / 180.0*F_PI;
//				}
//				if (DetectBulk(ptr_frame_->GetEdge(20), theta, phi))
//			{			
//				continue;
//			}	
//				cout << theta << ", " << phi << ",0" << endl;					
//		}
//	}
}



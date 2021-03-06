#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "gen_grid.h"
#include "common.h"

#define READ_BUFFER_LINE	4096

GenMapGrid::GenMapGrid(string segs_file)
{
	this->_init();
}

int GenMapGrid::getErrno()
{
	return this->errno;
}

string GenMapGrid::getErrMsg()
{
	return this->errmsg;
}

void GenMapGrid::genGrid(string segs_file)
{
	FILE *fp;
	char buffer[READ_BUFFER_LINE];
	struct seg seg;
	vector<string> seg_info;

	if(segs_file == "")
		return;

	debug_msg("Start genGrid ... ...\n");

	seg_info.clear();

	fp = fopen(segs_file.c_str(), "r");
	while(NULL != fgets(buffer, READ_BUFFER_LINE, fp))
	{
		EchoRunning();
		seg_info = SplitBySep(buffer, "\t");
		if(seg_info.size() != 6)
			continue;
		seg.seg_id = atoi(seg_info[0].c_str());
		seg.start_lng = atof(seg_info[1].c_str());
		seg.start_lat = atof(seg_info[2].c_str());
		seg.end_lng = atof(seg_info[3].c_str());
		seg.end_lat = atof(seg_info[4].c_str());
		seg.edge_id = atoi(seg_info[5].c_str());
		this->updateGrids(seg);
	}
	fclose(fp);
	debug_msg("genGrid Finish.\n");
}

void GenMapGrid::dumpGrid(string dump_file="")
{
	int i, j;
	FILE *fp;

	if("" == dump_file)
		fp = stdout;
	else
	{
		fp = fopen(dump_file.c_str(), "a+");
	}

	debug_msg("Start dumpGrid ... ...\n");

	for(i = 0; i < lng_num; i++)
	{
		for(j = 0; j < lat_num; j++)
		{
			//format:i j start_lng start_lat end_lng end_lat seg_id1 ... seg_idn
			fprintf(fp, "%d\t%d\t%f\t%f\t%f\t%f", i, j, grid[i][j].start_lng, grid[i][j].start_lat,
					grid[i][j].end_lng, grid[i][j].end_lat);
			for(vector<seg>::iterator iter = grid[i][j].node_segs.begin(); iter != grid[i][j].node_segs.end(); iter++)
			{
				fprintf(fp, "\t%d", iter->seg_id);
			}
			fprintf(fp, "\n");
		}
	}

	if("" != dump_file)
	{
		fclose(fp);
	}

	debug_msg("dumpGrid Finish.\n");
}


//private functions

void GenMapGrid::_init()
{
	lat_num = lng_num = 0;
	start_lat = START_LAT;
	end_lat = END_LAT;
	start_lng = START_LNG;
	end_lng = END_LNG;
	lat_gap = LAT_GAP;
	lng_gap = LNG_GAP;

	this->_preprocess("round");
	this->_initGrid();
}

double GenMapGrid::_round(double val, double mod, char type)
{
#define MULTI	100000
	int i_val, i_mod;
	int tmp;

	i_val = val*MULTI;
	i_mod = mod*MULTI;

	if(0 == type)		//up bound
	{
		tmp = i_val % i_mod;
		if(0 != tmp)
			i_val = i_val - tmp + i_mod;
		val = (double)i_val / MULTI;
	}
	else if(1 == type)		//down bound
	{
		tmp = i_val % i_mod;
		if(0 != tmp)
			i_val = i_val - tmp;
		val = (double)i_val / MULTI;
	}
	else
	{
		return val;
	}

	return val;
}

void GenMapGrid::_preprocess(string type)
{
	if(0 == strcmp(type.c_str(), "round"))
	{
		debug_msg("%f\t%f\t%f\t%f\n", start_lat, end_lat, start_lng, end_lng);
		start_lat = this->_round(start_lat, lat_gap, 1);
		end_lat = this->_round(end_lat, lat_gap, 0);
		start_lng = this->_round(start_lng, lng_gap, 1);
		end_lng = this->_round(end_lng, lng_gap, 0);
	}

	debug_msg("%f\t%f\t%f\t%f\t%f\t%f\n", start_lat, end_lat, start_lng, end_lng, lat_gap, lng_gap);
	lat_num = fabs(end_lat - start_lat) / lat_gap + 1;
	lng_num = fabs(end_lng - start_lng) / lng_gap + 1;
	debug_msg("lat_num:%d\tlng_num:%d\n", lat_num, lng_num);
}

void GenMapGrid::_initGrid()
{
	int i, j;

	//grid = new grid_node[lng_num][lat_num];	//invalid, second dim should be const
	grid = new grid_node*[lng_num];
	for(i = 0; i < lng_num; i++)
	{
		grid[i] = new grid_node[lat_num];
	}

	/**
	 * first dimension is horizontal, lng
	 * second dimension is vertical, lat
	 */
	for(i = 0; i < lng_num; i++)
		for(j = 0; j < lat_num; j++) {
			grid[i][j].start_lng = this->start_lng + i*this->lng_gap;
			grid[i][j].end_lng = grid[i][j].start_lng + this->lng_gap;
			grid[i][j].start_lat = this->start_lat + j*this->lat_gap;
			grid[i][j].end_lat = grid[i][j].start_lat + this->lat_gap;
			grid[i][j].node_segs.clear();
		}
}

struct grid_index GenMapGrid::getGridIndexByPoint(double lng, double lat)
{
	int i=0, j=0;
	struct grid_index index;

	i = (lng - this->start_lng) / this->lng_gap + 1;
	j = (lat - this->start_lat) / this->lat_gap + 1;

	if(i >= this-> lng_num || j >= this->lat_num)
	{
		i = -1;
		j = -1;
	}

	if(i < 0 || j < 0) {
		/*
		cout << "getGridIndexByPoint: lng_num = " << lng_num << " lat_num = " << lat_num << endl;
		cout << "getGridIndexByPoint: i = " << i << " j = " << j << endl;
		cout << "getGridIndexByPoint start_lng:" << this->start_lng << " start_lat:" << this->start_lat << endl;
		cout << "getGridIndexByPoint lng:" << lng << " lat:" << lat << endl;
		*/
	}

	index.i = i;
	index.j = j;

	return index;
}

void GenMapGrid::updateGrid(struct grid_index index, struct seg seg)
{
	if(index.i < 0 || index.i > lng_num || index.j < 0 || index.j > lat_num)
	{
		cout << "index valid: i = " << index.i << " j = " << index.j << endl;
		return;
	}
	this->grid[index.i][index.j].node_segs.push_back(seg);
	//printf("i:%d\tj:%d\tsize:%d\n", index.i, index.j, grid[index.i][index.j].node_segs.size());
}

void GenMapGrid::updateHorizontalGrids(double k, double start_lng, double start_lat, 
		double end_lng, double end_lat, struct seg seg)
{
	double curr_lng, curr_lat;
	struct grid_index index={0, 0}, last_index={0, 0};
	
	curr_lng = start_lng;
	curr_lat = start_lat;

	while(curr_lng <= end_lng)
	{
		index = this->getGridIndexByPoint(curr_lng, curr_lat);
		last_index = index;
		this->updateGrid(index, seg);
		curr_lng = curr_lng + lng_gap;
	}

	if(curr_lng > end_lng)
	{
		curr_lng = end_lng;
		index = this->getGridIndexByPoint(curr_lng, curr_lat);
		if(last_index.i != index.i || last_index.j != index.j)
			this->updateGrid(index, seg);
	}

}

void GenMapGrid::updateVerticalGrids(double k, double start_lng, double start_lat, 
		double end_lng, double end_lat, struct seg seg)
{
	double curr_lng, curr_lat;
	struct grid_index index={0, 0}, last_index={0, 0};
	
	curr_lat = start_lat;
	curr_lng = start_lng;

	while(curr_lat <= end_lat)
	{
		index = this->getGridIndexByPoint(curr_lng, curr_lat);
		last_index = index;
		this->updateGrid(index, seg);
		curr_lat = curr_lat + lat_gap;
	}

	if(curr_lat > end_lat)
	{
		curr_lat = end_lat;
		index = this->getGridIndexByPoint(curr_lng, curr_lat);
		if(last_index.i != index.i || last_index.j != index.j)
			this->updateGrid(index, seg);
	}
}

void GenMapGrid::updateHorizOrientGrids(double k, double start_lng, double start_lat, 
		double end_lng, double end_lat, struct seg seg)
{
	double curr_lng, curr_lat;
	struct grid_index index={0, 0}, last_index={0, 0};

	curr_lng = start_lng;
	curr_lat = start_lat;

	while(curr_lng <= end_lng)
	{
		index = this->getGridIndexByPoint(curr_lng, curr_lat);
		last_index = index;
		this->updateGrid(index, seg);
		curr_lng = curr_lng + lng_gap;
		curr_lat = k * (curr_lng - start_lng) + start_lat;
	}

	if(curr_lng > end_lng)
	{
		curr_lng = end_lng;
		curr_lat = k * (curr_lng - start_lng) + start_lat;
		index = this->getGridIndexByPoint(curr_lng, curr_lat);
		if(last_index.i != index.i || last_index.j != index.j)
			this->updateGrid(index, seg);
	}
}

void GenMapGrid::updateVertOrientGrids(double k, double start_lng, double start_lat, 
		double end_lng, double end_lat, struct seg seg)
{	
	double curr_lng, curr_lat;
	struct grid_index index={0, 0}, last_index={0, 0};

	curr_lat = start_lat;
	curr_lng = start_lng;

	while(curr_lat <= end_lat)
	{
		index = this->getGridIndexByPoint(curr_lng, curr_lat);
		last_index = index;
		this->updateGrid(index, seg);
		curr_lat = curr_lat + lat_gap;
		curr_lng = (curr_lat - start_lat) / k + start_lng;
	}

	if(curr_lng > end_lng)
	{
		curr_lat = end_lat;
		curr_lng = (curr_lat - start_lat) / k + start_lng;
		index = this->getGridIndexByPoint(curr_lng, curr_lat);
		if(last_index.i != index.i || last_index.j != index.j)
			this->updateGrid(index, seg);
	}
}

void GenMapGrid::updateGrids(struct seg seg)
{
	double k = 0;
	double left_lat=0, left_lng=0, right_lat=0, right_lng=0;
	double buttom_lat=0, buttom_lng=0, top_lat=0, top_lng=0;
	// 1:horizontal		2:vertical		3:horiz_orientation	4:vertical_orientation
	int direction = 0;

	if(seg.start_lat == seg.end_lat) {

		k = 0;
		direction = 1;
	} else if(seg.start_lng == seg.end_lng) {

		k = HUGE_VAL;
		direction = 2;
	} else {

		double k = (start_lng - end_lng)/(start_lat - end_lat);
		double abs_k = abs(k);

		if(abs_k <= 1) {
			direction = 3;
		} else {
			direction = 4;
		}
	}

	if(2 == direction || 4 == direction) {

		if(seg.start_lng <= seg.end_lng) {

			buttom_lng = seg.start_lng;
			buttom_lat = seg.start_lat;
			top_lng = seg.end_lng;
			top_lat = seg.end_lat;
		} else {

			buttom_lng = seg.end_lng;
			buttom_lat = seg.end_lat;
			top_lng = seg.start_lng;
			top_lat = seg.start_lat;
		}

	} else if(1 == direction || 3 == direction) {

		if(seg.start_lat <= seg.end_lat) {

			left_lng = seg.start_lng;
			left_lat = seg.start_lat;
			right_lng = seg.end_lng;
			right_lat = seg.end_lat;
		} else {

			left_lng = seg.end_lng;
			left_lat = seg.end_lat;
			right_lng = seg.start_lng;
			right_lat = seg.start_lat;
		}
	}

	switch(direction)
	{
		case 0:
			break;
		case 1:
			updateHorizontalGrids(k, left_lng, left_lat, right_lng, right_lat, seg);
			break;
		case 2:
			updateVerticalGrids(k, buttom_lng, buttom_lat, top_lng, top_lat, seg);
			break;
		case 3:
			updateHorizOrientGrids(k, left_lng, left_lat, right_lng, right_lat, seg);
			break;
		case 4:
			updateVertOrientGrids(k, buttom_lng, buttom_lat, top_lng, top_lat, seg);
			break;
		default:
			break;
	}
}



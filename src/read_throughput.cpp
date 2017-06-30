#include <iostream>
#include <string>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>
#include <utility>
#include <iomanip>
#include <ctime>
#include <tuple>
#include <algorithm>
#include <cmath>
#include "utils.h"

#define d_debug false
#define DEBUG d_debug && std::cout

using namespace std;

bool parse_args(int argc, char** argv, string& thr)
{
	if(argc<2){
		return false;
	}
	string str(argv[1]);
	size_t dot_pos = str.find_first_of('.');
	if(dot_pos==string::npos){
		thr = str+".tmc";
	}else{
		string name = str.substr(0,dot_pos);
		thr = name +".tmc";
	}
	int count =3;
	while(count<argc){
		string tmp_str(argv[count]);
		if(tmp_str=="--tmc"){
			count++;
			if(count<argc){
				string crc(argv[count]);
				size_t check = crc.find_first_of('-');
				if(check==string::npos){
					thr = crc;
					count++;
				}
			}
		}
	}
	return true;
}

void read_result(fstream& file,
				 vector< pair<int,long> >& data)
{
	data.clear();
	string str, line;
	int pkt_cnt;
	long millisecs;
	while(getline(file,line,'\n')){
		if(line=="[Event]"){
			while(getline(file,line,'\n')){
				if(line=="[Event*]"){
					data.push_back(std::make_pair(pkt_cnt,millisecs));
				}else if(line=="<count>"){
					getline(file,line,'\n');
					pkt_cnt = std::atoi(line.c_str());
				}else if(line=="<duration>"){
					getline(file,line,'\n');
					millisecs = std::atol(line.c_str());
				}else{
					// unrecognized pattern
				}
			}
		}
	}
}

int count_and_write(fstream& file, vector<pair<int,long> >& data, float& avg)
{
	int pkt_num;
	long duration_acc=0;
	long duration;
	file<<"pkt_cnt,duration"<<std::endl;
	for(int i=0;i<data.size();++i){
		pkt_num = get<0>(data[i]);
		duration = get<1>(data[i]);
		duration_acc += duration;
		file<<pkt_num<<","<<duration<<std::endl;
	}
	avg = duration_acc/(float)data.size();
	return data.size();
}

int main(int argc, char** argv)
{
	string tmc_file;
	if(!parse_args(argc,argv,tmc_file)){
		cout<<"<USAGE> throughput_reader [data file] (--tmc time_count_file)"<<std::endl;
		return 0;
	}
	std::vector< std::pair<int,long> > d_data;
	fstream d_file, d_file_result;
	DEBUG<<"reading file:"<<argv[1]<<std::endl;
	d_file.open(argv[1],std::fstream::in);
	if(!d_file.is_open()){
		std::cerr<<"ERROR: source file cannot be opened..."<<std::endl;
		return 1;
	}
	read_result(d_file,d_data);
	d_file.close();

	// count and write result
	d_file_result.open(tmc_file.c_str(),fstream::out|fstream::trunc);
	if(!d_file_result.is_open()){
		std::cerr<<"ERROR: cannot write to file..."<<std::endl;
		return 1;
	}
	float d_avg_duration;
	int total=count_and_write(d_file_result,d_data,d_avg_duration);
	d_file_result.close();
	
	cout<<setfill('*')<<setw(30)<<"RESULT"<<setfill('*')<<setw(30)<<"*"<<std::endl;
	cout<<left<<setfill(' ')<<setw(20)<<"Events count:"<<total<<std::endl;
	cout<<left<<setfill(' ')<<setw(20)<<"Average duration:"<<fixed<<setprecision(4)<<d_avg_duration<<std::endl;

	return 0;
}

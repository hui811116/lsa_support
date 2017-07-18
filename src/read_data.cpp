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

enum WRITETYPE{
	SNR,
	LIST
	};
bool parse_args(int argc, char** argv, string& ber, string& thr, WRITETYPE& type)
{
	if(argc<3){
		return false;
	}
	string str(argv[2]);
	size_t dot_pos = str.find_first_of('.');
	if(dot_pos==string::npos){
		ber = str+".ber";
		thr = str+".thr";
	}else{
		string name = str.substr(0,dot_pos);
		ber = name +".ber";
		thr = name +".thr";
	}
	int count =3;
	while(count<argc){
		string tmp_str(argv[count]);
		if(tmp_str == "--ber"){
			count++;
			if(count<argc){
				string crc(argv[count]);
				size_t check = crc.find_first_of('-');
				if(check==string::npos){
					ber = crc;
					count++;
				}
			}
		}else if(tmp_str=="--thr"){
			count++;
			if(count<argc){
				string crc(argv[count]);
				size_t check = crc.find_first_of('-');
				if(check==string::npos){
					thr = crc;
					count++;
				}
			}
		}else if(tmp_str=="--list"){
			count++;
			type = LIST;
		}else{
			return false;
		}
	}
	return true;
}

void read_data(fstream& file,std::vector< vector<unsigned char> >& src)
{
    std::string str,line;
    while(getline(file,line,'\n')){
        std::istringstream temp(line);
        std::vector<unsigned char> u8;
        while(getline(temp,str,',')){
            int tmp = std::atoi(str.c_str());
            u8.push_back((unsigned char)tmp);
        }
        src.push_back(u8);
    }
}

void read_result(fstream& file, 
                 std::vector< std::tuple<int,std::vector<unsigned char>,float> >& data,
                 std::vector<std::tuple<time_t,int,int> >& events)
{
    data.clear();
    events.clear();
    std::string str,line;
    int seqno, byte_size;
    struct tm tm;
    time_t parsed_time;
    int ins_bytes, ins_pkts;
	float pwr = 0.0;
    while(getline(file,line,'\n')){
        if(line=="[Event]"){
            while(getline(file,line,'\n')){
                if(line=="[Event*]"){
                    // put tuples(time, bytes, pkts) to a buffer for throughput reconstruction
                    auto obj = std::make_tuple(parsed_time,ins_bytes,ins_pkts);
                    events.push_back(obj);
                    DEBUG << "Time event:"<<ctime(&std::get<0>(obj))<<" ,bytes:"<<std::get<1>(obj)<<" ,pkts:"<<std::get<2>(obj)<<std::endl;
                    break;
                }else if(line=="<Time>"){
                    getline(file,line,'\n');
                    strptime(line.c_str(),"%c",&tm);
                    parsed_time = mktime(&tm);
                }else if(line=="<Bytes>"){
                    getline(file,line,'\n');
                    ins_bytes = atoi(line.c_str());
                }else if(line=="<Packets>"){
                    getline(file,line,'\n');
                    ins_pkts = atoi(line.c_str());
                }else{
                    // unrecognized pattern
                }
            }
        }else if(line == "[LSA]"|| line=="[SNS]"||line=="[PROU]"){
            while(getline(file,line,'\n')){
                if(line=="[LSA*]"||line=="[SNS*]"||line=="[PROU*]"){
                    break;
                }else if(line=="<seq>"){
                    getline(file,line,'\n');
                    seqno = atoi(line.c_str());
                }else if(line=="<pwr>"){
			getline(file,line,'\n');
			pwr = atof(line.c_str());
		}else if(line=="<size>"){
			getline(file,line,'\n');
			byte_size = atoi(line.c_str());
                }else if(line=="[Hex]"){
                     // payload bytes
                     getline(file,line,'\n');
                     std::istringstream temp(line);
                     std::vector<unsigned char> u8;
                     while(getline(temp,str,',')){
                         unsigned char hexint= std::strtol(str.c_str(),NULL,16);
                         u8.push_back(hexint);
                     }
                     if(u8.size()==byte_size){
                         // size matched
			 auto temp_data = std::make_tuple(seqno,u8,pwr);
                         data.push_back(temp_data);
                     }else{
                         DEBUG<<"<WARNING> parsed size:"<<u8.size()<<" ,nominal:"<<byte_size<<" begin:"<<(int)u8[0]<<" ,end:"<<(int)u8[u8.size()-1]<<std::endl;
                     }
                     while(getline(file,line,'\n')){
                         if(line=="[Hex*]"){
                             break;
                         }
                     }
                }else{
                    // empty line or data not relevent
                }
            }
        }else{

        }
    }
}


int err_count(
		std::vector<int>& bits_count,
		std::vector<float>& pwr_count,
		std::vector<int>& pkt_bytes,	
		const std::vector< std::vector<unsigned char> >& data_src, 
		const std::vector<std::tuple<int,std::vector<unsigned char>,float> >& result)
{
	int total=0;
	bits_count.clear();
	pwr_count.clear();
	pkt_bytes.clear();
	for(int i=0;i<result.size();++i){
		int seq = std::get<0>(result[i]);
		int count =0;
		std::vector<unsigned char> tmp_bytes = std::get<1>(result[i]);
		if(seq>=data_src.size()){
			std::cerr<<"<Warning> found a sequence number not in the range of data source"<<std::endl;
			continue;
		}
		if(tmp_bytes.size()!=data_src[seq].size()){
			std::cerr<<"<Warning> the number of received bytes and data bytes are not matched"<<std::endl;
			continue;
		}
		for(int j=0;j<data_src[seq].size();++j){
			count += count_bits(tmp_bytes[j],data_src[seq].at(j));
		}
		total+=count;
		bits_count.push_back(count);
		pwr_count.push_back(std::get<2>(result[i]));
		pkt_bytes.push_back(data_src[seq].size());
	}
	return total;
}

double throughput_count(
		std::vector<double>& seconds,
		std::vector<int>& bytes,
		const std::vector< std::tuple<time_t,int,int> >& events)
{
	seconds.clear();
	bytes.clear();
	double time_acc=0;
	for(int i=0;i<events.size()-1;++i){
		time_t begin = std::get<0>(events[i]);
		time_t end = std::get<0>(events[i+1]);
		int bytes_acc = std::get<1>(events[i+1]);
		double diff = std::difftime(end,begin);
		seconds.push_back(diff);
		bytes.push_back(bytes_acc);
		time_acc+= diff;
	}
	return time_acc;
}

void ber_calc_and_write(
	std::fstream& file, 
	const std::vector<float>& pwrs, 
	const std::vector<int>& err_bits,
	const std::vector<int>& pkts)
{
	std::vector<float>::const_iterator it;
	it = std::min_element(pwrs.begin(),pwrs.end());
	float min = *it;
	it = std::max_element(pwrs.begin(),pwrs.end());
	float max = *it;
	float min_pwr = std::round(min);
	float max_pwr = std::round(max);
	int interval = (max_pwr-min_pwr)+1;
	std::vector<int> err_buf(interval);
	std::vector<int> acc_buf(interval);
	std::vector<float> pwr_level(interval);
	for(int i=0;i<interval;++i){
		pwr_level[i] = min_pwr + i;
	}
	for(int i=0;i<err_bits.size();++i){
		float rnd = std::round(pwrs[i]);
		int idx = (int)floor(rnd-min_pwr);
		err_buf[idx]+=err_bits[i];
		acc_buf[idx]+=pkts[i];
	}
	file<<"pwr,acc,err,rate"<<std::endl;
	for(int i=0;i<interval;i++){
		if(acc_buf[i]==0){
			continue;
		}
		double tmp_rate = err_buf[i]/(double)acc_buf[i];
		file<<pwr_level[i]<<","<<acc_buf[i]<<","<<err_buf[i]<<","<<tmp_rate<<std::endl;
	}
}

void list_err_bits(
	std::fstream& file,
	const std::vector<int>& err_bits,
	const std::vector<int>& pkts
	)
{
	file<<"pktsize,err"<<std::endl;
	for(int i=0;i<err_bits.size();++i){
		file<<pkts[i]<<","<<err_bits[i]<<std::endl;
	}
}

void thr_calc_and_write(
	std::fstream& file,
	const std::vector<double>& seconds,
	const std::vector<int>& bytes)
{
	std::vector<int> time_recs;
	int time_acc=0;
	std::vector<float> avg_rates;
	for(int i=0;i<seconds.size();++i){
		int duration = (int)seconds[i];
		for(int j=0;j<duration;++j){
			float tmp_rate = bytes[i]/seconds[i];
			avg_rates.push_back(tmp_rate);
			time_recs.push_back(time_acc++);
		}
	}
	file<<"time,rate"<<std::endl;
	for(int i=0;i<time_recs.size();++i){
		file<<time_recs[i]<<","<<avg_rates[i]<<std::endl;
	}
}

int main(int argc,char** argv)
{
	string ber_file, thr_file;
	WRITETYPE type = SNR;
	if(!parse_args(argc,argv,ber_file,thr_file,type)){
		std::cout<<"<USAGE> data_reader [data_file] [result_file] <arguments>"<<std::endl;
		std::cout<<"-----------------------------------------------------------"<<std::endl;
		std::cout<<"Arguments:"<<std::endl;
		std::cout<<"--ber [filename]: specify filename of BER result"<<std::endl;
		std::cout<<"--thr [filename]: specify filename of Throughput result"<<std::endl;
		std::cout<<"--list: change write mode to LIST (default:SNR)"<<std::endl;
		return 0;
	}
	std::vector< std::vector<unsigned char> > d_data_src;
	fstream d_file_data, d_file_result;
	DEBUG<<"reading file:"<<argv[1]<<std::endl;
	d_file_data.open(argv[1],std::fstream::in);
	if(!d_file_data.is_open()){
		cerr<<"ERROR: data file cannot be opened..."<<std::endl;
		return 1;
	}
	// read data source
	read_data(d_file_data,d_data_src);
	d_file_data.close();


	DEBUG<<"reading file:"<<argv[2]<<std::endl;
	d_file_result.open(argv[2],std::fstream::in);
	if(!d_file_result.is_open()){
		cerr<<"ERROR: result file cannot be opened..."<<std::endl;
		return 1;
	}
	std::vector<std::tuple<int,std::vector<unsigned char>,float> > d_result;
	std::vector<std::tuple<time_t,int,int>> d_events;
	// read result
	read_result(d_file_result,d_result,d_events);
	d_file_result.close();

	// counting ber
	std::vector<int> d_err_bits;
	std::vector<float> d_pwr_recs;
	std::vector<int> d_pkt_bytes;
	DEBUG<<"counting error bits"<<std::endl;
	int d_total_count = err_count(
				d_err_bits,
				d_pwr_recs, 
				d_pkt_bytes,
				d_data_src,
				d_result);
	// counting throughput
	std::vector<double> d_thr_seconds;
	std::vector<int> d_thr_bytes;
	DEBUG<<"counting throughput rates"<<std::endl;
	double d_time_acc = throughput_count(d_thr_seconds,d_thr_bytes,d_events);

	std::cout<<setfill('*')<<setw(30)<<"RESULT"<<setfill('*')<<setw(30)<<"*"<<std::endl;
	std::cout<<std::left<<setfill(' ')<<setw(20)<<"Data-source size"<<":"<<d_data_src.size()<<std::endl;
	std::cout<<std::left<<setfill(' ')<<setw(20)<<"Result-payload size"<<":"<<d_result.size()<<std::endl;
	std::cout<<std::left<<setfill(' ')<<setw(20)<<"Result-time events"<<":"<<d_events.size()<<std::endl;
	std::cout<<std::left<<setfill(' ')<<setw(20)<<"Result-error bits"<<":"<<d_total_count<<std::endl;
	std::cout<<setfill('*')<<setw(60)<<"*"<<std::endl;

	// integrating results
	fstream d_out_file;
	DEBUG<<"writing file--"<<ber_file<<std::endl;
	d_out_file.open(ber_file.c_str(),std::fstream::out | std::fstream::trunc);
	if(!d_out_file.is_open()){
		std::cerr<<"ERROR:file cannot be opened"<<std::endl;
		return 1;
	}
	switch(type){
		case SNR:
			ber_calc_and_write(d_out_file,d_pwr_recs,d_err_bits, d_pkt_bytes);
		break;
		case LIST:
			
		break;
		default:
			std::cerr<<"Undefined write mode";
			return 1;
		break;
	}
	d_out_file.close();

	// integrating throughput
	fstream d_thr_file;
	DEBUG<<"writing throughput--"<<thr_file<<std::endl;
	d_thr_file.open(thr_file.c_str(),std::fstream::out | std::fstream::trunc);
	if(!d_thr_file.is_open()){
		std::cerr<<"ERROR:file connot be opened"<<std::endl;
		return 1;
	}
	thr_calc_and_write(d_thr_file,d_thr_seconds,d_thr_bytes);
	d_thr_file.close();
	return 0;
}

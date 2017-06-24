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
#include "utils.h"

#define d_debug false
#define DEBUG d_debug && std::cout

using namespace std;

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


int err_count(std::vector<int>& bits_count,
			  std::vector<float>& pwr_count,
			  const std::vector< std::vector<unsigned char> >& data_src, 
			  const std::vector<std::tuple<int,std::vector<unsigned char>,float> > result)
{
	int total=0;
	bits_count.clear();
	for(int i=0;i<result.size();++i){
		int seq = std::get<0>(result[i]);
		int count =0;
		std::vector<unsigned char> tmp_bytes = std::get<1>(result[i]);
		if(seq>=data_src.size()){
			DEBUG<<"<Warning> found a sequence number not in the range of data source"<<std::endl;
			continue;
		}
		if(tmp_bytes.size()!=data_src[seq].size()){
			DEBUG<<"<Warning> the number of received bytes and data bytes are not matched"<<std::endl;
			continue;
		}
		for(int j=0;j<data_src[seq].size();++j){
			count += count_bits(tmp_bytes[j],data_src[seq].at(j));
		}
		total+=count;
		bits_count.push_back(count);
		pwr_count.push_back(std::get<2>(result[i]));
	}
	return total;
}

int main(int argc,char** argv)
{
    if(argc<3){
        std::cout<<"<USAGE> data_reader [data_file] [result_file]"<<std::endl;
        return 0;
    }
    std::vector< std::vector<unsigned char> > d_data_src;
    fstream d_file_data, d_file_result;
    DEBUG<<"reading file:"<<argv[1]<<std::endl;
    d_file_data.open(argv[1],std::fstream::in);
	// read data source
    read_data(d_file_data,d_data_src);
    d_file_data.close();


    DEBUG<<"reading file:"<<argv[2]<<std::endl;
    d_file_result.open(argv[2],std::fstream::in);
    std::vector<std::tuple<int,std::vector<unsigned char>,float> > d_result;
    std::vector<std::tuple<time_t,int,int>> d_events;
	// read result
    read_result(d_file_result,d_result,d_events);
    d_file_result.close();

	// counting ber
	std::vector<int> d_err_bits;
	std::vector<float> d_pwr_recs;
	DEBUG<<"counting error bits"<<std::endl;
	int d_total_count = err_count(d_err_bits,d_pwr_recs,d_data_src,d_result);

    std::cout<<setfill('*')<<setw(30)<<"RESULT"<<setfill('*')<<setw(30)<<"*"<<std::endl;
    std::cout<<std::left<<setfill(' ')<<setw(20)<<"Data-source size"<<":"<<d_data_src.size()<<std::endl;
    std::cout<<std::left<<setfill(' ')<<setw(20)<<"Result-payload size"<<":"<<d_result.size()<<std::endl;
    std::cout<<std::left<<setfill(' ')<<setw(20)<<"Result-time events"<<":"<<d_events.size()<<std::endl;
    std::cout<<std::left<<setfill(' ')<<setw(20)<<"Result-error bits"<<":"<<d_total_count<<std::endl;
	std::cout<<setfill('*')<<setw(60)<<"*"<<std::endl;

    return 0;
}

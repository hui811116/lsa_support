#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <iomanip>

using namespace std;

int main(int argc, char** argv){
	vector< vector<int> > data;
	fstream file;
	cout<<"argv1:"<<argv[1]<<std::endl;
	file.open(argv[1]);
	string line;
	while(getline(file,line,'\n')){
		istringstream temp(line);
		string str;
		vector<int> tmp_data;
		vector<unsigned char> u8_data;
		while(getline(temp,str,',')){
			int tmp = atoi(str.c_str());
			tmp_data.push_back(atoi(str.c_str()));
			if(tmp<128 && tmp>=0)
				u8_data.push_back((unsigned char)tmp);
		}
		data.push_back(tmp_data);
	}
	cout<<"total lines:"<<data.size()<<endl;
	cout<<"first content size:"<<data[0].size()<<endl;
	cout<<hex;
	for(int i=0;i<data[0].size();++i)
	 cout<<setfill('0')<<setw(2)<<data[0].at(i)<<" ";
	cout<<dec<<endl;
	file.close();
	return 0;
}
